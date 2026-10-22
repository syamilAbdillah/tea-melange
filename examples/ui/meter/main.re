open Tea;
module C = Ui.Meter;
type model = {
  value: string,
  formatted: bool,
  optimum: float,
};
type msg =
  | Value(string)
  | Format(bool)
  | Optimum(string);
let update = (msg, model) => (
  switch (msg) {
  | Value(value) => {
      ...model,
      value,
    }
  | Format(formatted) => {
      ...model,
      formatted,
    }
  | Optimum(value) => {
      ...model,
      optimum: float_of_string(value),
    }
  },
  Cmd.none,
);
let option_ = (value, label) =>
  Html.option([Html.attribute("value", value)], [Html.text(label)]);
let view = model => {
  let value =
    switch (model.value) {
    | "nan" => nan
    | "inf" => infinity
    | "-inf" => neg_infinity
    | _ => float_of_string(model.value)
    };
  let format =
    model.formatted ? Some(v => Js.Float.toString(v) ++ " units") : None;
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Capacity at a glance")]),
      Html.p(
        [],
        [
          Html.text(
            "A bounded reading, not a task. Change the reading and optimum to compare the three quality bands.",
          ),
        ],
      ),
      Html.section(
        [],
        [
          Html.h2([], [Html.text("Reading controls")]),
          Html.label([Html.for_("value")], [Html.text("Value")]),
          Html.select(
            [
              Html.id("value"),
              Html.value(model.value),
              Html.on_change(value => Value(value)),
            ],
            [
              option_("0", "0"),
              option_("25", "25"),
              option_("50", "50"),
              option_("75", "75"),
              option_("100", "100"),
              option_("-20", "Below minimum"),
              option_("120", "Above maximum"),
              option_("nan", "Not a number"),
              option_("inf", "Positive infinity"),
              option_("-inf", "Negative infinity"),
            ],
          ),
          Html.label([Html.for_("optimum")], [Html.text("Optimum")]),
          Html.select(
            [
              Html.id("optimum"),
              Html.value(Js.Float.toString(model.optimum)),
              Html.on_change(value => Optimum(value)),
            ],
            [
              option_("0", "Low"),
              option_("50", "Middle"),
              option_("100", "High"),
            ],
          ),
          Html.label(
            [],
            [
              Html.input(
                [
                  Html.id("format"),
                  Html.type_("checkbox"),
                  Html.checked(model.formatted),
                  Html.on_check(value => Format(value)),
                ],
                [],
              ),
              Html.text(" Describe units"),
            ],
          ),
        ],
      ),
      C.view(
        ~id="reading",
        ~value,
        ~min=0.,
        ~max=100.,
        ~low=25.,
        ~high=75.,
        ~optimum=model.optimum,
        ~format?,
        ~to_view=
          p =>
            Html.section(
              [],
              [
                Html.h2(p.label, [Html.text("Storage used")]),
                Html.div(p.root @ [Html.class_("track")], []),
                Html.p(
                  [Html.id("state"), Html.class_("reading")],
                  [Html.text(p.state)],
                ),
                Html.p(
                  [],
                  [Html.text(Js.Float.toString(p.value) ++ " / 100")],
                ),
              ],
            ),
        (),
      ),
      C.view(
        ~id="default",
        ~value=0.5,
        ~has_label=false,
        ~aria_label="Default range",
        ~to_view=
          p =>
            Html.section(
              [],
              [
                Html.h2([], [Html.text("Default range")]),
                Html.div(p.root @ [Html.class_("track")], []),
              ],
            ),
        (),
      ),
    ],
  );
};
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              value: "50",
              formatted: true,
              optimum: 50.,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=_ => Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
