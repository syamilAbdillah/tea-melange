open Tea;
module C = Ui.Progress;
type model = {
  value: string,
  formatted: bool,
};
type msg =
  | Value(string)
  | Format(bool);
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
  },
  Cmd.none,
);
let option_ = (value, label) =>
  Html.option([Html.attribute("value", value)], [Html.text(label)]);
let view = model => {
  let value =
    String.equal(model.value, "none")
      ? None
      : Some(
          switch (model.value) {
          | "nan" => nan
          | "inf" => infinity
          | "-inf" => neg_infinity
          | _ => float_of_string(model.value)
          },
        );
  let format =
    model.formatted ? Some(v => Js.Float.toString(v) ++ " units") : None;
  Html.main(
    [],
    [
      Html.h1([], [Html.text("A task taking shape")]),
      Html.p(
        [],
        [
          Html.text(
            "Known completion is a percentage. Unknown completion is a distinct state, not zero.",
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
              option_("none", "Unknown"),
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
        ~value?,
        ~min=0.,
        ~max=100.,
        ~format?,
        ~to_view=
          p =>
            Html.section(
              [],
              [
                Html.h2(p.label, [Html.text("Import progress")]),
                Html.div(p.root @ [Html.class_("track")], []),
                Html.p(
                  [Html.id("state"), Html.class_("reading")],
                  [Html.text(p.state)],
                ),
                Html.div(
                  p.region @ [Html.id("loaded")],
                  [
                    Html.p([], [Html.text("Imported records appear here.")]),
                  ],
                ),
              ],
            ),
        (),
      ),
      C.view(
        ~id="default",
        ~to_view=
          p =>
            Html.section(
              [],
              [
                Html.h2(p.label, [Html.text("Default range")]),
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
