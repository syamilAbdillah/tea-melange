open Tea;
open Test_support;
module T = Ui.Tooltip;
module G = Ui.Hover_intent.Group;
type timer = {
  id: int,
  at: int,
  callback: unit => unit,
};
let timers = ref([]);
let clock = ref(0);
let next = ref(0);
let schedule = (callback, ms) => {
  incr(next);
  timers :=
    timers^
    @ [
      {
        id: next^,
        at: clock^ + ms,
        callback,
      },
    ];
  next^;
};
let cancel = id => timers := List.filter(timer => timer.id != id, timers^);
let flush_all = () => {
  for (_ in 1 to 12) {
    frame();
  };
};
let advance = ms => {
  clock := clock^ + ms;
  let rec run = () =>
    switch (List.find_opt(t => t.at <= clock^, timers^)) {
    | None => ()
    | Some(t) =>
      cancel(t.id);
      t.callback();
      run();
    };
  run();
  flush_all();
};
[@mel.scope "Object"]
external define: ('a, string, Js.t({..})) => unit = "defineProperty";
let hover = (~kind="mouse", ~stamp=?, selector, name) => {
  let event =
    pointer_event(
      name,
      {
        "bubbles": false,
        "cancelable": true,
        "pointerType": kind,
      },
    );
  Option.iter(
    stamp => define(event, "timeStamp", { "value": stamp }),
    stamp,
  );
  check("hover default unchanged", dispatch(node(selector), event));
};
[@mel.send]
external listen: (Dom.document, string, Dom.event => unit) => unit =
  "addEventListener";
[@mel.send]
external unlisten: (Dom.document, string, Dom.event => unit) => unit =
  "removeEventListener";
let escape_reaches_document = selector => {
  let reached = ref(false);
  let spy = _ => reached := true;
  listen(document, "keydown", spy);
  let accepted = key(selector, "keydown", "Escape");
  unlisten(document, "keydown", spy);
  (accepted, reached^);
};
type entry = {
  key: string,
  tip: T.model,
};
type msg =
  | Tip(string, T.msg)
  | Set(string, bool)
  | Disable(bool)
  | Remove(string)
  | Hook;
type model = {
  entries: list(entry),
  group: G.t,
  out: list(string),
  last_close: float,
  last: option((string, T.msg)),
  hooks: int,
};
let update = (message, model) =>
  switch (message) {
  | Hook => (
      {
        ...model,
        hooks: model.hooks + 1,
      },
      Cmd.none,
    )
  | Remove(key) => (
      {
        ...model,
        entries: List.filter(e => !String.equal(e.key, key), model.entries),
      },
      Cmd.none,
    )
  | _ =>
    let (entries, commands, out, group, last_close, last, hooks) =
      List.fold_right(
        (entry, (entries, commands, out, group, last_close, last, hooks)) => {
          let changed =
            switch (message) {
            | Tip(key, intent) when String.equal(key, entry.key) =>
              let (tip, cmd, outs) =
                T.update(~group=model.group, intent, entry.tip);
              let (out, group, last_close) =
                List.fold_left(
                  ((out, group, last_close), notification) =>
                    switch (notification) {
                    | T.Opened => (out @ [key ++ "+"], group, last_close)
                    | T.Closed(time_stamp) => (
                        out @ [key ++ "-"],
                        G.record_close(~time_stamp, group),
                        time_stamp,
                      )
                    },
                  (out, group, last_close),
                  outs,
                );
              (tip, cmd, out, group, last_close, Some((key, intent)), hooks);
            | Set(key, opened) when String.equal(key, entry.key) =>
              let (tip, cmd) =
                opened ? T.open_(entry.tip) : T.close(entry.tip);
              (tip, cmd, out, group, last_close, last, hooks);
            | Disable(disabled) when String.equal(entry.key, "a") =>
              let (tip, cmd) = T.set_disabled(disabled, entry.tip);
              (tip, cmd, out, group, last_close, last, hooks);
            | _ => (entry.tip, Cmd.none, out, group, last_close, last, hooks)
            };
          let (tip, cmd, out, group, last_close, last, hooks) = changed;
          (
            [
              {
                ...entry,
                tip,
              },
              ...entries,
            ],
            [Cmd.map(x => Tip(entry.key, x), cmd), ...commands],
            out,
            group,
            last_close,
            last,
            hooks,
          );
        },
        model.entries,
        (
          [],
          [],
          model.out,
          model.group,
          model.last_close,
          model.last,
          model.hooks,
        ),
      );
    (
      {
        entries,
        out,
        group,
        last_close,
        last,
        hooks,
      },
      Cmd.batch(commands),
    );
  };
let view = model =>
  Html.div(
    [],
    [
      Html.button(
        [Html.id("outside"), Html.type_("button")],
        [Html.text("Outside")],
      ),
      ...List.map(
           entry =>
             T.view(
               ~model=entry.tip,
               ~to_parent=x => Tip(entry.key, x),
               ~on_key_down=
                 key => String.equal(key, "F2") ? Some(Hook) : None,
               ~to_view=
                 parts => {
                   /* Inspect facts as well as the real DOM: no duplicate hidden/id/data facts
                      may depend on renderer ordering for correctness. */
                   let count = name =>
                     List.fold_left(
                       (n, fact) =>
                         switch (fact) {
                         | Tea_vdom.Vdom.Property(key, _)
                         | Tea_vdom.Vdom.Attribute(key, _)
                             when String.equal(name, key) =>
                           n + 1
                         | _ => n
                         },
                       0,
                       parts.tooltip,
                     );
                   check("one hidden owner", count("hidden") == 1);
                   check("one id owner", count("id") == 1);
                   check("one data-closed owner", count("data-closed") <= 1);
                   check("one data-open owner", count("data-open") <= 1);
                   Html.div(
                     [],
                     [
                       Html.button(
                         parts.trigger @ [Html.type_("button")],
                         [Html.text(entry.key)],
                       ),
                       Html.div(
                         parts.tooltip,
                         [
                           Html.text("Help " ++ entry.key),
                           Html.span(parts.arrow, []),
                         ],
                       ),
                     ],
                   );
                 },
               (),
             ),
           model.entries,
         ),
    ],
  );
let subscriptions = model =>
  Sub.batch(
    List.map(
      e => Sub.map(x => Tip(e.key, x), T.subscriptions(e.tip)),
      model.entries,
    ),
  );
let entry = (app: Tea_core.Runtime.handle(model, msg), key) =>
  List.find(e => String.equal(e.key, key), app.model().entries).tip;
let hidden = (key, expected) =>
  check(
    "hidden " ++ key,
    get(node("#" ++ key ++ "-tooltip"), "hidden") == expected,
  );
let completed = {
  let original_set = get(global, "setTimeout");
  let original_clear = get(global, "clearTimeout");
  put(global, "setTimeout", schedule);
  put(global, "clearTimeout", cancel);
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              entries:
                List.map(
                  key =>
                    {
                      key,
                      tip:
                        T.init(
                          ~id=key,
                          ~mode=
                            String.equal(key, "b") ? T.Label : T.Description,
                          ~open_delay_ms=100,
                          ~close_delay_ms=50,
                          ~arrow_size=8.,
                          (),
                        ),
                    },
                  ["a", "b"],
                ),
              group: G.init(~skip_delay_ms=1800, ()),
              out: [],
              last_close: 0.,
              last: None,
              hooks: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions,
      ~node="#app",
      ~flags=(),
      (),
    );
  flush_all();
  hidden("a", true);
  hidden("b", true);
  check(
    "role retained while hidden",
    attr("#a-tooltip", "role") == Some("tooltip"),
  );
  check(
    "default describedby",
    attr("#a-trigger", "aria-describedby") == Some("a-tooltip")
    && attr("#a-trigger", "aria-labelledby") == None,
  );
  check(
    "label reference",
    attr("#b-trigger", "aria-labelledby") == Some("b-tooltip")
    && attr("#b-trigger", "aria-describedby") == None,
  );
  check(
    "closed Escape propagates",
    escape_reaches_document("#a-trigger") == (true, true),
  );
  check("not opened by hidden escape", !T.is_open(entry(app, "a")));
  focus(node("#a-trigger"));
  check("keyboard focus opens synchronously", T.is_open(entry(app, "a")));
  flush_all();
  hidden("a", false);
  check("focus retained", has_focus("#a-trigger"));
  check("opened event", app.model().out == ["a+"]);
  hover("#a-trigger", "pointerleave");
  advance(1000);
  check(
    "pointer leave cannot dismiss keyboard focus",
    T.is_open(entry(app, "a")),
  );
  List.iter(
    key_ => {
      check(
        "ctrl untouched",
        key(~ctrl=true, "#a-trigger", "keydown", key_),
      );
      check("alt untouched", key(~alt=true, "#a-trigger", "keydown", key_));
      check(
        "meta untouched",
        key(~meta=true, "#a-trigger", "keydown", key_),
      );
      check(
        "composition untouched",
        key(~composing=true, "#a-trigger", "keydown", key_),
      );
    },
    ["Escape", "Enter", " ", "ArrowDown"],
  );
  check("shortcuts do not close", T.is_open(entry(app, "a")));
  check(
    "composed hook keeps default",
    key("#a-trigger", "keydown", "F2") && app.model().hooks == 1,
  );
  check(
    "Tab never prevented",
    key("#a-trigger", "keydown", "Tab")
    && key(~shift=true, "#a-trigger", "keydown", "Tab"),
  );
  check(
    "claimed Escape does not reach document",
    escape_reaches_document("#a-trigger") == (false, false),
  );
  check("Escape leaves focus", has_focus("#a-trigger"));
  check(
    "leave retains Anchor",
    T.phase(entry(app, "a")) == Ui.Animation.Leaving
    && listener_count_for("document", "scroll", true) > 0,
  );
  frame();
  hidden("a", false);
  flush_all();
  hidden("a", true);
  check(
    "closed Anchor stopped",
    listener_count_for("document", "scroll", true) == 0,
  );
  check(
    "next Escape propagates",
    escape_reaches_document("#a-trigger") == (true, true),
  );
  focus(node("#outside"));
  pointer("#a-trigger");
  focus(node("#a-trigger"));
  flush_all();
  check("pointer focus does not open", !T.is_open(entry(app, "a")));
  focus(node("#outside"));
  ignore(key("#outside", "keydown", "Tab"));
  focus(node("#a-trigger"));
  check("keyboard restores modality", T.is_open(entry(app, "a")));
  focus(node("#outside"));
  flush_all();
  hidden("a", true);
  check("blur clears timers", timers^ == []);
  /* Use a cold timestamp to avoid the focus close's warm-up group. */
  hover(~stamp=app.model().last_close +. 2000., "#a-trigger", "pointerenter");
  check(
    "opening timer owned",
    List.length(timers^) == 1 && !T.is_open(entry(app, "a")),
  );
  advance(99);
  hidden("a", true);
  advance(1);
  hidden("a", false);
  check("hover open event", T.is_open(entry(app, "a")));
  let stale = app.model().last;
  hover("#a-trigger", "pointerleave");
  advance(20);
  hover("#a-tooltip", "pointerenter");
  advance(100);
  check(
    "content hover cancels close",
    T.is_open(entry(app, "a")) && timers^ == [],
  );
  advance(100000);
  check("persistent without timeout", T.is_open(entry(app, "a")));
  hover("#a-tooltip", "pointerleave");
  advance(49);
  hidden("a", false);
  advance(1);
  hidden("a", true);
  Option.iter(((key, msg)) => app.dispatch(Tip(key, msg)), stale);
  check("stale timer cannot reopen", !T.is_open(entry(app, "a")));
  hover(~stamp=app.model().last_close +. 1., "#b-trigger", "pointerenter");
  check(
    "group skips subsequent delay",
    T.is_open(entry(app, "b")) && timers^ == [],
  );
  flush_all();
  hidden("b", false);
  check(
    "hover-only Escape from elsewhere claimed",
    escape_reaches_document("#outside") == (false, false),
  );
  flush_all();
  hidden("b", true);
  hover(~stamp=app.model().last_close +. 1800., "#b-trigger", "pointerenter");
  check(
    "warm-up expires at boundary",
    !T.is_open(entry(app, "b")) && List.length(timers^) == 1,
  );
  hover("#b-trigger", "pointerleave");
  check("leaving before open physically cancels timer", timers^ == []);
  advance(1000);
  hidden("b", true);
  hover(~kind="touch", "#a-trigger", "pointerenter");
  advance(1000);
  check(
    "touch hover ignored",
    !T.is_open(entry(app, "a")) && timers^ == [],
  );
  let touch =
    pointer_event(
      "pointerdown",
      {
        "bubbles": true,
        "pointerType": "touch",
      },
    );
  ignore(dispatch(node("#a-trigger"), touch));
  focus(node("#a-trigger"));
  check("touch focus ignored", !T.is_open(entry(app, "a")));
  focus(node("#outside"));
  app.dispatch(Set("a", true));
  flush_all();
  app.dispatch(Disable(true));
  check(
    "disable closes atomically",
    !T.is_open(entry(app, "a")) && T.disabled(entry(app, "a")),
  );
  flush_all();
  hidden("a", true);
  hover("#a-trigger", "pointerenter");
  advance(1000);
  app.dispatch(Set("a", true));
  check(
    "disabled blocks hover and programmatic open",
    !T.is_open(entry(app, "a")) && timers^ == [],
  );
  check(
    "disabled styling only",
    attr("#a-trigger", "data-disabled") == Some("")
    && get(node("#a-trigger"), "disabled") == false,
  );
  app.dispatch(Disable(false));
  flush_all();
  let out = app.model().out;
  for (_ in 1 to 15) {
    app.dispatch(Set("a", true));
    app.dispatch(Set("a", false));
  };
  app.dispatch(Set("a", true));
  flush_all();
  check(
    "batched reversals end open",
    T.is_open(entry(app, "a"))
    && T.phase(entry(app, "a")) == Ui.Animation.Open,
  );
  hidden("a", false);
  check("programmatic setters emit nothing", app.model().out == out);
  app.dispatch(Set("a", false));
  frame();
  hidden("a", false);
  app.dispatch(Set("a", true));
  flush_all();
  hidden("a", false);
  check(
    "reopen reverses leave",
    T.phase(entry(app, "a")) == Ui.Animation.Open,
  );
  app.dispatch(Remove("a"));
  flush_all();
  check(
    "unmount stops positioning",
    listener_count_for("document", "scroll", true) == 0,
  );
  hover(~stamp=app.model().last_close +. 2000., "#b-trigger", "pointerenter");
  check("pending open exists before shutdown", List.length(timers^) == 1);
  app.shutdown();
  flush_all();
  check("shutdown cancels timer", timers^ == []);
  check(
    "shutdown listeners",
    listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
  );
  /* A second mount deliberately leaves a real animation wait unfinished.
     The shared harness accounts MutationObservers; our timer fixture checks
     cancellation immediately on reversal and shutdown, not five seconds later. */
  let waiting =
    App.element(
      ~init=
        () =>
          (
            {
              entries: [
                {
                  key: "wait",
                  tip: T.init(~id="wait", ()),
                },
              ],
              group: G.init(~skip_delay_ms=0, ()),
              out: [],
              last_close: 0.,
              last: None,
              hooks: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions,
      ~node="#app",
      ~flags=(),
      (),
    );
  let unfinished = Js.Promise.make((~resolve as _, ~reject as _) => ());
  put(node("#wait-tooltip"), "getAnimations", () =>
    [|{ "finished": unfinished }|]
  );
  waiting.dispatch(Set("wait", true));
  flush_all();
  check("in-flight animation owns watchdog", List.length(timers^) == 1);
  waiting.dispatch(Set("wait", false));
  check("reversal cancels previous wait synchronously", timers^ == []);
  flush_all();
  check("leaving starts its own wait", List.length(timers^) == 1);
  waiting.dispatch(Set("wait", true));
  check("reopen cancels leaving wait", timers^ == []);
  flush_all();
  check("reopen wait active", List.length(timers^) == 1);
  waiting.shutdown();
  check("shutdown cancels animation watchdog immediately", timers^ == []);
  flush_all();
  check(
    "animation shutdown listeners",
    listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
  );
  put(global, "setTimeout", original_set);
  put(global, "clearTimeout", original_clear);
  report("tooltip");
  Js.Promise.resolve();
};
