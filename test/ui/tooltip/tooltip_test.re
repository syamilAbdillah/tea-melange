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
  | Immediate(string)
  | Disable(bool)
  | Remove(string)
  | Hook;
type model = {
  entries: list(entry),
  group: G.t,
  /* The documented one-visible-at-a-time policy, enabled per fixture. */
  exclusive: bool,
  out: list(string),
  last_close: float,
  last: option((string, T.msg)),
  seen: list((string, T.msg)),
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
        group: G.forget(~key, model.group),
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
                    | T.Opened => (
                        out @ [key ++ "+"],
                        T.track(~key, notification, group),
                        last_close,
                      )
                    | T.Closed(time_stamp) => (
                        out @ [key ++ "-"],
                        T.track(~key, notification, group),
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
              /* Programmatic closes emit nothing: the parent forgets. */
              let group =
                opened && T.is_open(tip)
                  ? G.record_open(~key, group) : G.forget(~key, group);
              (tip, cmd, out, group, last_close, last, hooks);
            | Immediate(key) when String.equal(key, entry.key) =>
              let (tip, cmd) = T.close_immediately(entry.tip);
              (
                tip,
                cmd,
                out,
                G.forget(~key, group),
                last_close,
                last,
                hooks,
              );
            | Disable(disabled) when String.equal(entry.key, "a") =>
              let (tip, cmd) = T.set_disabled(disabled, entry.tip);
              let group = disabled ? G.forget(~key="a", group) : group;
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
    /* A member that just opened takes over: every other member that is still
       visible, even mid-Leaving, closes immediately and leaves the group. */
    let winner =
      switch (message) {
      | Tip(key, _)
          when
            List.length(out) > List.length(model.out)
            && T.is_open(
                 List.find(entry => String.equal(entry.key, key), entries).
                   tip,
               ) =>
        Some(key)
      | _ => None
      };
    let (entries, commands, group) =
      switch (winner) {
      | Some(winner) when model.exclusive =>
        List.fold_right(
          (entry, (entries, commands, group)) =>
            if (!String.equal(entry.key, winner) && T.is_visible(entry.tip)) {
              let (tip, command) = T.close_immediately(entry.tip);
              (
                [
                  {
                    ...entry,
                    tip,
                  },
                  ...entries,
                ],
                [Cmd.map(x => Tip(entry.key, x), command), ...commands],
                G.forget(~key=entry.key, group),
              );
            } else {
              ([entry, ...entries], commands, group);
            },
          entries,
          ([], commands, group),
        )
      | _ => (entries, commands, group)
      };
    (
      {
        ...model,
        entries,
        out,
        group,
        last_close,
        last,
        seen:
          switch (message) {
          | Tip(key, intent) => model.seen @ [(key, intent)]
          | _ => model.seen
          },
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
              exclusive: false,
              out: [],
              last_close: 0.,
              last: None,
              seen: [],
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
  /* Group contract: notifications are folded with [track], programmatic
     closes and unmounts forget, and a member that opens takes over. */
  let warmed = T.track(~key="x", T.Opened, G.init(~skip_delay_ms=1800, ()));
  check("Opened records the member", G.open_keys(warmed) == ["x"]);
  check(
    "Closed removes the member",
    G.open_keys(T.track(~key="x", T.Closed(100.), warmed)) == [],
  );
  let switching =
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
                          ~open_delay_ms=100,
                          ~close_delay_ms=50,
                          (),
                        ),
                    },
                  ["a", "b", "c"],
                ),
              group: G.init(~skip_delay_ms=1800, ()),
              exclusive: true,
              out: [],
              last_close: 0.,
              last: None,
              seen: [],
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
  let member = key => entry(switching, key);
  let outs = () => switching.model().out;
  let open_keys = () => G.open_keys(switching.model().group);
  let after = offset => switching.model().last_close +. offset;
  hover(~stamp=50000., "#a-trigger", "pointerenter");
  check("cold group waits", !T.is_open(member("a")) && timers^ != []);
  advance(100);
  check(
    "open is recorded",
    T.is_open(member("a")) && outs() == ["a+"] && open_keys() == ["a"],
  );
  hover(~stamp=50200., "#a-trigger", "pointerleave");
  check("close delay pending", T.is_open(member("a")) && timers^ != []);
  hover(~stamp=50210., "#b-trigger", "pointerenter");
  check(
    "open sibling warms during the previous close delay",
    T.is_open(member("b")) && timers^ == [],
  );
  check(
    "previous member hidden in the same update",
    !T.is_open(member("a"))
    && !T.is_visible(member("a"))
    && T.phase(member("a")) == Ui.Animation.Closed,
  );
  check(
    "switch is silent for the displaced member",
    outs() == ["a+", "b+"] && open_keys() == ["b"],
  );
  flush_all();
  hidden("a", true);
  hidden("b", false);
  advance(1000);
  check(
    "cancelled close delay cannot fire",
    outs() == ["a+", "b+"] && T.is_open(member("b")),
  );
  check(
    "only the winner is positioned",
    listener_count_for("document", "scroll", true) > 0,
  );
  check(
    "Escape from the open trigger records a close",
    !key("#b-trigger", "keydown", "Escape")
    && outs() == ["a+", "b+", "b-"]
    && open_keys() == [],
  );
  check(
    "closing member is still visible while leaving",
    T.is_visible(member("b"))
    && T.phase(member("b")) == Ui.Animation.Leaving,
  );
  hover(~stamp=after(10.), "#c-trigger", "pointerenter");
  check(
    "skip window opens the next member immediately",
    T.is_open(member("c")) && timers^ == [],
  );
  check(
    "leaving sibling is hidden immediately by the takeover",
    !T.is_visible(member("b"))
    && T.phase(member("b")) == Ui.Animation.Closed
    && open_keys() == ["c"],
  );
  flush_all();
  hidden("b", true);
  hidden("c", false);
  hover("#c-trigger", "pointerleave");
  advance(50);
  check(
    "pointer close records the member close",
    outs() == ["a+", "b+", "b-", "c+", "c-"] && open_keys() == [],
  );
  hover(~stamp=after(1799.), "#a-trigger", "pointerenter");
  check(
    "still warm just inside the skip window",
    T.is_open(member("a")) && timers^ == [],
  );
  hover("#a-trigger", "pointerleave");
  advance(50);
  hover(~stamp=after(1800.), "#b-trigger", "pointerenter");
  check(
    "cold at the skip boundary",
    !T.is_open(member("b")) && List.length(timers^) == 1,
  );
  hover("#b-trigger", "pointerleave");
  check("pending open cancelled", timers^ == []);
  hover(~stamp=after(5000.), "#b-trigger", "pointerenter");
  advance(100);
  check("cold open recorded", open_keys() == ["b"]);
  switching.dispatch(Set("b", false));
  check(
    "programmatic close forgets without a notification",
    open_keys() == [] && List.length(outs()) == 8,
  );
  flush_all();
  hover(~stamp=after(5000.), "#a-trigger", "pointerenter");
  check(
    "forgotten member no longer warms the group",
    !T.is_open(member("a")) && List.length(timers^) == 1,
  );
  hover("#a-trigger", "pointerleave");
  hover(~stamp=after(5000.), "#c-trigger", "pointerenter");
  advance(100);
  check("third member open", open_keys() == ["c"]);
  switching.dispatch(Remove("c"));
  check("unmount forgets", open_keys() == []);
  flush_all();
  hover(~stamp=after(5000.), "#a-trigger", "pointerenter");
  check(
    "unmounted member no longer warms the group",
    !T.is_open(member("a")) && List.length(timers^) == 1,
  );
  advance(100);
  check(
    "disable setup open",
    T.is_open(member("a")) && open_keys() == ["a"],
  );
  switching.dispatch(Disable(true));
  check("disabling forgets", open_keys() == [] && !T.is_open(member("a")));
  flush_all();
  switching.dispatch(Disable(false));
  hover(~stamp=after(5000.), "#b-trigger", "pointerenter");
  advance(100);
  check("immediate close setup", T.is_open(member("b")));
  flush_all();
  switching.dispatch(Immediate("b"));
  check(
    "close_immediately skips the leave",
    !T.is_visible(member("b"))
    && T.phase(member("b")) == Ui.Animation.Closed
    && open_keys() == [],
  );
  flush_all();
  hidden("b", true);
  check(
    "close_immediately stops positioning and leaves no timers",
    listener_count_for("document", "scroll", true) == 0 && timers^ == [],
  );
  check(
    "closed Escape propagates after an immediate close",
    escape_reaches_document("#b-trigger") == (true, true),
  );
  switching.dispatch(Set("b", true));
  let before_progress = List.length(switching.model().seen);
  flush_all();
  let obsolete_progress =
    List.filteri((i, _) => i >= before_progress, switching.model().seen);
  check(
    "captured asynchronous animation/anchor progress",
    obsolete_progress != [],
  );
  switching.dispatch(Immediate("b"));
  switching.dispatch(Set("b", true));
  List.iter(
    ((key, intent)) => switching.dispatch(Tip(key, intent)),
    obsolete_progress,
  );
  check(
    "old progress cannot finish the reopened animation generation",
    T.phase(member("b")) == Ui.Animation.Entering,
  );
  flush_all();
  hidden("b", false);
  check("reopens after an immediate close", T.is_open(member("b")));
  hover("#b-trigger", "pointerenter");
  hover("#b-trigger", "pointerleave");
  advance(50);
  flush_all();
  hidden("b", true);
  switching.shutdown();
  flush_all();
  check(
    "switching shutdown leaves nothing behind",
    timers^ == []
    && listener_count() == 0
    && Tea_dom.Debug.active_listeners() == 0,
  );
  /* Leave a real animation wait unfinished. The shared harness accounts
     MutationObservers; our timer fixture checks cancellation immediately on
     reversal, immediate close and shutdown, not five seconds later. */
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
              exclusive: false,
              out: [],
              last_close: 0.,
              last: None,
              seen: [],
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
  waiting.dispatch(Immediate("wait"));
  check(
    "immediate close cancels entering wait synchronously",
    timers^ == [] && T.phase(entry(waiting, "wait")) == Ui.Animation.Closed,
  );
  flush_all();
  hidden("wait", true);
  check(
    "immediate close does not start a leaving wait or retain Anchor",
    timers^ == [] && listener_count_for("document", "scroll", true) == 0,
  );
  waiting.dispatch(Set("wait", true));
  flush_all();
  waiting.dispatch(Set("wait", false));
  flush_all();
  check("another leaving wait active", List.length(timers^) == 1);
  waiting.dispatch(Immediate("wait"));
  check("immediate close cancels leaving wait synchronously", timers^ == []);
  flush_all();
  hidden("wait", true);
  waiting.dispatch(Set("wait", true));
  flush_all();
  check(
    "reopening after skipped leave still waits",
    List.length(timers^) == 1,
  );
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
