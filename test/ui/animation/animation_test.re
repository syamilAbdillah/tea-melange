open Tea;
open Test_support;
module A = Ui.Animation;
type msg =
  | Show
  | Hide
  | Toggle
  | Progress(A.msg)
  | Remove;
type model = {
  animation: A.model,
  events: list(A.out_msg),
  desired: bool,
  removed: bool,
};
let start = (~open_=false, ~appear=false, ~unmount=false, ()) =>
  App.element(
    ~init=
      () =>
        (
          {
            animation: A.init(~id="panel", ~open_, ~appear, ()),
            events: [],
            desired: open_,
            removed: false,
          },
          Cmd.none,
        ),
    ~update=
      (message, model) => {
        let (animation, command, events, desired, removed) =
          switch (message) {
          | Show
          | Hide
          | Toggle =>
            let desired =
              switch (message) {
              | Show => true
              | Hide => false
              | _ => !model.desired
              };
            let (animation, command) =
              desired ? A.show(model.animation) : A.hide(model.animation);
            (animation, command, [], desired, model.removed);
          | Progress(message) =>
            let (animation, command, events) =
              A.update(message, model.animation);
            (animation, command, events, model.desired, model.removed);
          | Remove => (model.animation, Cmd.none, [], model.desired, true)
          };
        (
          {
            animation,
            events: model.events @ events,
            desired,
            removed,
          },
          Cmd.map(message => Progress(message), command),
        );
      },
    ~view=
      model =>
        Html.div(
          [],
          [
            Ui.Button.button(
              ~id="toggle",
              ~on_press=Toggle,
              [Html.text("Toggle")],
            ),
            !model.removed && (!unmount || A.should_render(model.animation))
              ? Html.div(
                  A.attrs(model.animation)
                  @ [
                    Html.id("panel"),
                    Html.role("region"),
                    Html.aria("label", "Animated content"),
                  ],
                  [
                    Ui.Button.button(
                      ~id="inside",
                      ~on_press=Hide,
                      [Html.text("Close")],
                    ),
                  ],
                )
              : Html.text(""),
          ],
        ),
    ~subscriptions=
      model =>
        Sub.map(
          message => Progress(message),
          A.subscriptions(model.animation),
        ),
    ~node="#app",
    ~flags=(),
    (),
  );
let settle_frames = () => {
  for (_i in 1 to 8) {
    flush();
  };
};
let flag = name => attr("#panel", "data-" ++ name) == Some("");
let hidden = () => get(node("#panel"), "hidden");
let pending = () => {
  let resolve_ = ref(() => ());
  let reject_ = ref(() => ());
  let promise =
    Js.Promise.make((~resolve, ~reject) => {
      resolve_ := (() => resolve(. true));
      reject_ := (() => reject(. Failure("cancelled")));
    });
  ({ "finished": promise }, () => resolve_^(), () => reject_^());
};
let stop = (app: Tea_core.Runtime.handle(model, msg)) => {
  app.shutdown();
  settle_frames();
  check("no element listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
};
/* Exercise opaque messages through the public subscription API, not a test-only
   constructor. Deliberately retain/replay them after the generation changes. */
let capture = animation => {
  let messages = ref([]);
  let dispose =
    switch (A.subscriptions(animation)) {
    | Sub.Leaf(leaf) =>
      leaf.start(raw => messages := messages^ @ [leaf.tagger(raw)])
    | _ => failwith("expected active animation subscription")
    };
  (messages, dispose);
};
let one = messages =>
  switch (messages^) {
  | [message] => message
  | _ => failwith("expected one progress message")
  };
let pure = () => {
  let closed = A.init(~id="absent", ~open_=false, ~appear=true, ());
  check(
    "initial closed ignores appear",
    A.phase(closed) == A.Closed
    && !A.is_visible(closed)
    && !A.should_render(closed),
  );
  let (same, command) = A.hide(closed);
  check("hide closed no-op", same === closed && command == Cmd.None);
  let open_ = A.init(~id="absent", ~open_=true, ());
  let (same, command) = A.show(open_);
  check("show open no-op", same === open_ && command == Cmd.None);
  let (enter, _) = A.show(closed);
  let (same, command) = A.show(enter);
  check("show entering idempotent", same === enter && command == Cmd.None);
  let (painted, dispose_paint) = capture(enter);
  flush();
  check("one rAF does not dispatch", painted^ == []);
  flush();
  let old_paint = one(painted);
  dispose_paint();
  let (leave, _) = A.hide(enter);
  let (same, command) = A.hide(leave);
  check("hide leaving idempotent", same === leave && command == Cmd.None);
  let (finished, dispose_wait) = capture(leave);
  frame();
  let old_finish = one(finished);
  dispose_wait();
  let (new_enter, _) = A.show(leave);
  List.iter(
    message => {
      let (same, command, outs) = A.update(message, new_enter);
      check(
        "old generation never changes new enter",
        same === new_enter && command == Cmd.None && outs == [],
      );
    },
    [old_paint, old_finish],
  );
  let (paints, dispose) = capture(new_enter);
  frame();
  dispose();
  let paint = one(paints);
  let (waiting, _, outs) = A.update(paint, new_enter);
  check(
    "paint advances without completion",
    A.phase(waiting) == A.Entering && outs == [],
  );
  let (same, _, outs) = A.update(paint, waiting);
  check("duplicate paint no-op", same === waiting && outs == []);
  let (finishes, dispose) = capture(waiting);
  frame();
  dispose();
  let finish = one(finishes);
  let (opened, _, outs) = A.update(finish, waiting);
  check(
    "only current enter completes",
    A.phase(opened) == A.Open && outs == [A.Entered],
  );
  let (same, _, outs) = A.update(finish, opened);
  check("duplicate completion never re-emits", same === opened && outs == []);
  let (same, _, outs) = A.update(old_finish, opened);
  check("stale leave cannot close open", same === opened && outs == []);
  let (leave, _) = A.hide(opened);
  let (messages, dispose) = capture(leave);
  frame();
  dispose();
  let (closed, _, outs) = A.update(one(messages), leave);
  check(
    "leave completes missing element",
    A.phase(closed) == A.Closed && outs == [A.Left],
  );
  let (same, _, outs) = A.update(one(messages), closed);
  check("duplicate Left ignored", same === closed && outs == []);
  /* Carry across multiple decimal digits; every retained message stays stale. */
  let final = ref(closed);
  for (_i in 1 to 150) {
    let (enter, _) = A.show(final^);
    let (leave, _) = A.hide(enter);
    final := leave;
  };
  let (same, _, outs) = A.update(old_finish, final^);
  check("many generations remain distinct", same === final^ && outs == []);
  let (messages, dispose) =
    capture(A.init(~id="absent", ~open_=true, ~appear=true, ()));
  dispose();
  settle_frames();
  check("stop before first frame suppresses appear", messages^ == []);
  let (messages, dispose) =
    capture(A.init(~id="absent", ~open_=true, ~appear=true, ()));
  flush();
  dispose();
  settle_frames();
  check("stop between frame callbacks suppresses appear", messages^ == []);
};
let synchronous = () => {
  pure();
  let app = start();
  check(
    "closed is hidden without CSS",
    hidden()
    && flag("closed")
    && !flag("transition")
    && !flag("enter")
    && !flag("leave"),
  );
  check(
    "attrs own no role/id overrides",
    attr("#panel", "role") == Some("region")
    && attr("#panel", "aria-label") == Some("Animated content"),
  );
  app.dispatch(Show);
  check(
    "enter visible immediately in model",
    A.is_visible(app.model().animation),
  );
  flush();
  check(
    "first drawn enter frame",
    flag("closed")
    && flag("enter")
    && flag("transition")
    && !flag("leave")
    && !hidden(),
  );
  flush();
  check("initial attributes survive a paint boundary", flag("closed"));
  flush();
  check(
    "next draw drops closed",
    !flag("closed") && flag("enter") && flag("transition"),
  );
  settle_frames();
  check(
    "no CSS reaches Open",
    A.phase(app.model().animation) == A.Open
    && app.model().events == [A.Entered],
  );
  check(
    "open clears transition flags",
    !hidden()
    && !flag("enter")
    && !flag("leave")
    && !flag("transition")
    && !flag("closed"),
  );
  focus(node("#inside"));
  List.iter(
    key_ =>
      check("animation never owns keys", key("#inside", "keydown", key_)),
    ["Tab", "Enter", " ", "Escape", "ArrowDown"],
  );
  check("animation never moves focus", has_focus("#inside"));
  app.dispatch(Hide);
  settle_frames();
  check(
    "no CSS reaches Closed",
    A.phase(app.model().animation) == A.Closed
    && app.model().events == [A.Entered, A.Left]
    && hidden(),
  );
  /* Actual pointer/click intent events share one frame; latest model wins. */
  for (_i in 1 to 101) {
    ignore(
      dispatch(
        node("#toggle"),
        event(
          "click",
          {
            "bubbles": true,
            "cancelable": true,
          },
        ),
      ),
    );
  };
  settle_frames();
  check(
    "odd rapid toggles end open only once",
    A.phase(app.model().animation) == A.Open
    && app.model().events == [A.Entered, A.Left, A.Entered],
  );
  for (_i in 1 to 100) {
    app.dispatch(Toggle);
  };
  settle_frames();
  check(
    "even rapid reversals end open",
    A.phase(app.model().animation) == A.Open
    && app.model().events == [A.Entered, A.Left, A.Entered, A.Entered],
  );
  stop(app);
  for (frames_before_reverse in 0 to 3) {
    let app = start();
    app.dispatch(Show);
    for (_i in 1 to frames_before_reverse) {
      flush();
    };
    app.dispatch(Hide);
    settle_frames();
    check(
      "hide at every enter handoff ends closed",
      A.phase(app.model().animation) == A.Closed
      && app.model().events == [A.Left]
      && hidden(),
    );
    stop(app);
    let app = start(~open_=true, ());
    app.dispatch(Hide);
    app.dispatch(Show);
    settle_frames();
    check(
      "show before leave draw reverses cleanly",
      A.phase(app.model().animation) == A.Open
      && app.model().events == [A.Entered],
    );
    stop(app);
  };
  let app = start(~open_=true, ());
  settle_frames();
  check(
    "initial open skips effects",
    A.phase(app.model().animation) == A.Open
    && app.model().events == []
    && !hidden(),
  );
  stop(app);
  let app = start(~open_=true, ~appear=true, ());
  check(
    "appear starts with initial enter attrs",
    flag("closed") && flag("enter") && !hidden(),
  );
  settle_frames();
  check("appear emits Entered", app.model().events == [A.Entered]);
  stop(app);
  let app = start(~unmount=true, ());
  check("closed may be unmounted", Js.testAny(node("#panel")));
  app.dispatch(Show);
  settle_frames();
  check("show remounts and opens", A.phase(app.model().animation) == A.Open);
  app.dispatch(Hide);
  settle_frames();
  check(
    "Left permits unmount",
    app.model().events == [A.Entered, A.Left] && Js.testAny(node("#panel")),
  );
  stop(app);
  let app = start();
  app.dispatch(Show);
  app.shutdown();
  settle_frames();
  check("shutdown before paint cannot complete", app.model().events == []);
};
let asynchronous = () => {
  let app = start();
  let (enter_animation, resolve_enter, _) = pending();
  let (leave_animation, resolve_leave, _) = pending();
  let (new_animation, resolve_new, _) = pending();
  let animations = ref([|enter_animation|]);
  let snapshots = ref(0);
  put(
    node("#panel"),
    "getAnimations",
    options => {
      check("wait includes descendants", get(options, "subtree") == true);
      incr(snapshots);
      animations^;
    },
  );
  app.dispatch(Show);
  settle_frames();
  check(
    "pending enter stays Entering",
    A.phase(app.model().animation) == A.Entering
    && app.model().events == []
    && snapshots^ == 1,
  );
  check(
    "active enter has dropped closed",
    !flag("closed") && flag("enter") && !hidden(),
  );
  animations := [|leave_animation|];
  app.dispatch(Hide);
  settle_frames();
  check(
    "hide reverses unfinished enter",
    A.phase(app.model().animation) == A.Leaving
    && flag("leave")
    && flag("closed")
    && flag("transition")
    && !hidden(),
  );
  animations := [|new_animation|];
  app.dispatch(Show);
  settle_frames();
  check(
    "show reverses unfinished leave",
    A.phase(app.model().animation) == A.Entering
    && !flag("leave")
    && !hidden(),
  );
  resolve_leave();
  resolve_enter();
  Js.Promise.then_(
    _ => {
      settle_frames();
      check(
        "out of order old waits ignored",
        A.phase(app.model().animation) == A.Entering
        && app.model().events == [],
      );
      resolve_new();
      Js.Promise.then_(
        _ => {
          settle_frames();
          check(
            "newest wait alone emits",
            A.phase(app.model().animation) == A.Open
            && app.model().events == [A.Entered],
          );
          let (first, resolve_first, _) = pending();
          let (second, _, reject_second) = pending();
          animations := [|first, second|];
          app.dispatch(Hide);
          settle_frames();
          resolve_first();
          Js.Promise.then_(
            _ => {
              check(
                "all subtree animations awaited",
                A.phase(app.model().animation) == A.Leaving
                && app.model().events == [A.Entered],
              );
              reject_second();
              Js.Promise.then_(
                _ => {
                  settle_frames();
                  check(
                    "cancel counts as completion",
                    A.phase(app.model().animation) == A.Closed
                    && app.model().events == [A.Entered, A.Left],
                  );
                  let (animation, _, _) = pending();
                  animations := [|animation|];
                  app.dispatch(Show);
                  settle_frames();
                  app.dispatch(Remove);
                  settle_frames();
                  Js.Promise.then_(
                    _ => {
                      settle_frames();
                      check(
                        "removal during enter completes",
                        A.phase(app.model().animation) == A.Open
                        && app.model().events
                        == [A.Entered, A.Left, A.Entered],
                      );
                      app.dispatch(Hide);
                      settle_frames();
                      check(
                        "missing element leave completes",
                        A.phase(app.model().animation) == A.Closed,
                      );
                      stop(app);
                      let app = start(~open_=true, ());
                      let (animation, _, _) = pending();
                      put(node("#panel"), "getAnimations", _ =>
                        [|animation|]
                      );
                      app.dispatch(Hide);
                      settle_frames();
                      app.dispatch(Remove);
                      settle_frames();
                      Js.Promise.then_(
                        _ => {
                          settle_frames();
                          check(
                            "removal during leave completes",
                            app.model().events == [A.Left]
                            && A.phase(app.model().animation) == A.Closed,
                          );
                          stop(app);
                          let app = start();
                          let (animation, resolve, _) = pending();
                          put(node("#panel"), "getAnimations", _ =>
                            [|animation|]
                          );
                          app.dispatch(Show);
                          settle_frames();
                          app.shutdown();
                          resolve();
                          Js.Promise.then_(
                            _ => {
                              settle_frames();
                              check(
                                "shutdown during wait suppresses completion",
                                app.model().events == [],
                              );
                              check(
                                "shutdown leaves no root/global listeners",
                                Tea_dom.Debug.active_listeners() == 0
                                && listener_count() == 0,
                              );
                              report("animation");
                              Js.Promise.resolve();
                            },
                            tick(),
                          );
                        },
                        tick(),
                      );
                    },
                    tick(),
                  );
                },
                tick(),
              );
            },
            tick(),
          );
        },
        tick(),
      );
    },
    tick(),
  );
};
let completed = {
  synchronous();
  asynchronous();
};
