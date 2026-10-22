open Tea;
module C = Ui.Calendar;
module D = C.Date;
let date = s => Option.get(D.of_iso(s));
let today = date("2024-01-31");
let weekend = d => D.day_of_week(d) == 0 || D.day_of_week(d) == 6;
type msg =
  | Calendar(bool, C.msg)
  | Readonly(bool)
  | Unavailable(bool)
  | Single_day(bool)
  | Reset;
type model = {
  calendar: C.model,
  rtl: C.model,
  readonly: bool,
  unavailable: bool,
  single_day: bool,
  changes: int,
};
let initial = () => {
  calendar:
    C.init(
      ~id="calendar",
      ~focus=today,
      ~selected=today,
      ~first_day_of_week=1,
      (),
    ),
  rtl:
    C.init(
      ~id="rtl",
      ~focus=today,
      ~dir=C.Rtl,
      ~first_day_of_week=1,
      ~fixed_weeks=false,
      ~show_outside_month=true,
      (),
    ),
  readonly: false,
  unavailable: false,
  single_day: false,
  changes: 0,
};
let constraints = m => {
  C.min: Some(m.single_day ? today : date("2023-01-01")),
  max: Some(m.single_day ? today : date("2025-12-31")),
  is_unavailable: d => m.unavailable || weekend(d),
};
let update = (msg, m) =>
  switch (msg) {
  | Calendar(rtl, msg) =>
    let (calendar, cmd, outs) =
      C.update(
        ~constraints=constraints(m),
        ~readonly=m.readonly,
        msg,
        rtl ? m.rtl : m.calendar,
      );
    let changes =
      m.changes
      + List.length(
          List.filter(
            out =>
              switch (out) {
              | C.Changed(_) => true
              | _ => false
              },
            outs,
          ),
        );
    (
      rtl
        ? {
          ...m,
          rtl: calendar,
          changes,
        }
        : {
          ...m,
          calendar,
          changes,
        },
      Cmd.map(msg => Calendar(rtl, msg), cmd),
    );
  | Readonly(readonly) => (
      {
        ...m,
        readonly,
      },
      Cmd.none,
    )
  | Unavailable(unavailable) => (
      {
        ...m,
        unavailable,
      },
      Cmd.none,
    )
  | Single_day(single_day) => (
      {
        ...m,
        single_day,
      },
      Cmd.none,
    )
  | Reset => (initial(), Cmd.none)
  };
let render = (p: C.parts(msg)) =>
  Html.section(
    p.root,
    [
      Html.div(
        [Html.class_("navigation")],
        [
          Html.button(p.previous_year, [Html.text("- Year")]),
          Html.button(p.previous_month, [Html.text("- Month")]),
          Html.button(p.next_month, [Html.text("Month +")]),
          Html.button(p.next_year, [Html.text("Year +")]),
        ],
      ),
      Html.h2(p.heading, [Html.text(p.heading_text)]),
      Html.table(
        p.grid,
        [
          Html.thead(
            [],
            [
              Html.tr(
                [],
                Array.to_list(
                  Array.map(
                    (d: C.weekday(msg)) =>
                      Html.th(d.attrs, [Html.text(d.text)]),
                    p.weekdays,
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
                        (d: C.cell(msg)) =>
                          Html.td(d.attrs, [Html.text(d.text)]),
                        week,
                      ),
                    ),
                  ),
                p.weeks,
              ),
            ),
          ),
        ],
      ),
    ],
  );
let checkbox = (id, checked, tagger, label) =>
  Html.label(
    [],
    [
      Html.input(
        [
          Html.id(id),
          Html.type_("checkbox"),
          Html.checked(checked),
          Html.on_check(tagger),
        ],
        [],
      ),
      Html.text(label),
    ],
  );
let view = m =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Choose a day.")]),
      Html.p(
        [],
        [
          Html.text(
            "A civil-date calendar: Monday first, weekends unavailable. Dates are limited to 2023 through 2025. Today is fixed at 31 January 2024 so this example stays reproducible.",
          ),
        ],
      ),
      Html.p(
        [Html.id("help")],
        [
          Html.text(
            "Use arrow keys for days and weeks; Home/End for week edges. Page Up/Down changes month, with Shift changes year. Enter or Space selects. Tab leaves the grid.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("options")],
        [
          checkbox(
            "readonly",
            m.readonly,
            value => Readonly(value),
            "Read-only",
          ),
          checkbox(
            "unavailable",
            m.unavailable,
            value => Unavailable(value),
            "All dates unavailable",
          ),
          checkbox(
            "single-day",
            m.single_day,
            value => Single_day(value),
            "Limit selection to today",
          ),
          Html.button(
            [Html.id("reset"), Html.type_("button"), Html.on_click(Reset)],
            [Html.text("Reset examples")],
          ),
        ],
      ),
      Html.div(
        [Html.class_("calendars")],
        [
          Html.div(
            [],
            [
              Html.h2([], [Html.text("Monday-first calendar")]),
              C.view(
                ~model=m.calendar,
                ~constraints=constraints(m),
                ~readonly=m.readonly,
                ~today,
                ~to_parent=msg => Calendar(false, msg),
                ~to_view=render,
                (),
              ),
            ],
          ),
          Html.div(
            [],
            [
              Html.h2([], [Html.text("Right-to-left with adjacent days")]),
              C.view(
                ~model=m.rtl,
                ~constraints=constraints(m),
                ~readonly=m.readonly,
                ~today,
                ~to_parent=msg => Calendar(true, msg),
                ~to_view=render,
                (),
              ),
            ],
          ),
        ],
      ),
      Html.p(
        [Html.id("activity"), Html.aria("live", "polite")],
        [
          Html.text(
            "Selections: "
            ++ string_of_int(m.changes)
            ++ ". Selected date: "
            ++ (
              switch (C.selected(m.calendar)) {
              | None => "none"
              | Some(d) => D.to_iso(d)
              }
            )
            ++ ".",
          ),
        ],
      ),
    ],
  );
let () =
  ignore(
    App.element(
      ~init=() => (initial(), Cmd.none),
      ~update,
      ~view,
      ~subscriptions=
        m =>
          Sub.batch([
            Sub.map(
              msg => Calendar(false, msg),
              C.subscriptions(m.calendar),
            ),
            Sub.map(msg => Calendar(true, msg), C.subscriptions(m.rtl)),
          ]),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
