open Tea;
module P = Ui.Date_picker;
module C = Ui.Calendar;
module D = Ui.Dialog;
let date = iso => Option.value(~default=C.Date.first, C.Date.of_iso(iso));
let today = date("2026-03-05");
let custom_format = date => {
  let iso = C.Date.to_iso(date);
  String.sub(iso, 8, 2)
  ++ "/"
  ++ String.sub(iso, 5, 2)
  ++ "/"
  ++ String.sub(iso, 0, 4);
};
let custom_parse = text =>
  String.length(text) == 10 && text.[2] == '/' && text.[5] == '/'
    ? C.Date.of_iso(
        String.sub(text, 6, 4)
        ++ "-"
        ++ String.sub(text, 3, 2)
        ++ "-"
        ++ String.sub(text, 0, 2),
      )
    : None;
let constraints = name =>
  String.equal(name, "range")
    ? {
      C.min: C.Date.of_iso("2026-03-02"),
      max: C.Date.of_iso("2026-03-27"),
      is_unavailable: date =>
        C.Date.day_of_week(date) == 0 || C.Date.day_of_week(date) == 6,
    }
    : C.unconstrained;
let names = [
  "basic",
  "custom",
  "range",
  "confirm",
  "form",
  "rtl",
  "readonly",
  "disabled",
  "nested",
];
let make = name =>
  P.init(
    ~id=name,
    ~focus=today,
    ~selected=today,
    ~confirm=String.equal(name, "confirm"),
    ~format=String.equal(name, "custom") ? custom_format : C.Date.to_iso,
    ~parse=String.equal(name, "custom") ? custom_parse : C.Date.of_iso,
    ~dir=String.equal(name, "rtl") ? C.Rtl : C.Ltr,
    (),
  );
type msg =
  | Picker(string, P.msg)
  | Dialog(D.msg)
  | Open_dialog
  | Submit
  | Remove
  | Reset;
type model = {
  pickers: list((string, P.model)),
  dialog: D.model,
  activity: string,
  submitted: string,
  mounted: bool,
};
let reason = reason =>
  switch (reason) {
  | P.Escape => "Escape"
  | P.Outside => "Outside"
  | P.Selection => "Selection"
  | P.Confirm => "Confirm"
  | P.Cancel => "Cancel"
  | P.Programmatic => "Programmatic"
  };
let update = (message, model) =>
  switch (message) {
  | Picker(name, message) =>
    let (picker, command, outs) =
      P.update(
        ~constraints=constraints(name),
        ~disabled=String.equal(name, "disabled"),
        ~readonly=String.equal(name, "readonly"),
        message,
        List.assoc(name, model.pickers),
      );
    let activity =
      List.fold_left(
        (_activity, out) =>
          switch (out) {
          | P.Opened => name ++ " Opened"
          | P.Closed(cause) => name ++ " Closed(" ++ reason(cause) ++ ")"
          | P.Changed(value) =>
            name
            ++ " Changed("
            ++ Option.fold(~none="empty", ~some=C.Date.to_iso, value)
            ++ ")"
          | P.Invalid(text, _) => name ++ " Invalid(" ++ text ++ ")"
          },
        model.activity,
        outs,
      );
    (
      {
        ...model,
        pickers:
          List.map(
            ((key, old)) => (key, String.equal(name, key) ? picker : old),
            model.pickers,
          ),
        activity,
      },
      Cmd.map(message => Picker(name, message), command),
    );
  | Open_dialog =>
    let (dialog, command) = D.open_(model.dialog);
    (
      {
        ...model,
        dialog,
      },
      Cmd.map(message => Dialog(message), command),
    );
  | Dialog(message) =>
    let (dialog, command, _) = D.update(message, model.dialog);
    (
      {
        ...model,
        dialog,
      },
      Cmd.map(message => Dialog(message), command),
    );
  | Submit => (
      {
        ...model,
        submitted:
          Option.fold(
            ~none="empty",
            ~some=C.Date.to_iso,
            P.value(List.assoc("form", model.pickers)),
          ),
      },
      Cmd.none,
    )
  | Remove => (
      {
        ...model,
        mounted: false,
      },
      Cmd.none,
    )
  | Reset => (
      {
        ...model,
        mounted: true,
        pickers: List.map(name => (name, make(name)), names),
      },
      Cmd.none,
    )
  };
let field_label = name =>
  switch (name) {
  | "basic" => "Field visit (ISO)"
  | "custom" => "Seed delivery (DD/MM/YYYY)"
  | "range" => "Weekday visit in March"
  | "confirm" => "Reserve a workshop"
  | "form" => "Booking date"
  | "rtl" => "Right-to-left date"
  | "readonly" => "Read-only visit"
  | "disabled" => "Disabled visit"
  | _ => "Date inside a modal"
  };
let render = (model, name) =>
  P.view(
    ~model=List.assoc(name, model.pickers),
    ~constraints=constraints(name),
    ~disabled=String.equal(name, "disabled"),
    ~readonly=String.equal(name, "readonly"),
    ~has_description=true,
    ~name=name ++ "-date",
    ~today,
    ~to_parent=message => Picker(name, message),
    ~to_view=
      p =>
        Html.div(
          [Html.class_("picker")],
          [
            Html.label(p.label, [Html.text(field_label(name))]),
            Html.div(
              p.group @ [Html.class_("field")],
              [
                Html.input(p.input, []),
                Html.button(p.trigger, [Html.text(P.english.choose)]),
                Html.div(
                  p.dialog @ [Html.class_("panel")],
                  [
                    Html.span(p.start_boundary, []),
                    Html.div(
                      p.calendar.root,
                      [
                        Html.h2(
                          p.calendar.heading,
                          [Html.text(p.calendar.heading_text)],
                        ),
                        Html.div(
                          [Html.class_("nav")],
                          [
                            Html.button(
                              p.calendar.previous_year,
                              [Html.text("Previous year")],
                            ),
                            Html.button(
                              p.calendar.previous_month,
                              [Html.text("Previous month")],
                            ),
                            Html.button(
                              p.calendar.next_month,
                              [Html.text("Next month")],
                            ),
                            Html.button(
                              p.calendar.next_year,
                              [Html.text("Next year")],
                            ),
                          ],
                        ),
                        Html.table(
                          p.calendar.grid,
                          [
                            Html.thead(
                              [],
                              [
                                Html.tr(
                                  [],
                                  Array.to_list(
                                    Array.map(
                                      (day: C.weekday(msg)) =>
                                        Html.th(
                                          day.attrs,
                                          [Html.text(day.text)],
                                        ),
                                      p.calendar.weekdays,
                                    ),
                                  ),
                                ),
                              ],
                            ),
                            Html.tbody(
                              [],
                              Array.to_list(
                                Array.map(
                                  week =>
                                    Html.tr(
                                      [],
                                      Array.to_list(
                                        Array.map(
                                          (cell: C.cell(msg)) =>
                                            Html.td(
                                              cell.attrs,
                                              [Html.text(cell.text)],
                                            ),
                                          week,
                                        ),
                                      ),
                                    ),
                                  p.calendar.weeks,
                                ),
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                    Html.div(
                      [Html.class_("actions")],
                      [
                        p.confirm
                          ? Html.button(
                              p.confirm_button,
                              [Html.text(P.english.confirm)],
                            )
                          : Html.text(""),
                        Html.button(
                          p.cancel_button,
                          [Html.text(P.english.cancel)],
                        ),
                      ],
                    ),
                    Html.p(
                      p.instructions @ [Html.class_("sr-only")],
                      [Html.text(p.instructions_text)],
                    ),
                    Html.span(p.end_boundary, []),
                  ],
                ),
              ],
            ),
            Html.p(
              p.description,
              [
                Html.text(
                  String.equal(name, "custom")
                    ? "Use DD/MM/YYYY; for example 05/03/2026."
                    : String.equal(name, "range")
                        ? "March 2 through March 27, 2026. Weekends are unavailable."
                        : "Use YYYY-MM-DD; for example 2026-03-05.",
                ),
              ],
            ),
            Html.p(
              p.error @ [Html.class_("error")],
              [Html.text(p.error_text)],
            ),
            Html.input(p.hidden_input, []),
          ],
        ),
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Plan a day in the garden")]),
      Html.p(
        [],
        [
          Html.text(
            "Editable dates, weekday visits and workshop reservations. All examples use March 5, 2026 as today, not your device clock.",
          ),
        ],
      ),
      model.mounted
        ? Html.div(
            [],
            [
              Html.section(
                [Html.aria("label", "Date formats")],
                [render(model, "basic"), render(model, "custom")],
              ),
              Html.section(
                [Html.aria("label", "Selection policies")],
                [render(model, "range"), render(model, "confirm")],
              ),
              Html.section(
                [Html.aria("label", "Form submission")],
                [
                  Html.form(
                    [Html.id("booking"), Html.on_submit(Submit)],
                    [
                      render(model, "form"),
                      Html.button(
                        [Html.id("submit"), Html.type_("submit")],
                        [Html.text("Book visit")],
                      ),
                    ],
                  ),
                  Html.node(
                    "output",
                    [Html.id("submitted")],
                    [Html.text(model.submitted)],
                  ),
                ],
              ),
              Html.section(
                [Html.aria("label", "Direction and modes")],
                [
                  render(model, "rtl"),
                  render(model, "readonly"),
                  render(model, "disabled"),
                ],
              ),
              Ui.Button.button(
                ~id="open-modal",
                ~on_press=Open_dialog,
                [Html.text("Plan inside a modal")],
              ),
              D.view(
                ~model=model.dialog,
                ~to_parent=message => Dialog(message),
                ~to_view=
                  d =>
                    Html.dialog(
                      d.root,
                      [
                        Html.h2(d.title, [Html.text("Visit details")]),
                        render(model, "nested"),
                        Html.button(d.close_button, [Html.text("Close")]),
                      ],
                    ),
                (),
              ),
            ],
          )
        : Html.p([], [Html.text("Date pickers unmounted.")]),
      Html.node(
        "output",
        [Html.id("activity"), Html.aria("live", "polite")],
        [Html.text(model.activity)],
      ),
      Ui.Button.button(
        ~id="outside",
        ~on_press=Submit,
        [Html.text("Outside focus target")],
      ),
      Ui.Button.button(
        ~id="unmount",
        ~on_press=Remove,
        [Html.text("Unmount date pickers")],
      ),
      Ui.Button.button(
        ~id="reset",
        ~on_press=Reset,
        [Html.text("Reset date pickers")],
      ),
    ],
  );
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              pickers: List.map(name => (name, make(name)), names),
              dialog: D.init(~id="details", ()),
              activity: "Ready",
              submitted: "",
              mounted: true,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          model.mounted
            ? Sub.batch([
                Sub.map(
                  message => Dialog(message),
                  D.subscriptions(model.dialog),
                ),
                Sub.batch(
                  List.map(
                    ((name, picker)) =>
                      Sub.map(
                        message => Picker(name, message),
                        P.subscriptions(picker),
                      ),
                    model.pickers,
                  ),
                ),
              ])
            : Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
