open Tea;
open Test_support;
module T = Ui.Toast;
/* Fake only the clock, not component/runtime behavior. Check physical timer
   disposal independently, and restore globals before the shared leak gate. */
type timer = {
  id: int,
  at: float,
  callback: unit => unit,
};
let timers = ref([]);
let time = ref(0.);
let serial = ref(0);
let schedule = (callback, ms) => {
  incr(serial);
  timers :=
    timers^
    @ [
      {
        id: serial^,
        at: time^ +. float_of_int(ms),
        callback,
      },
    ];
  serial^;
};
let cancel = id => timers := List.filter(t => t.id != id, timers^);
let pending = () => List.length(timers^);
let frames = () =>
  for (_i in 1 to 14) {
    flush();
  };
let advance = ms => {
  let target = time^ +. float_of_int(ms);
  let rec run = () =>
    switch (List.find_opt(t => t.at <= target, timers^)) {
    | None => time := target
    | Some(t) =>
      time := t.at;
      cancel(t.id);
      t.callback();
      run();
    };
  run();
  frames();
};
[@mel.scope "Object"]
external define: ('a, string, Js.t({..})) => unit = "defineProperty";
[@mel.send]
external listen: (Dom.node, string, Dom.event => unit) => unit =
  "addEventListener";
[@mel.send]
external unlisten: (Dom.node, string, Dom.event => unit) => unit =
  "removeEventListener";
external resources:
  unit =>
  {
    .
    "timers": int,
    "observers": int,
  } =
  "teaResourceProbe";
let performance = get(global, "performance");
let original_now = get(performance, "now");
let original_timeout = get(global, "setTimeout");
let original_clear = get(global, "clearTimeout");
let install_clock = () => {
  put(global, "setTimeout", schedule);
  put(global, "clearTimeout", cancel);
  define(
    performance,
    "now",
    {
      "configurable": true,
      "writable": true,
      "value": () => time^,
    },
  );
};
let restore_clock = () => {
  put(global, "setTimeout", original_timeout);
  put(global, "clearTimeout", original_clear);
  define(
    performance,
    "now",
    {
      "configurable": true,
      "writable": true,
      "value": original_now,
    },
  );
};
type msg =
  | Intent(T.msg)
  | Push(string, string, T.urgency, option(int), bool)
  | Dismiss(string)
  | Clear
  | Hook
  | Remove;
type model = {
  toast: T.model,
  events: list(T.out_msg),
  mounted: bool,
  hooks: int,
  last: option(T.msg),
};
let render = toast =>
  T.view(
    ~model=toast,
    ~to_parent=message => Intent(message),
    ~on_key_down=key => String.equal(key, "Home") ? Some(Hook) : None,
    ~to_view=
      parts => {
        let lane = urgency =>
          List.filter_map(
            (p: T.toast_parts(msg)) =>
              p.urgency != urgency
                ? None
                : Some((
                    p.key,
                    Html.div(
                      p.root @ [Html.data("key", p.key)],
                      [
                        Html.h2(p.title, [Html.text(p.content_key)]),
                        Html.p(
                          p.description,
                          [Html.text("Description " ++ p.content_key)],
                        ),
                        Html.button(
                          p.action_attrs(
                            ~key="undo",
                            ~alt_text="Use the history page instead.",
                            (),
                          ),
                          [Html.text("Undo")],
                        ),
                        Html.button(
                          p.action_attrs(~key="blank", ~alt_text="  ", ()),
                          [Html.text("Invalid action")],
                        ),
                        Html.button(p.close_button, [Html.text("Dismiss")]),
                      ],
                    ),
                  )),
            parts.toasts,
          );
        Html.div(
          parts.region,
          [
            Html.Keyed.div(parts.polite, lane(T.Normal)),
            Html.Keyed.div(parts.assertive, lane(T.Urgent)),
          ],
        );
      },
    (),
  );
let start =
    (
      ~initial=false,
      ~max_visible=3,
      ~order=T.Oldest_first,
      ~hotkey={
                T.key: "F8",
                ctrl: false,
                alt: false,
                meta: false,
                shift: false,
              },
      (),
    ) =>
  App.element(
    ~init=
      () => {
        let toast =
          T.init(
            ~id="notice",
            ~label="Updates",
            ~max_visible,
            ~order,
            ~hotkey,
            (),
          );
        let (toast, command) =
          initial
            ? T.push(
                toast,
                ~key="initial",
                ~urgency=T.Normal,
                "First announcement",
              )
            : (toast, Cmd.none);
        (
          {
            toast,
            events: [],
            mounted: true,
            hooks: 0,
            last: None,
          },
          Cmd.map(message => Intent(message), command),
        );
      },
    ~update=
      (message, model) => {
        let (toast, command, events, mounted, last) =
          switch (message) {
          | Intent(message) =>
            let (toast, command, events) = T.update(message, model.toast);
            (toast, command, events, model.mounted, Some(message));
          | Push(key, content, urgency, duration, has_actions) =>
            let (toast, command) =
              T.push(
                model.toast,
                ~key,
                ~duration,
                ~urgency,
                ~has_actions,
                content,
              );
            (toast, command, [], model.mounted, model.last);
          | Dismiss(key) =>
            let (toast, command) = T.dismiss(model.toast, key);
            (toast, command, [], model.mounted, model.last);
          | Clear =>
            let (toast, command) = T.dismiss_all(model.toast);
            (toast, command, [], model.mounted, model.last);
          | Hook => (model.toast, Cmd.none, [], model.mounted, model.last)
          | Remove => (model.toast, Cmd.none, [], false, model.last)
          };
        (
          {
            toast,
            events: model.events @ events,
            mounted,
            hooks:
              model.hooks
              + (
                switch (message) {
                | Hook => 1
                | _ => 0
                }
              ),
            last,
          },
          Cmd.map(message => Intent(message), command),
        );
      },
    ~view=
      model =>
        Html.div(
          [Html.id("ancestor")],
          [
            Html.button([Html.id("opener")], [Html.text("Origin")]),
            model.mounted ? render(model.toast) : Html.text(""),
            Html.button([Html.id("after")], [Html.text("After")]),
          ],
        ),
    ~subscriptions=
      model =>
        model.mounted
          ? Sub.map(
              message => Intent(message),
              T.subscriptions(model.toast),
            )
          : Sub.none,
    ~node="#app",
    ~flags=(),
    (),
  );
let root = key => "#notice-item-" ++ key;
let close_button = key => root(key) ++ "-close";
let action = key => root(key) ++ "-action-undo";
let count = () => Array.length(query_all(document, "[data-key]"));
let has_event = (app: Tea_core.Runtime.handle(model, msg), key, reason) =>
  List.exists(
    event => event == T.Dismissed(key, reason),
    app.model().events,
  );
let push =
    (
      app: Tea_core.Runtime.handle(model, msg),
      key,
      ~duration=Some(5000),
      ~urgency=T.Normal,
      ~has_actions=false,
      content,
    ) => {
  app.dispatch(Push(key, content, urgency, duration, has_actions));
  frames();
};
let hover = (~kind="mouse", name) => {
  ignore(
    dispatch(
      node("#notice"),
      pointer_event(
        name,
        {
          "bubbles": false,
          "pointerType": kind,
        },
      ),
    ),
  );
  frames();
};
let visibility = hidden => {
  define(
    document,
    "visibilityState",
    {
      "configurable": true,
      "value": hidden ? "hidden" : "visible",
    },
  );
  define(
    document,
    "hidden",
    {
      "configurable": true,
      "value": hidden,
    },
  );
  ignore(
    dispatch(document, event("visibilitychange", { "bubbles": false })),
  );
  frames();
};
let stop = (app: Tea_core.Runtime.handle(model, msg)) => {
  app.shutdown();
  frames();
  check("timers physically cancelled", pending() == 0);
  check("no managed listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  check("no animation observers", resources()##observers == 0);
};
let completed = {
  install_clock();
  visibility(false);
  let app = start(~initial=true, ());
  check(
    "persistent region exists before first entry",
    count() == 0
    && attr("#notice", "role") == Some("region")
    && attr("#notice", "aria-label") == Some("Updates"),
  );
  check(
    "empty live regions preexist",
    attr("#notice-polite", "role") == Some("status")
    && attr("#notice-assertive", "role") == Some("alert"),
  );
  frames();
  check(
    "initial toast inserted after mount",
    count() == 1 && T.keys(app.model().toast) == ["initial"],
  );
  check(
    "initial timer starts after entered: pending="
    ++ string_of_int(pending())
    ++ " entering="
    ++ Option.value(~default="absent", attr(root("initial"), "data-enter")),
    pending() == 1 && attr(root("initial"), "data-enter") == None,
  );
  check(
    "idrefs resolve",
    attr(root("initial"), "aria-labelledby")
    == Some("notice-item-initial-title")
    && attr("#notice-item-initial-title", "id")
    == Some("notice-item-initial-title")
    && attr(root("initial"), "aria-describedby")
    == Some("notice-item-initial-description"),
  );
  check(
    "close has accessible name",
    attr(close_button("initial"), "aria-label")
    == Some("Dismiss notification"),
  );
  check(
    "finite action disabled",
    get(node(action("initial")), "disabled"),
  );
  check(
    "blank alternative disabled",
    get(node("#notice-item-initial-action-blank"), "disabled"),
  );
  advance(2000);
  hover("pointerenter");
  check(
    "hover pauses all timers",
    pending() == 0
    && T.paused(app.model().toast)
    && attr(root("initial"), "data-paused") == Some(""),
  );
  advance(20000);
  check("hover retains toast", count() == 1);
  focus(node(close_button("initial")));
  frames();
  hover("pointerleave");
  check("focus keeps pause after hover ends", pending() == 0);
  advance(10000);
  focus(node("#opener"));
  frames();
  check("resume timer", pending() == 1 && !T.paused(app.model().toast));
  advance(2999);
  check("remaining not reset or shortened", count() == 1);
  advance(1);
  check(
    "timeout completed through Left",
    count() == 0 && has_event(app, "initial", T.Timeout),
  );
  check("one dismissal", List.length(app.model().events) == 1);
  push(app, "hidden", "Hidden test");
  advance(1200);
  visibility(true);
  advance(10000);
  check("visibility pauses", pending() == 0 && count() == 1);
  visibility(false);
  advance(3799);
  check("hidden remaining time", count() == 1);
  advance(1);
  check(
    "visibility resumed exact remaining",
    has_event(app, "hidden", T.Timeout),
  );
  push(app, "minimum", ~duration=Some(-1), "Minimum");
  hover(~kind="touch", "pointerenter");
  check("touch is not hover", !T.paused(app.model().toast));
  advance(4999);
  check("minimum five seconds", count() == 1);
  advance(1);
  check("minimum expires", has_event(app, "minimum", T.Timeout));
  push(app, "a", ~duration=None, "A");
  push(app, "b", ~duration=None, "B");
  push(app, "c", ~duration=None, ~urgency=T.Urgent, "C");
  push(app, "d", ~duration=None, "D");
  push(app, "e", ~duration=None, "E");
  check(
    "queue limit",
    count() == 3 && T.queued_keys(app.model().toast) == ["d", "e"],
  );
  check(
    "urgency is correct lane",
    Array.length(query_all(document, "#notice-assertive [data-key]")) == 1
    && attr(root("c"), "data-urgency") == Some("urgent"),
  );
  check(
    "stacking data",
    attr(root("b"), "data-index") == Some("1")
    && get(get(node(root("b")), "style"), "cssText")
    == "--tea-toast-index: 1;",
  );
  let before = node(root("a"));
  push(app, "a", ~duration=None, ~urgency=T.Urgent, "Updated A");
  check(
    "duplicate updates same node and priority",
    node(root("a")) === before
    && attr(root("a"), "data-urgency") == Some("normal")
    && T.keys(app.model().toast) == ["a", "b", "c"],
  );
  check(
    "updated text is inside existing polite region",
    get(node("#notice-item-a-title"), "textContent") == "Updated A",
  );
  app.dispatch(Dismiss("d"));
  check(
    "queued cancellation is asynchronous",
    !has_event(app, "d", T.Programmatic),
  );
  frames();
  check(
    "queued cancellation leaves without mounting",
    has_event(app, "d", T.Programmatic)
    && T.queued_keys(app.model().toast) == ["e"]
    && count() == 3,
  );
  ignore(
    dispatch(
      node(close_button("a")),
      event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
        },
      ),
    ),
  );
  check(
    "leave retains slot",
    count() == 3 && !has_event(app, "a", T.Close_button),
  );
  frames();
  check(
    "FIFO promotion only after Left",
    has_event(app, "a", T.Close_button)
    && T.keys(app.model().toast) == ["b", "c", "e"],
  );
  app.dispatch(Clear);
  frames();
  check(
    "dismiss all",
    count() == 0 && T.queued_keys(app.model().toast) == [],
  );
  app.dispatch(Dismiss("absent"));
  frames();
  push(app, "action", ~has_actions=true, "Persistent action");
  check(
    "actions force persistence",
    pending() == 0 && !get(node(action("action")), "disabled"),
  );
  advance(20000);
  check("action does not expire", count() == 1);
  ignore(
    dispatch(
      node(action("action")),
      event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
        },
      ),
    ),
  );
  check(
    "Action immediate before Dismissed",
    List.exists(out => out == T.Action("action"), app.model().events)
    && !has_event(app, "action", T.Action),
  );
  frames();
  check("action dismissal after Left", has_event(app, "action", T.Action));
  push(app, "focus", ~duration=None, "Focused notification");
  let ancestor = node("#ancestor");
  let propagated = ref(0);
  let listener = _ => incr(propagated);
  listen(ancestor, "keydown", listener);
  focus(node("#opener"));
  check("F8 consumed", !key("#opener", "keydown", "F8"));
  frames();
  check("F8 focuses newest", has_focus(root("focus")) && propagated^ == 0);
  List.iter(
    k => {
      check(
        "unclaimed chord propagates",
        key(~ctrl=true, root("focus"), "keydown", k),
      );
      check(
        "IME propagates",
        key(~composing=true, root("focus"), "keydown", k),
      );
    },
    ["Escape", "F8", "ArrowDown"],
  );
  check("all six unclaimed events reached ancestor", propagated^ == 6);
  check("Tab not prevented", key(root("focus"), "keydown", "Tab"));
  check("Tab propagates", propagated^ == 7);
  check("Escape consumed", !key(root("focus"), "keydown", "Escape"));
  check("Escape never reaches enclosing overlay", propagated^ == 7);
  frames();
  check(
    "Escape dismisses and returns",
    has_event(app, "focus", T.Escape) && has_focus("#opener"),
  );
  check("empty F8 consumed", !key("#opener", "keydown", "F8"));
  frames();
  check("empty region focuses", has_focus("#notice"));
  check("empty Escape consumed", !key("#notice", "keydown", "Escape"));
  frames();
  check("empty Escape returns", has_focus("#opener"));
  push(app, "tab", ~duration=None, "Tab exit");
  ignore(key("#opener", "keydown", "F8"));
  frames();
  ignore(key(root("tab"), "keydown", "Tab"));
  /* jsdom cannot traverse Tab: exercise the boundary focus event separately. */
  focus(node("#after"));
  frames();
  check("Tab-out returns hotkey origin", has_focus("#opener"));
  ignore(key("#opener", "keydown", "F8"));
  frames();
  focus(node("#after"));
  frames();
  check("pointer/programmatic departure not stolen", has_focus("#after"));
  focus(node(close_button("tab")));
  frames();
  click(close_button("tab"));
  frames();
  check("last ordinary close repairs focus to region", has_focus("#notice"));
  check("hook preserves default", key("#notice", "keydown", "Home"));
  check("hook receives unhandled key", app.model().hooks == 1);
  ignore(key(~ctrl=true, "#notice", "keydown", "Home"));
  ignore(key(~composing=true, "#notice", "keydown", "Home"));
  check("hook excludes shortcut and IME", app.model().hooks == 1);
  unlisten(ancestor, "keydown", listener);
  stop(app);
  let app =
    start(
      ~max_visible=2,
      ~order=T.Newest_first,
      ~hotkey={
        key: "F9",
        ctrl: true,
        shift: true,
        alt: false,
        meta: false,
      },
      (),
    );
  frames();
  push(app, "one", ~duration=None, "One");
  push(app, "two", ~duration=None, "Two");
  push(app, "three", ~duration=None, "Three");
  check(
    "newest first",
    T.keys(app.model().toast) == ["two", "one"]
    && T.queued_keys(app.model().toast) == ["three"],
  );
  focus(node("#opener"));
  check("default hotkey no longer claimed", key("#opener", "keydown", "F8"));
  check(
    "custom exact chord",
    !key(~ctrl=true, ~shift=true, "#opener", "keydown", "F9"),
  );
  frames();
  check("custom hotkey most recent", has_focus(root("two")));
  check(
    "repeat ignored",
    key(~ctrl=true, ~shift=true, ~repeat=true, root("two"), "keydown", "F9"),
  );
  app.dispatch(Clear);
  frames();
  push(app, "reversal", "First");
  app.dispatch(Dismiss("reversal"));
  app.dispatch(Push("reversal", "Reopened", T.Normal, Some(5000), false));
  frames();
  check(
    "duplicate reverses leave without dismissal",
    count() == 1 && !has_event(app, "reversal", T.Programmatic),
  );
  advance(1000);
  push(app, "reversal", "Reset timer");
  advance(4999);
  check("duplicate resets duration", count() == 1);
  advance(1);
  check("replacement timeout", has_event(app, "reversal", T.Timeout));
  push(app, "timer", "Unmount while timer active");
  check("active timer before removal", pending() == 1);
  app.dispatch(Remove);
  frames();
  check("unmount cancels timer", pending() == 0 && listener_count() == 0);
  advance(20000);
  check(
    "unmounted timer never dismisses",
    !has_event(app, "timer", T.Timeout),
  );
  stop(app);
  let app = start(~max_visible=0, ());
  frames();
  push(app, "active", "Shutdown active clock");
  push(app, "queued", "Queued");
  check(
    "max_visible clamp",
    count() == 1 && T.queued_keys(app.model().toast) == ["queued"],
  );
  stop(app);
  let app = start();
  frames();
  push(app, "token", "Old timer");
  let expired_callback = List.hd(timers^).callback;
  expired_callback();
  let stale = Option.get(app.model().last);
  app.dispatch(Push("token", "Fresh timer", T.Normal, Some(5000), false));
  app.dispatch(Intent(stale));
  frames();
  check(
    "stale elapsed generation cannot dismiss replacement",
    count() == 1 && !has_event(app, "token", T.Timeout),
  );
  app.dispatch(Dismiss("token"));
  frames();
  push(app, "token", ~duration=None, "Reused key");
  app.dispatch(Intent(stale));
  frames();
  check("stale elapsed instance cannot dismiss reused key", count() == 1);
  app.dispatch(Clear);
  frames();
  push(app, "a b", ~duration=None, "Space key");
  push(app, "a_0020b", ~duration=None, "Escape-looking key");
  check(
    "keys sanitize injectively",
    attr("#notice-item-a_0020b", "id") == Some("notice-item-a_0020b")
    && attr("#notice-item-a_005f0020b", "id")
    == Some("notice-item-a_005f0020b"),
  );
  stop(app);
  /* Every timer pauses, including entries promoted while already hovered. */
  let app = start(~max_visible=2, ());
  frames();
  push(app, "one", "First clock");
  push(app, "two", "Second clock");
  push(app, "three", "Queued clock");
  check("independent timer per visible toast", pending() == 2);
  advance(1000);
  hover("pointerenter");
  check("hover cancels both clocks", pending() == 0);
  app.dispatch(Dismiss("one"));
  frames();
  check(
    "promotion while paused starts no clock",
    T.keys(app.model().toast) == ["two", "three"] && pending() == 0,
  );
  advance(10000);
  hover("pointerleave");
  check("resume starts independent remaining clocks", pending() == 2);
  advance(4000);
  check(
    "older timer retains remainder",
    has_event(app, "two", T.Timeout) && !has_event(app, "three", T.Timeout),
  );
  advance(1000);
  check(
    "promoted timer gets full duration",
    has_event(app, "three", T.Timeout),
  );
  stop(app);
  /* Cancellation releases Animation's pending watchdog/observer immediately. */
  let app = start();
  frames();
  push(app, "animation", ~duration=None, "Animated");
  let forever = Js.Promise.make((~resolve as _, ~reject as _) => ());
  put(node(root("animation")), "getAnimations", _ =>
    [|{ "finished": forever }|]
  );
  app.dispatch(Dismiss("animation"));
  frames();
  check(
    "leaving animation awaited",
    count() == 1
    && !has_event(app, "animation", T.Programmatic)
    &&
    resources()##observers == 1
    && pending() == 1,
  );
  stop(app);
  visibility(false);
  restore_clock();
  check(
    "tracked resource count",
    resources()##timers == 0 && resources()##observers == 0,
  );
  report("toast");
  Js.Promise.resolve();
};
