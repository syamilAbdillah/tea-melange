open Tea;
open Test_support;
module E = Browser_events;
[@mel.send] external blur_now: Dom.node => unit = "blur";
type model = {
  generation: int,
  pending: bool,
  ready: bool,
  alternate: bool,
  results: list(bool),
};
type msg =
  | Request
  | Cancel
  | Set_ready(bool)
  | Retarget
  | Completed(bool);
let start = (~target=E.Element("target"), ~ready=true, ~from=[], ()) =>
  App.element(
    ~init=
      () =>
        (
          {
            generation: 0,
            pending: false,
            ready,
            alternate: false,
            results: [],
          },
          Cmd.none,
        ),
    ~update=
      (msg, model) =>
        (
          switch (msg) {
          | Request => {
              ...model,
              generation: model.generation + 1,
              pending: true,
            }
          | Cancel => {
              ...model,
              pending: false,
            }
          | Set_ready(ready) => {
              ...model,
              ready,
            }
          | Retarget => {
              ...model,
              alternate: true,
            }
          | Completed(value) => {
              ...model,
              pending: false,
              results: model.results @ [value],
            }
          },
          Cmd.none,
        ),
    ~view=
      model =>
        Html.div(
          [],
          [
            Html.button([Html.id("before")], []),
            Html.div(
              [Html.id("root")],
              [
                Html.button(
                  [Html.id("target"), Html.disabled(!model.ready)],
                  [],
                ),
                Html.button(
                  [Html.id("last"), Html.disabled(!model.ready)],
                  [],
                ),
              ],
            ),
            Html.button([Html.id("outside")], []),
          ],
        ),
    ~subscriptions=
      model =>
        model.pending
          ? E.focus(
              ~key=string_of_int(model.generation),
              ~target=model.alternate ? E.Element("last") : target,
              ~ready=model.ready,
              ~from,
              value =>
              Completed(value)
            )
          : Sub.none,
    ~node="#app",
    ~flags=(),
    (),
  );
let completed = {
  let app = start();
  focus(node("#before"));
  app.dispatch(Request);
  check("focus waits for render", has_focus("#before"));
  frame();
  check(
    "focus completes once",
    has_focus("#target") && app.model().results == [true],
  );
  check("completion releases listener", listener_count() == 0);
  focus(node("#before"));
  app.dispatch(Request);
  app.dispatch(Request);
  frame();
  check(
    "new generation cancels older attempt",
    app.model().results == [true, true],
  );
  focus(node("#before"));
  app.dispatch(Request);
  focus(node("#outside"));
  frame();
  check(
    "user focus movement wins",
    has_focus("#outside") && app.model().results == [true, true, false],
  );
  focus(node("#before"));
  app.dispatch(Request);
  focus(node("#outside"));
  focus(node("#before"));
  frame();
  check(
    "away and back cancels",
    has_focus("#before") && app.model().results == [true, true, false, false],
  );
  app.dispatch(Request);
  app.dispatch(Cancel);
  check("removal releases listener immediately", listener_count() == 0);
  frame();
  check(
    "removed attempt cannot focus or deliver",
    has_focus("#before") && List.length(app.model().results) == 4,
  );
  app.dispatch(Request);
  app.shutdown();
  check("shutdown releases listener immediately", listener_count() == 0);
  frame();
  check("shutdown cancels queued focus", has_focus("#before"));
  let app = start();
  focus(node("#before"));
  app.dispatch(Request);
  blur_now(node("#before"));
  frame();
  check(
    "explicit blur to body supersedes pending focus",
    !has_focus("#target") && app.model().results == [false],
  );
  app.shutdown();
  frame();
  let app = start(~from=["before"], ());
  focus(node("#outside"));
  app.dispatch(Request);
  frame();
  check(
    "opening origin guard",
    has_focus("#outside") && app.model().results == [false],
  );
  app.shutdown();
  frame();
  let app = start(~ready=false, ());
  focus(node("#outside"));
  app.dispatch(Request);
  frame();
  check(
    "unready attempt retains the opening focus snapshot",
    has_focus("#outside") && app.model().results == [],
  );
  app.dispatch(Retarget);
  app.dispatch(Set_ready(true));
  frame();
  check(
    "kept source uses newest readiness and target",
    has_focus("#last") && app.model().results == [true],
  );
  app.shutdown();
  frame();
  let app = start(~ready=false, ());
  focus(node("#before"));
  app.dispatch(Request);
  frame();
  focus(node("#outside"));
  focus(node("#before"));
  frame();
  check(
    "pre-readiness away-and-back movement cancels",
    has_focus("#before") && app.model().results == [false],
  );
  app.dispatch(Set_ready(true));
  frame();
  check(
    "later readiness cannot revive an obsolete request",
    has_focus("#before") && app.model().results == [false],
  );
  app.shutdown();
  frame();
  let app = start(~ready=false, ());
  app.dispatch(Request);
  frame();
  app.shutdown();
  frame();
  check(
    "shutdown cancels readiness waits and listeners",
    listener_count() == 0,
  );
  let app = start(~target=E.Element("missing"), ());
  app.dispatch(Request);
  frame();
  check(
    "missing destination completes false",
    app.model().results == [false],
  );
  app.shutdown();
  frame();
  List.iter(
    ((target, selector)) => {
      let app = start(~target, ());
      app.dispatch(Request);
      frame();
      check(
        "tabbable destination",
        has_focus(selector) && app.model().results == [true],
      );
      app.shutdown();
      frame();
    },
    [
      (E.First_tabbable("root"), "#target"),
      (E.Last_tabbable("root"), "#last"),
    ],
  );
  check("no leaked listeners", listener_count() == 0);
  report("cancellable focus");
  Js.Promise.resolve();
};
