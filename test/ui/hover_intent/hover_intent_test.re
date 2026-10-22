open Tea;
open Test_support;
module H = Ui.Hover_intent;
/* Deterministic timer fixture around the real module's setTimeout task. It
   verifies physical cancellation, not merely suppression of model changes. */
type timer = {
  id: int,
  at: int,
  callback: unit => unit,
};
let timers = ref([]);
let clock = ref(0);
let next_id = ref(0);
let fired = ref(0);
let schedule = (callback, ms) => {
  incr(next_id);
  timers :=
    timers^
    @ [
      {
        id: next_id^,
        at: clock^ + ms,
        callback,
      },
    ];
  next_id^;
};
let cancel = id => timers := List.filter(timer => timer.id != id, timers^);
let advance = ms => {
  let target = clock^ + ms;
  let rec drain = () =>
    switch (List.find_opt(timer => timer.at <= target, timers^)) {
    | None => clock := target
    | Some(timer) =>
      clock := timer.at;
      cancel(timer.id);
      incr(fired);
      timer.callback();
      drain();
    };
  drain();
  frame();
};
let pending = () => List.length(timers^);
[@mel.scope "Object"]
external define: ('a, string, Js.t({..})) => unit = "defineProperty";
let hover = (~kind="mouse", ~stamp=100., selector, name) => {
  let event =
    pointer_event(
      name,
      {
        "bubbles": false,
        "cancelable": true,
        "pointerType": kind,
      },
    );
  define(event, "timeStamp", { "value": stamp });
  check("hover never prevents default", dispatch(node(selector), event));
};
type entry = {
  key: string,
  hover: H.model,
};
type msg =
  | Intent(string, H.msg)
  | Set(string, bool)
  | Disabled(bool)
  | Remove(string)
  | Group(H.Group.t);
type model = {
  entries: list(entry),
  disabled: bool,
  group: H.Group.t,
  out: list(string),
  last: option(H.msg),
};
let disabled = (key, model) =>
  String.equal(key, "off") || String.equal(key, "a") && model.disabled;
let update = (msg, model) =>
  switch (msg) {
  | Disabled(value) => (
      {
        ...model,
        disabled: value,
        entries:
          List.map(
            entry =>
              if (String.equal(entry.key, "a")) {
                let (hover, _) = H.close_now(entry.hover);
                {
                  ...entry,
                  hover,
                };
              } else {
                entry;
              },
            model.entries,
          ),
      },
      Cmd.none,
    )
  | Group(group) => (
      {
        ...model,
        group,
      },
      Cmd.none,
    )
  | Remove(key) => (
      {
        ...model,
        entries:
          List.filter(entry => !String.equal(key, entry.key), model.entries),
      },
      Cmd.none,
    )
  | Intent(key, intent) =>
    let out = ref(model.out);
    let group = ref(model.group);
    let cmd = ref(Cmd.none);
    let entries =
      List.map(
        entry =>
          if (!String.equal(key, entry.key)) {
            entry;
          } else {
            let (hover, effects, messages) =
              H.update(
                ~disabled=disabled(key, model),
                ~group=model.group,
                intent,
                entry.hover,
              );
            cmd := Cmd.map(intent => Intent(key, intent), effects);
            List.iter(
              message => {
                out :=
                  out^
                  @ [
                    key
                    ++ (
                      switch (message) {
                      | H.Intent_open => "+"
                      | H.Intent_close => "-"
                      }
                    ),
                  ];
                switch (message) {
                | H.Intent_close =>
                  Option.iter(
                    time_stamp =>
                      group := H.Group.record_close(~time_stamp, group^),
                    H.last_close_time(hover),
                  )
                | H.Intent_open => ()
                };
              },
              messages,
            );
            {
              ...entry,
              hover,
            };
          },
        model.entries,
      );
    (
      {
        ...model,
        entries,
        out: out^,
        group: group^,
        last: Some(intent),
      },
      cmd^,
    );
  | Set(key, opened) =>
    let cmd = ref(Cmd.none);
    let entries =
      List.map(
        entry =>
          if (!String.equal(key, entry.key)) {
            entry;
          } else {
            let (hover, effects) =
              opened ? H.open_now(entry.hover) : H.close_now(entry.hover);
            cmd := Cmd.map(intent => Intent(key, intent), effects);
            {
              ...entry,
              hover,
            };
          },
        model.entries,
      );
    (
      {
        ...model,
        entries,
      },
      cmd^,
    );
  };
let view = model =>
  Html.div(
    [],
    List.map(
      entry => {
        let render = (trigger_attrs, content_attrs, opened) =>
          Html.div(
            [],
            [
              Html.button(
                trigger_attrs
                @ [
                  Html.id(entry.key),
                  Html.type_("button"),
                  Html.aria("expanded", opened ? "true" : "false"),
                  Html.aria("controls", entry.key ++ "-content"),
                ],
                [Html.text(entry.key)],
              ),
              Html.div(
                content_attrs
                @ [Html.id(entry.key ++ "-content"), Html.hidden(!opened)],
                [Html.text("Content")],
              ),
            ],
          );
        let to_parent = intent => Intent(entry.key, intent);
        String.equal(entry.key, "a")
          ? H.view(
              ~model=entry.hover,
              ~disabled=disabled(entry.key, model),
              ~to_parent,
              ~to_view=
                parts =>
                  render(
                    parts.trigger_attrs,
                    parts.content_attrs,
                    parts.is_open,
                  ),
              (),
            )
          : render(
              H.trigger_attrs(
                ~model=entry.hover,
                ~disabled=disabled(entry.key, model),
                ~to_parent,
                (),
              ),
              H.content_attrs(
                ~model=entry.hover,
                ~disabled=disabled(entry.key, model),
                ~to_parent,
                (),
              ),
              H.is_open(entry.hover),
            );
      },
      model.entries,
    ),
  );
let completed = {
  let original_timeout: (unit => unit, int) => int =
    get(global, "setTimeout");
  let original_clear: int => unit = get(global, "clearTimeout");
  put(global, "setTimeout", schedule);
  put(global, "clearTimeout", cancel);
  let baseline = listener_count();
  let init = () => (
    {
      entries:
        List.map(
          key =>
            {
              key,
              hover:
                H.init(
                  ~open_delay_ms=String.equal(key, "instant") ? (-1) : 100,
                  ~close_delay_ms=String.equal(key, "instant") ? (-1) : 80,
                  (),
                ),
            },
          ["a", "b", "instant", "off"],
        ),
      disabled: false,
      group: H.Group.init(~skip_delay_ms=0, ()),
      out: [],
      last: None,
    },
    Cmd.none,
  );
  let app =
    App.element(
      ~init,
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch(
            List.map(
              entry =>
                Sub.map(
                  intent => Intent(entry.key, intent),
                  H.subscriptions(~key=entry.key, entry.hover),
                ),
              model.entries,
            ),
          ),
      ~node="#app",
      ~flags=(),
      (),
    );
  let opened = key => attr("#" ++ key, "data-open") == Some("");
  let count = () => List.length(app.model().out);
  let reset = key => {
    app.dispatch(Set(key, false));
    frame();
  };
  check(
    "initial closed and no timers/listeners",
    !opened("a") && pending() == 0 && listener_count() == baseline,
  );
  check(
    "data closed presence and consumer ARIA",
    attr("#a", "data-closed") == Some("")
    && attr("#a", "aria-expanded") == Some("false")
    && attr("#a", "aria-controls") == Some("a-content"),
  );
  check(
    "consumer hides content without CSS",
    get(node("#a-content"), "hidden"),
  );
  check(
    "no component semantics leaked",
    attr("#a-content", "role") == None
    && attr("#off", "aria-disabled") == None,
  );
  hover("#a", "pointerenter");
  frame();
  check(
    "enter pending not open",
    pending() == 1 && !opened("a") && count() == 0,
  );
  advance(99);
  check("full opening delay", !opened("a"));
  hover("#a", "pointerenter");
  advance(1);
  check(
    "duplicate enter does not restart opening",
    opened("a") && count() == 1 && pending() == 0,
  );
  check(
    "open state and hidden property",
    attr("#a", "data-closed") == None
    && attr("#a-content", "data-open") == Some("")
    && attr("#a", "aria-expanded") == Some("true")
    && !get(node("#a-content"), "hidden"),
  );
  let stale_elapsed = app.model().last;
  hover("#a", "pointerleave");
  advance(30);
  check("leave grace", opened("a") && pending() == 1);
  hover("#a-content", "pointerenter");
  frame();
  check(
    "content reentry physically cancels close",
    opened("a") && pending() == 0,
  );
  advance(200);
  check("stale close never fires", opened("a") && count() == 1);
  hover("#a-content", "pointerleave");
  advance(40);
  hover("#a", "pointerenter");
  advance(100);
  check(
    "return to trigger across gap",
    opened("a") && count() == 1 && pending() == 0,
  );
  hover("#a", "pointerleave");
  advance(40);
  hover("#a", "pointerleave");
  advance(40);
  check(
    "duplicate leave preserves shared deadline",
    !opened("a") && count() == 2,
  );
  check(
    "closed timer records monotonic time",
    Option.is_some(H.last_close_time(List.hd(app.model().entries).hover)),
  );
  /* Replayed valid-but-old timer message must be rejected by the model itself. */
  hover("#a", "pointerenter");
  Option.iter(intent => app.dispatch(Intent("a", intent)), stale_elapsed);
  frame();
  check(
    "generation rejects replayed elapsed message",
    !opened("a") && pending() == 1 && count() == 2,
  );
  reset("a");
  let before = fired^;
  advance(1000);
  check(
    "close_now cancels timeout, no output",
    fired^ == before && count() == 2 && listener_count() == baseline,
  );
  /* Multiple intents without a render cannot close over stale model state. */
  hover("#a", "pointerenter");
  hover("#a", "pointerleave");
  hover("#a", "pointerenter");
  check("rapid events leave one timer", pending() == 1);
  advance(99);
  check("rapid enter delay remains intact", !opened("a"));
  advance(1);
  check("rapid enter opens once", opened("a") && count() == 3);
  reset("a");
  hover("#a", "pointerenter");
  hover("#a", "pointerleave");
  advance(200);
  check(
    "rapid enter leave never opens",
    !opened("a") && count() == 3 && pending() == 0,
  );
  List.iter(
    selector => {
      hover(~kind="touch", selector, "pointerenter");
      hover(~kind="touch", selector, "pointerleave");
      hover(~kind="touch", selector, "pointercancel");
    },
    ["#a", "#a-content"],
  );
  advance(200);
  check("touch never creates intent", count() == 3 && pending() == 0);
  hover(~kind="pen", "#a", "pointerenter");
  advance(100);
  check("pen hovers", opened("a") && count() == 4);
  hover(~kind="touch", "#a", "pointerleave");
  advance(200);
  check("touch cannot end a pen hover", opened("a"));
  hover(~kind="pen", "#a", "pointercancel");
  advance(80);
  check("pointercancel ends region", !opened("a") && count() == 5);
  hover("#off", "pointerenter");
  hover("#off-content", "pointerenter");
  advance(200);
  check(
    "all disabled regions emit no intents",
    count() == 5
    && pending() == 0
    && attr("#off", "data-disabled") == Some("")
    && !opened("off"),
  );
  hover("#a", "pointerenter");
  app.dispatch(Disabled(true));
  advance(200);
  check(
    "disable during delay cancels",
    !opened("a") && count() == 5 && pending() == 0,
  );
  /* A stale enabled view still emits an intent; current update flag wins. */
  app.dispatch(Disabled(false));
  frame();
  app.dispatch(Disabled(true));
  hover("#a", "pointerenter");
  advance(200);
  check("disabled guard is update-time", !opened("a") && count() == 5);
  app.dispatch(Disabled(false));
  frame();
  hover("#instant", "pointerenter");
  frame();
  check(
    "negative delay clamps to synchronous zero",
    opened("instant") && pending() == 0 && count() == 6,
  );
  hover("#instant", "pointerleave");
  frame();
  check(
    "zero close is synchronous",
    !opened("instant") && pending() == 0 && count() == 7,
  );
  focus(node("#a"));
  check("focus ignored by primitive", has_focus("#a") && !opened("a"));
  List.iter(
    name =>
      List.iter(
        value =>
          check("all keyboard rows untouched", key("#a", name, value)),
        ["Tab", "Escape", "Enter", " ", "ArrowDown"],
      ),
    ["keydown", "keyup"],
  );
  check("shift tab untouched", key(~shift=true, "#a", "keydown", "Tab"));
  check("ctrl chord untouched", key(~ctrl=true, "#a", "keydown", "Enter"));
  check("IME untouched", key(~composing=true, "#a", "keydown", "Escape"));
  app.dispatch(Set("a", true));
  frame();
  check(
    "open_now no intent or focus movement",
    opened("a") && count() == 7 && has_focus("#a"),
  );
  app.dispatch(Set("a", false));
  frame();
  check(
    "close_now no intent or focus movement",
    !opened("a") && count() == 7 && has_focus("#a"),
  );
  /* Independent source identities: both open, both receive visibility loss. */
  hover("#a", "pointerenter");
  hover("#b", "pointerenter");
  advance(100);
  check(
    "independent instance timers",
    opened("a") && opened("b") && count() == 9,
  );
  define(
    document,
    "visibilityState",
    {
      "value": "hidden",
      "configurable": true,
    },
  );
  ignore(
    dispatch(document, event("visibilitychange", { "bubbles": false })),
  );
  frame();
  check(
    "all instances receive hidden document",
    !opened("a")
    && !opened("b")
    && count() == 11
    && listener_count() == baseline,
  );
  define(
    document,
    "visibilityState",
    {
      "value": "visible",
      "configurable": true,
    },
  );
  hover("#a", "pointerenter");
  frame();
  ignore(dispatch(window, event("blur", { "bubbles": false })));
  advance(200);
  check(
    "window blur cancels unopened intent",
    !opened("a") && count() == 11 && pending() == 0,
  );
  hover("#a", "pointerenter");
  advance(100);
  let out =
    pointer_event(
      "pointerout",
      {
        "bubbles": true,
        "pointerType": "mouse",
        "relatedTarget": Js.Nullable.null,
      },
    );
  ignore(dispatch(document, out));
  advance(79);
  check("window exit shares grace", opened("a"));
  advance(1);
  check("window exit closes", !opened("a") && count() == 13);
  hover("#a", "pointerenter");
  frame();
  let internal =
    pointer_event(
      "pointerout",
      {
        "bubbles": true,
        "pointerType": "mouse",
        "relatedTarget": node("#b"),
      },
    );
  ignore(dispatch(document, internal));
  advance(100);
  check(
    "internal pointerout is not window exit",
    opened("a") && count() == 14,
  );
  reset("a");
  /* Warm-up is optional, parent-owned, and checked against current event time. */
  let cold_group = H.Group.init(~skip_delay_ms=200, ());
  let warm = H.Group.record_close(~time_stamp=1000., cold_group);
  app.dispatch(Group(warm));
  hover(~stamp=1199., "#b", "pointerenter");
  frame();
  check(
    "group skips delay before expiry",
    opened("b") && pending() == 0 && count() == 15,
  );
  reset("b");
  app.dispatch(Group(warm));
  hover(~stamp=1200., "#b", "pointerenter");
  frame();
  check("group expiry is exclusive", !opened("b") && pending() == 1);
  reset("b");
  app.dispatch(Group(warm));
  hover(~stamp=999., "#b", "pointerenter");
  frame();
  check("backwards group stamp stays cold", !opened("b") && pending() == 1);
  reset("b");
  /* Subtracting these doubles rounds the exact deadline back inside the window. */
  let fractional_close = 248.006;
  app.dispatch(
    Group(
      H.Group.record_close(
        ~time_stamp=fractional_close,
        H.Group.init(~skip_delay_ms=1800, ()),
      ),
    ),
  );
  hover(~stamp=fractional_close +. 1800., "#b", "pointerenter");
  frame();
  check(
    "fractional timestamp expiry is exclusive",
    !opened("b") && pending() == 1,
  );
  reset("b");
  app.dispatch(
    Group(
      H.Group.record_close(
        ~time_stamp=1000.,
        H.Group.init(~skip_delay_ms=-1, ()),
      ),
    ),
  );
  hover(~stamp=1000., "#b", "pointerenter");
  frame();
  check("negative skip disables warmup", !opened("b") && pending() == 1);
  reset("b");
  app.dispatch(Group(cold_group));
  hover(~stamp=1199., "#b", "pointerenter");
  frame();
  check(
    "recording a close never mutates another group value",
    !opened("b") && pending() == 1,
  );
  reset("b");
  /* An open member warms the group regardless of elapsed time, so a sibling
     opens before the first member's close delay has finished. */
  let open_a = H.Group.record_open(~key="a", cold_group);
  check(
    "record_open is idempotent",
    H.Group.open_keys(H.Group.record_open(~key="a", open_a)) == ["a"],
  );
  app.dispatch(Group(open_a));
  hover(~stamp=50000., "#b", "pointerenter");
  frame();
  check("open member warms sibling", opened("b") && pending() == 0);
  reset("b");
  let closed_a = H.Group.record_close(~key="a", ~time_stamp=1000., open_a);
  check("keyed close removes member", H.Group.open_keys(closed_a) == []);
  app.dispatch(Group(closed_a));
  hover(~stamp=1300., "#b", "pointerenter");
  frame();
  check(
    "after keyed close the window applies again",
    !opened("b") && pending() == 1,
  );
  reset("b");
  app.dispatch(Group(H.Group.forget(~key="a", open_a)));
  hover(~stamp=50000., "#b", "pointerenter");
  frame();
  check("forgotten member no longer warms", !opened("b") && pending() == 1);
  reset("b");
  app.dispatch(
    Group(
      H.Group.record_open(~key="a", H.Group.init(~skip_delay_ms=0, ())),
    ),
  );
  hover(~stamp=1000., "#b", "pointerenter");
  frame();
  check(
    "zero skip disables warmup even with an open member",
    !opened("b") && pending() == 1,
  );
  reset("b");
  hover("#a", "pointerenter");
  app.dispatch(Remove("a"));
  advance(200);
  check(
    "removing pending trigger cancels timer and listeners",
    pending() == 0 && count() == 16 && listener_count() == baseline,
  );
  hover("#b", "pointerenter");
  advance(100);
  hover("#b", "pointerleave");
  frame();
  let before_shutdown = fired^;
  app.shutdown();
  frame();
  check(
    "shutdown clears pending close and global listeners",
    pending() == 0
    && listener_count() == baseline
    && Tea_dom.Debug.active_listeners() == 0,
  );
  advance(1000);
  check(
    "nothing fires after shutdown",
    fired^ == before_shutdown && count() == 17,
  );
  /* Single/empty consumer collections, overlapping regions and setters that
     invalidate an already-running close. No component owns a collection. */
  let single_init = () => {
    let (model, cmd) = init();
    (
      {
        ...model,
        entries: [List.hd(model.entries)],
      },
      cmd,
    );
  };
  let mount = init =>
    App.element(
      ~init,
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch(
            List.map(
              entry =>
                Sub.map(
                  intent => Intent(entry.key, intent),
                  H.subscriptions(~key=entry.key, entry.hover),
                ),
              model.entries,
            ),
          ),
      ~node="#app",
      ~flags=(),
      (),
    );
  let single = mount(single_init);
  let single_count = () => List.length(single.model().out);
  let single_hover = () => List.hd(single.model().entries).hover;
  check(
    "single model initially has no close time",
    H.last_close_time(single_hover()) == None,
  );
  hover("#a", "pointerenter");
  hover("#a-content", "pointerenter");
  advance(100);
  hover("#a", "pointerleave");
  advance(200);
  check(
    "overlapping regions stay open until both leave",
    opened("a") && pending() == 0 && single_count() == 1,
  );
  let touch_out =
    pointer_event(
      "pointerout",
      {
        "bubbles": true,
        "pointerType": "touch",
        "relatedTarget": Js.Nullable.null,
      },
    );
  ignore(dispatch(document, touch_out));
  advance(100);
  check(
    "touch window exit cannot close pen or mouse hover",
    opened("a") && pending() == 0,
  );
  hover("#a-content", "pointerleave");
  check("overlapping regions share one close", pending() == 1);
  single.dispatch(Set("a", true));
  advance(200);
  check(
    "open_now cancels pending close without intent",
    opened("a") && pending() == 0 && single_count() == 1,
  );
  single.dispatch(Disabled(true));
  frame();
  check(
    "disabling an open trigger silently clears intent",
    !opened("a") && single_count() == 1 && listener_count() == baseline,
  );
  single.dispatch(Disabled(false));
  frame();
  check(
    "reenabling does not synthesize hover",
    !opened("a") && pending() == 0,
  );
  hover("#a", "pointerenter");
  single.dispatch(Set("a", false));
  frame();
  let before_cancelled_start = fired^;
  advance(200);
  check(
    "close before first frame cancels timer and deferred listeners",
    fired^ == before_cancelled_start
    && listener_count() == baseline
    && single_count() == 1,
  );
  hover("#a", "pointerenter");
  frame();
  define(
    document,
    "visibilityState",
    {
      "value": "hidden",
      "configurable": true,
    },
  );
  ignore(
    dispatch(document, event("visibilitychange", { "bubbles": false })),
  );
  advance(200);
  check(
    "hidden while opening emits no false close or warmup timestamp",
    !opened("a")
    && single_count() == 1
    && H.last_close_time(single_hover()) == None
    && pending() == 0,
  );
  define(
    document,
    "visibilityState",
    {
      "value": "visible",
      "configurable": true,
    },
  );
  hover("#a", "pointerenter");
  advance(100);
  ignore(dispatch(window, event("blur", { "bubbles": false })));
  advance(79);
  check(
    "window blur of open instance honors close grace",
    opened("a") && single_count() == 2,
  );
  advance(1);
  check(
    "window blur emits one close",
    !opened("a") && single_count() == 3 && listener_count() == baseline,
  );
  hover("#a", "pointerenter");
  advance(100);
  single.dispatch(Remove("a"));
  frame();
  check(
    "removing open instance leaves an empty collection with no listeners",
    single.model().entries == []
    && pending() == 0
    && listener_count() == baseline
    && Tea_dom.Debug.active_listeners() == 0,
  );
  single.shutdown();
  let opening = mount(single_init);
  hover("#a", "pointerenter");
  check("shutdown fixture is actively opening", pending() == 1);
  let before_opening_shutdown = fired^;
  opening.shutdown();
  advance(200);
  check(
    "shutdown before opening and before first frame disposes everything",
    fired^ == before_opening_shutdown
    && opening.model().out == []
    && pending() == 0
    && listener_count() == baseline
    && Tea_dom.Debug.active_listeners() == 0,
  );
  put(global, "setTimeout", original_timeout);
  put(global, "clearTimeout", original_clear);
  report("hover_intent");
  Js.Promise.resolve();
};
