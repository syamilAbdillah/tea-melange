open Tea;
open Test_support;
module C = Ui.Calendar;
module D = C.Date;
let date = s => Option.get(D.of_iso(s));
let initial_date = date("2024-01-31");
let cell = s => "#cal-item-" ++ s;
[@mel.send]
external query_all: (Dom.document, string) => Dom.nodeList =
  "querySelectorAll";
[@mel.get] external length: Dom.nodeList => int = "length";
[@mel.get] external text: Dom.node => string = "textContent";
let total = s => length(query_all(document, s));
type msg =
  | Calendar(C.msg)
  | Focus(D.t)
  | Selection(option(D.t))
  | Limits(C.constraints)
  | Readonly(bool)
  | Labels(C.labels)
  | Hook
  | Reinit(C.model);
type model = {
  calendar: C.model,
  constraints: C.constraints,
  readonly: bool,
  labels: C.labels,
  hooks: int,
  outs: list(C.out_msg),
};
let update = (msg, m) =>
  switch (msg) {
  | Calendar(msg) =>
    let (calendar, cmd, outs) =
      C.update(
        ~constraints=m.constraints,
        ~readonly=m.readonly,
        msg,
        m.calendar,
      );
    (
      {
        ...m,
        calendar,
        outs: m.outs @ outs,
      },
      Cmd.map(msg => Calendar(msg), cmd),
    );
  | Focus(date) =>
    let calendar = C.set_focus(date, m.calendar);
    (
      {
        ...m,
        calendar,
      },
      Cmd.map(msg => Calendar(msg), C.focus_grid(calendar)),
    );
  | Selection(date) => (
      {
        ...m,
        calendar: C.set_selected(date, m.calendar),
      },
      Cmd.none,
    )
  | Limits(constraints) => (
      {
        ...m,
        constraints,
      },
      Cmd.none,
    )
  | Readonly(readonly) => (
      {
        ...m,
        readonly,
      },
      Cmd.none,
    )
  | Labels(labels) => (
      {
        ...m,
        labels,
      },
      Cmd.none,
    )
  | Hook => (
      {
        ...m,
        hooks: m.hooks + 1,
      },
      Cmd.none,
    )
  | Reinit(calendar) => (
      {
        ...m,
        calendar,
      },
      Cmd.none,
    )
  };
let render = (p: C.parts(msg)) =>
  Html.div(
    p.root,
    [
      Html.h2(p.heading, [Html.text(p.heading_text)]),
      Html.button(p.previous_year, [Html.text("Previous year")]),
      Html.button(p.previous_month, [Html.text("Previous month")]),
      Html.button(p.next_month, [Html.text("Next month")]),
      Html.button(p.next_year, [Html.text("Next year")]),
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
let view = m =>
  C.view(
    ~model=m.calendar,
    ~constraints=m.constraints,
    ~readonly=m.readonly,
    ~today=initial_date,
    ~labels=m.labels,
    ~on_key_down=
      key =>
        String.equal(key, "Escape") || String.equal(key, "Tab")
          ? Some(Hook) : None,
    ~to_parent=msg => Calendar(msg),
    ~to_view=render,
    (),
  );
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              calendar:
                C.init(
                  ~id="cal",
                  ~focus=initial_date,
                  ~selected=initial_date,
                  ~first_day_of_week=1,
                  (),
                ),
              constraints: C.unconstrained,
              readonly: false,
              labels: C.english,
              hooks: 0,
              outs: [],
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        m => Sub.map(msg => Calendar(msg), C.subscriptions(m.calendar)),
      ~node="#app",
      ~flags=(),
      (),
    );
  let current = () => D.to_iso(C.focus_date(app.model().calendar));
  let selection = () =>
    Option.map(D.to_iso, C.selected(app.model().calendar));
  let changed = () =>
    List.length(
      List.filter(
        out =>
          switch (out) {
          | C.Changed(_) => true
          | _ => false
          },
        app.model().outs,
      ),
    );
  let set = s => {
    app.dispatch(Focus(date(s)));
    frame();
    check("programmatic focus command", has_focus(cell(s)));
  };
  let one_stop = () =>
    check(
      "exactly one grid tab stop",
      total("#cal-grid [tabindex='0']") == 1,
    );
  one_stop();
  check(
    "grid role and idref",
    attr("#cal-grid", "role") == Some("grid")
    && attr("#cal-grid", "aria-labelledby") == Some("cal-heading"),
  );
  check(
    "heading live region",
    attr("#cal-heading", "aria-live") == Some("polite")
    && attr("#cal-heading", "aria-atomic") == Some("true")
    && text(node("#cal-heading")) == "January 2024",
  );
  check(
    "six rows seven headers",
    total("#cal-grid tbody tr") == 6
    && total("#cal-grid th") == 7
    && total("#cal-grid td") == 42,
  );
  check(
    "full weekday header",
    attr("#cal-grid th", "abbr") == Some("Monday")
    && attr("#cal-grid th", "aria-label") == Some("Monday"),
  );
  check(
    "selected today",
    attr(cell("2024-01-31"), "aria-selected") == Some("true")
    && attr(cell("2024-01-31"), "aria-current") == Some("date")
    && attr(cell("2024-01-31"), "data-selected") == Some("")
    && attr(cell("2024-01-31"), "data-today") == Some(""),
  );
  check(
    "outside days absent",
    total("#cal-grid [data-outside-month]") == 0
    && total("#cal-grid td:not([id])") == 11,
  );
  let initial_outs = app.model().outs;
  set("2024-01-31");
  check(
    "programmatic changes emit no outs",
    app.model().outs == initial_outs,
  );
  /* Both events see the same DOM but update against the newest model. */
  check("right claimed", !key(cell("2024-01-31"), "keydown", "ArrowRight"));
  check(
    "second right claimed",
    !key(cell("2024-01-31"), "keydown", "ArrowRight"),
  );
  check("batched state latest", current() == "2024-02-02");
  frame();
  check("batched focus latest", has_focus(cell("2024-02-02")));
  check(
    "month announcement",
    text(node("#cal-heading")) == "February 2024",
  );
  one_stop();
  check(
    "focused out messages",
    List.exists(
      out =>
        switch (out) {
        | C.Focused(d) => D.to_iso(d) == "2024-02-02"
        | _ => false
        },
      app.model().outs,
    ),
  );
  check(
    "month out message",
    List.exists(
      out =>
        switch (out) {
        | C.Month_changed(d) => D.to_iso(d) == "2024-02-01"
        | _ => false
        },
      app.model().outs,
    ),
  );
  List.iter(
    ((start, name, shift, expected)) => {
      set(start);
      check(
        "keyboard row claims default",
        !key(~shift, cell(start), "keydown", name),
      );
      frame();
      check(
        "keyboard row " ++ name,
        current() == expected && has_focus(cell(expected)),
      );
      one_stop();
    },
    [
      ("2024-01-31", "ArrowLeft", false, "2024-01-30"),
      ("2024-01-31", "ArrowRight", false, "2024-02-01"),
      ("2024-01-31", "ArrowUp", false, "2024-01-24"),
      ("2024-01-31", "ArrowDown", false, "2024-02-07"),
      ("2024-01-31", "Home", false, "2024-01-29"),
      ("2024-01-31", "End", false, "2024-02-04"),
      ("2024-03-31", "PageUp", false, "2024-02-29"),
      ("2024-01-31", "PageDown", false, "2024-02-29"),
      ("2024-02-29", "PageUp", true, "2023-02-28"),
      ("2024-02-29", "PageDown", true, "2025-02-28"),
      ("0001-01-01", "ArrowLeft", false, "0001-01-01"),
      ("9999-12-31", "ArrowRight", false, "9999-12-31"),
    ],
  );
  set("2024-01-31");
  List.iter(
    name => {
      check(
        "ctrl untouched",
        key(~ctrl=true, cell("2024-01-31"), "keydown", name),
      );
      check(
        "alt untouched",
        key(~alt=true, cell("2024-01-31"), "keydown", name),
      );
      check(
        "meta untouched",
        key(~meta=true, cell("2024-01-31"), "keydown", name),
      );
      check(
        "IME untouched",
        key(~composing=true, cell("2024-01-31"), "keydown", name),
      );
    },
    [
      "ArrowLeft",
      "ArrowRight",
      "ArrowUp",
      "ArrowDown",
      "Home",
      "End",
      "PageUp",
      "PageDown",
      "Enter",
      " ",
    ],
  );
  check(
    "Tab native",
    key(cell("2024-01-31"), "keydown", "Tab")
    && key(~shift=true, cell("2024-01-31"), "keydown", "Tab"),
  );
  check("other keys native", key(cell("2024-01-31"), "keydown", "x"));
  check(
    "unhandled hook preserves default",
    key(cell("2024-01-31"), "keydown", "Escape"),
  );
  check(
    "Tab and Escape hooks in parent message type",
    app.model().hooks == 3,
  );
  ignore(key(~ctrl=true, cell("2024-01-31"), "keydown", "Escape"));
  ignore(key(~composing=true, cell("2024-01-31"), "keydown", "Escape"));
  check("hook shortcuts and IME untouched", app.model().hooks == 3);
  check(
    "shortcuts do not mutate",
    current() == "2024-01-31" && changed() == 0,
  );
  app.dispatch(Selection(None));
  frame();
  check(
    "clear selection",
    selection() == None && total("#cal-grid [aria-selected]") == 0,
  );
  let before = changed();
  ignore(key(cell("2024-01-31"), "keydown", "Enter"));
  frame();
  check(
    "Enter selects once",
    selection() == Some("2024-01-31") && changed() == before + 1,
  );
  set("2024-01-30");
  check("Space prevents scroll", !key(cell("2024-01-30"), "keydown", " "));
  frame();
  check(
    "Space selects",
    selection() == Some("2024-01-30") && changed() == before + 2,
  );
  check("Space up untouched", key(cell("2024-01-30"), "keyup", " "));
  click(cell("2024-01-29"));
  check(
    "pointer selects and focuses",
    selection() == Some("2024-01-29")
    && current() == "2024-01-29"
    && has_focus(cell("2024-01-29")),
  );
  let limits = {
    C.min: Some(date("2024-01-15")),
    max: Some(date("2024-03-10")),
    is_unavailable: d => D.day_of_week(d) == 0 || D.day_of_week(d) == 6,
  };
  app.dispatch(Limits(limits));
  frame();
  check(
    "min month previous disabled",
    attr("#cal-previous-month", "aria-disabled") == Some("true")
    && attr("#cal-previous-month", "tabindex") == Some("-1")
    && attr("#cal-previous-month", "data-disabled") == Some(""),
  );
  check(
    "year buttons bounded",
    attr("#cal-previous-year", "aria-disabled") == Some("true")
    && attr("#cal-next-year", "aria-disabled") == Some("true"),
  );
  set("2024-01-13");
  check(
    "unavailable focusable",
    attr(cell("2024-01-13"), "aria-disabled") == Some("true")
    && has_focus(cell("2024-01-13")),
  );
  let before = changed();
  ignore(key(cell("2024-01-13"), "keydown", "Enter"));
  ignore(key(cell("2024-01-13"), "keydown", " "));
  frame();
  click(cell("2024-01-13"));
  check(
    "disabled never selects",
    changed() == before && selection() == Some("2024-01-29"),
  );
  set("2024-01-14");
  ignore(key(cell("2024-01-14"), "keydown", "ArrowRight"));
  frame();
  check("disabled not skipped", current() == "2024-01-15");
  set("2024-01-12");
  click(cell("2024-01-12"));
  check(
    "before min never selects",
    changed() == before
    && attr(cell("2024-01-12"), "aria-disabled") == Some("true"),
  );
  set("2024-03-11");
  click(cell("2024-03-11"));
  check("after max never selects", changed() == before);
  check(
    "max month next disabled",
    attr("#cal-next-month", "aria-disabled") == Some("true"),
  );
  set("2024-02-29");
  focus(node("#cal-previous-month"));
  click("#cal-previous-month");
  check(
    "browse clamps and retains button focus",
    current() == "2024-01-29" && has_focus("#cal-previous-month"),
  );
  let prior = current();
  click("#cal-previous-month");
  check("stale disabled browse guarded", current() == prior);
  click("#cal-next-month");
  check("next browse", current() == "2024-02-29");
  click("#cal-next-month");
  check("next browse second", current() == "2024-03-29");
  app.dispatch(Readonly(true));
  set("2024-02-01");
  check(
    "readonly aria and data",
    attr("#cal-grid", "aria-readonly") == Some("true")
    && attr("#cal-grid", "data-readonly") == Some(""),
  );
  click(cell("2024-02-02"));
  ignore(key(cell("2024-02-02"), "keydown", "Enter"));
  ignore(key(cell("2024-02-02"), "keydown", " "));
  frame();
  check(
    "readonly navigates without selection",
    current() == "2024-02-02" && changed() == before,
  );
  app.dispatch(Readonly(false));
  app.dispatch(
    Limits({
      ...C.unconstrained,
      is_unavailable: _ => true,
    }),
  );
  frame();
  one_stop();
  check(
    "all unavailable remains focused",
    has_focus(cell("2024-02-02"))
    && total("#cal-grid td[id]:not([aria-disabled])") == 0,
  );
  ignore(key(cell("2024-02-02"), "keydown", "ArrowRight"));
  frame();
  check("all unavailable still navigable", current() == "2024-02-03");
  click(cell("2024-02-03"));
  check("all unavailable cannot select", changed() == before);
  app.dispatch(
    Limits({
      ...C.unconstrained,
      min: Some(date("2024-02-29")),
      max: Some(date("2024-02-01")),
    }),
  );
  frame();
  check(
    "reversed bounds disable selection",
    total("#cal-grid td[id]:not([aria-disabled])") == 0,
  );
  one_stop();
  app.dispatch(
    Limits({
      ...C.unconstrained,
      min: Some(date("2024-02-29")),
      max: Some(date("2024-02-29")),
    }),
  );
  set("2024-02-29");
  check(
    "singleton selectable date",
    total("#cal-grid td[id]:not([aria-disabled])") == 1,
  );
  click(cell("2024-02-29"));
  check("singleton selects", selection() == Some("2024-02-29"));
  app.dispatch(Limits(C.unconstrained));
  set("2024-01-31");
  List.iter(
    _ =>
      ignore(
        dispatch(
          node("#cal-next-month"),
          event(
            "click",
            {
              "bubbles": true,
              "cancelable": true,
            },
          ),
        ),
      ),
    [(), ()],
  );
  frame();
  check(
    "batched browse uses latest focus and clamping",
    current() == "2024-03-29",
  );
  app.dispatch(
    Reinit(
      C.init(
        ~id="cal",
        ~focus=initial_date,
        ~first_day_of_week=0,
        ~dir=C.Rtl,
        ~fixed_weeks=false,
        ~show_outside_month=true,
        (),
      ),
    ),
  );
  frame();
  check("RTL attribute", attr("#cal", "dir") == Some("rtl"));
  check("variable weeks", total("#cal-grid tbody tr") == 5);
  check("visible outside", total("#cal-grid [data-outside-month]") == 4);
  ignore(key(cell("2024-01-31"), "keydown", "ArrowLeft"));
  frame();
  check("RTL left goes forward", current() == "2024-02-01");
  ignore(key(cell("2024-02-01"), "keydown", "ArrowRight"));
  frame();
  check("RTL right goes back", current() == "2024-01-31");
  ignore(key(cell("2024-01-31"), "keydown", "Home"));
  frame();
  check("Sunday-first Home", current() == "2024-01-28");
  ignore(key(cell("2024-01-28"), "keydown", "End"));
  frame();
  check("Sunday-first End", current() == "2024-02-03");
  set("2024-01-31");
  click(cell("2024-02-01"));
  check(
    "outside pointer selects changes month",
    selection() == Some("2024-02-01")
    && text(node("#cal-heading")) == "February 2024"
    && has_focus(cell("2024-02-01")),
  );
  app.dispatch(Reinit(C.init(~id="cal", ~focus=initial_date, ())));
  frame();
  ignore(key(cell("2024-01-31"), "keydown", "ArrowRight"));
  let before = changed();
  /* Cell remains in stale DOM but is not in latest visible month's hidden-outside grid. */
  ignore(
    dispatch(
      node(cell("2024-01-31")),
      event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
        },
      ),
    ),
  );
  frame();
  check(
    "stale hidden cell click ignored",
    current() == "2024-02-01" && changed() == before,
  );
  List.iter(
    bound => {
      set(bound);
      one_stop();
      check("domain edge complete month", total("#cal-grid td[id]") == 31);
    },
    ["0001-01-01", "9999-12-31"],
  );
  check(
    "upper bound button",
    attr("#cal-next-month", "aria-disabled") == Some("true")
    && attr("#cal-next-year", "aria-disabled") == Some("true"),
  );
  set("0001-01-01");
  check(
    "lower bound button",
    attr("#cal-previous-month", "aria-disabled") == Some("true")
    && attr("#cal-previous-year", "aria-disabled") == Some("true"),
  );
  let before = app.model().outs;
  app.dispatch(
    Labels({
      C.month: _ => "Month",
      weekday: _ => "Weekday",
      weekday_short: _ => "Wd",
      previous_month: "Back a month",
      next_month: "Forward a month",
      previous_year: "Back a year",
      next_year: "Forward a year",
      date: d => string_of_int(D.day(d)) ++ " localized date",
    }),
  );
  frame();
  check("localized month", text(node("#cal-heading")) == "Month 1");
  check(
    "localized weekday",
    text(node("#cal-grid th")) == "Wd"
    && attr("#cal-grid th", "abbr") == Some("Weekday"),
  );
  check(
    "localized buttons",
    attr("#cal-next-month", "aria-label") == Some("Forward a month")
    && attr("#cal-previous-month", "aria-label") == Some("Back a month")
    && attr("#cal-next-year", "aria-label") == Some("Forward a year")
    && attr("#cal-previous-year", "aria-label") == Some("Back a year"),
  );
  check(
    "localized date",
    attr(cell("0001-01-01"), "aria-label") == Some("1 localized date"),
  );
  check("labels do not emit outs", app.model().outs == before);
  focus(node(cell("0001-01-01")));
  ignore(
    dispatch(
      node(cell("0001-01-01")),
      event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
        },
      ),
    ),
  );
  focus(node("#cal-next-month"));
  frame();
  check(
    "cell selection never overrides a later navigation-button focus",
    has_focus("#cal-next-month"),
  );
  focus(node(cell("0001-01-01")));
  ignore(key(cell("0001-01-01"), "keydown", "ArrowRight"));
  focus(node("#cal-next-month"));
  frame();
  check(
    "arrow focus yields to intervening navigation",
    has_focus("#cal-next-month"),
  );
  ignore(key(cell("0001-01-02"), "keydown", "ArrowRight"));
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("calendar DOM");
  Js.Promise.resolve();
};
