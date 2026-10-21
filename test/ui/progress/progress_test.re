open Tea;
open Test_support;
[@mel.send] external css: ('a, string) => string = "getPropertyValue";
module P = Ui.Progress;
type model = {
  value: option(float),
  formatted: bool,
};
type msg =
  | Set(option(float))
  | Format(bool);
let update = (msg, model) => (
  switch (msg) {
  | Set(value) => {
      ...model,
      value,
    }
  | Format(formatted) => {
      ...model,
      formatted,
    }
  },
  Cmd.none,
);
let view = model => {
  let value = model.value;
  let format =
    model.formatted
      ? Some(value => Js.Float.toString(value) ++ " units") : None;
  P.view(
    ~id="display",
    ~value?,
    ~min=0.,
    ~max=100.,
    ~format?,
    ~to_view=
      p =>
        Html.div(
          [],
          [
            Html.span(p.label, [Html.text("Capacity")]),
            Html.div(p.root, []),
            Html.div(
              p.region @ [Html.id("region")],
              [Html.text("Loaded content")],
            ),
          ],
        ),
    (),
  );
};
let rejected = f =>
  try(
    {
      f();
      false;
    }
  ) {
  | Invalid_argument(_) => true
  };
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              value: Some(25.),
              formatted: true,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=_ => Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  check("role", attr("#display", "role") == Some("progressbar"));
  check(
    "label resolves",
    attr("#display", "aria-labelledby") == Some("display-label")
    && get(node("#display-label"), "textContent") == "Capacity",
  );
  check(
    "bounds",
    attr("#display", "aria-valuemin") == Some("0")
    && attr("#display", "aria-valuemax") == Some("100"),
  );
  check(
    "formatted",
    attr("#display", "aria-valuetext") == Some("25 units"),
  );
  check(
    "percentage",
    css(get(node("#display"), "style"), "--tea-progress-percent") == "25%",
  );
  check("not focusable", attr("#display", "tabindex") == None);
  check(
    "keys untouched",
    key("#display", "keydown", "ArrowDown")
    && key("#display", "keydown", "Tab"),
  );
  pointer("#display");
  click("#display");
  check("display noninteractive", app.model().value == Some(25.));
  List.iter(
    ((input, expected)) => {
      app.dispatch(Set(Some(input)));
      frame();
      check(
        "clamped value",
        attr("#display", "aria-valuenow")
        == Some(Js.Float.toString(expected)),
      );
      check(
        "formatter sees clamped value",
        attr("#display", "aria-valuetext")
        == Some(Js.Float.toString(expected) ++ " units"),
      );
    },
    [
      ((-10.), 0.),
      (150., 100.),
      (nan, 0.),
      (infinity, 100.),
      (neg_infinity, 0.),
      (0., 0.),
      (100., 100.),
      (0.5, 0.5),
    ],
  );
  app.dispatch(Format(false));
  frame();
  check(
    "optional formatter removed",
    attr("#display", "aria-valuetext") == None,
  );
  check(
    "complete",
    {
      app.dispatch(Set(Some(100.)));
      frame();
      attr("#display", "data-state") == Some("complete")
      && attr("#region", "aria-busy") == Some("false");
    },
  );
  app.dispatch(Set(None));
  app.dispatch(Format(true));
  frame();
  check(
    "indeterminate removes numeric semantics",
    attr("#display", "data-state") == Some("indeterminate")
    && attr("#display", "aria-valuenow") == None
    && attr("#display", "aria-valuetext") == None,
  );
  check(
    "indeterminate removes geometry",
    css(get(node("#display"), "style"), "--tea-progress-percent") == "",
  );
  check(
    "region busy and description",
    attr("#region", "aria-busy") == Some("true")
    && attr("#region", "aria-describedby") == Some("display"),
  );
  app.dispatch(Set(Some(0.)));
  frame();
  check(
    "loading",
    attr("#display", "data-state") == Some("loading")
    && attr("#region", "aria-busy") == Some("true"),
  );
  check(
    "restores percentage",
    css(get(node("#display"), "style"), "--tea-progress-percent") == "0%",
  );
  check(
    "default indeterminate",
    P.parts(~id="pure", ()).value == None
    && P.parts(~id="pure", ()).percent == None,
  );
  List.iter(
    ((min, max)) =>
      check(
        "invalid bounds rejected",
        rejected(() =>
          ignore(P.parts(~id="pure", ~value=0., ~min, ~max, ()))
        ),
      ),
    [
      (1., 0.),
      (1., 1.),
      (nan, 1.),
      (0., nan),
      (neg_infinity, 1.),
      (0., infinity),
    ],
  );
  let extreme =
    P.parts(~id="pure", ~value=0., ~min=-1.7e308, ~max=1.7e308, ());
  check("overflow-safe percentage", extreme.percent == Some(50.));
  let tiny = P.parts(~id="pure", ~value=1.e-310, ~min=0., ~max=2.e-310, ());
  check("subnormal percentage", tiny.percent == Some(50.));
  app.dispatch(Set(Some(5.)));
  app.dispatch(Set(Some(95.)));
  frame();
  check(
    "rapid parent updates use last value",
    attr("#display", "aria-valuenow") == Some("95"),
  );
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("progress");
  Js.Promise.resolve();
};
