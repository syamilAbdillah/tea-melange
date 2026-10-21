open Tea;
open Test_support;
[@mel.send] external css: ('a, string) => string = "getPropertyValue";
module M = Ui.Meter;
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
  M.view(
    ~id="display",
    ~value=Option.value(~default=0., value),
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
            Html.text(""),
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
  check("role", attr("#display", "role") == Some("meter"));
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
    css(get(node("#display"), "style"), "--tea-meter-percent") == "25%",
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
    "default bounds",
    M.parts(~id="pure", ~value=0.5, ()).percent == 50.,
  );
  let state = (~low=25., ~high=75., ~optimum=50., value) =>
    M.parts(~id="pure", ~value, ~min=0., ~max=100., ~low, ~high, ~optimum, ()).
      state;
  List.iter(
    ((value, expected)) =>
      check("middle optimum boundaries", state(value) == expected),
    [
      (0., "suboptimal"),
      (24.99, "suboptimal"),
      (25., "optimum"),
      (50., "optimum"),
      (75., "optimum"),
      (75.01, "suboptimal"),
      (100., "suboptimal"),
    ],
  );
  List.iter(
    ((value, expected)) =>
      check("low optimum boundaries", state(~optimum=0., value) == expected),
    [
      (0., "optimum"),
      (25., "optimum"),
      (25.01, "suboptimal"),
      (75., "suboptimal"),
      (75.01, "critical"),
      (100., "critical"),
    ],
  );
  List.iter(
    ((value, expected)) =>
      check(
        "high optimum boundaries",
        state(~optimum=100., value) == expected,
      ),
    [
      (0., "critical"),
      (24.99, "critical"),
      (25., "suboptimal"),
      (74.99, "suboptimal"),
      (75., "optimum"),
      (100., "optimum"),
    ],
  );
  check(
    "optimum equals low in middle",
    state(~optimum=25., 0.) == "suboptimal"
    && state(~optimum=25., 25.) == "optimum",
  );
  check(
    "optimum equals high in middle",
    state(~optimum=75., 100.) == "suboptimal"
    && state(~optimum=75., 75.) == "optimum",
  );
  check(
    "coincident thresholds",
    state(~low=50., ~high=50., 50.) == "optimum"
    && state(~low=50., ~high=50., 49.) == "suboptimal",
  );
  check(
    "reversed thresholds normalize",
    state(~low=75., ~high=25., ~optimum=100., 74.) == "critical",
  );
  check(
    "out of range thresholds clamp",
    state(~low=neg_infinity, ~high=infinity, ~optimum=nan, 50.) == "optimum",
  );
  check(
    "finite thresholds clamp",
    state(~low=-1., ~high=101., ~optimum=200., 100.) == "optimum",
  );
  check(
    "no thresholds entire range optimum",
    M.parts(~id="pure", ~value=0., ()).state == "optimum"
    && M.parts(~id="pure", ~value=1., ()).state == "optimum",
  );
  check(
    "invalid thresholds use defaults",
    M.parts(
      ~id="pure",
      ~value=0.5,
      ~low=nan,
      ~high=nan,
      ~optimum=infinity,
      (),
    ).
      state
    == "optimum",
  );
  List.iter(
    ((min, max)) =>
      check(
        "invalid bounds rejected",
        rejected(() =>
          ignore(M.parts(~id="pure", ~value=0., ~min, ~max, ()))
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
    M.parts(~id="pure", ~value=0., ~min=-1.7e308, ~max=1.7e308, ());
  check("overflow-safe percentage", extreme.percent == 50.);
  let tiny = M.parts(~id="pure", ~value=1.e-310, ~min=0., ~max=2.e-310, ());
  check("subnormal percentage", tiny.percent == 50.);
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
  report("meter");
  Js.Promise.resolve();
};
