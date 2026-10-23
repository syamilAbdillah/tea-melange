module Html = Tea_html.Html;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Date = {
  type t = {
    year: int,
    month: int,
    day: int,
  };
  let year = d => d.year;
  let month = d => d.month;
  let day = d => d.day;
  let leap_year = year =>
    year mod 4 == 0 && (year mod 100 != 0 || year mod 400 == 0);
  let days_in_month = (~year, ~month) =>
    switch (month) {
    | 2 => leap_year(year) ? 29 : 28
    | 4
    | 6
    | 9
    | 11 => 30
    | 1
    | 3
    | 5
    | 7
    | 8
    | 10
    | 12 => 31
    | _ => 0
    };
  let make = (~year, ~month, ~day) =>
    year >= 1
    && year <= 9999
    && month >= 1
    && month <= 12
    && day >= 1
    && day <= days_in_month(~year, ~month)
      ? Some({
          year,
          month,
          day,
        })
      : None;
  let first = {
    year: 1,
    month: 1,
    day: 1,
  };
  let last = {
    year: 9999,
    month: 12,
    day: 31,
  };
  let before_year = year => {
    let y = year - 1;
    365 * y + y / 4 - y / 100 + y / 400;
  };
  let ordinal = d => {
    let n = ref(before_year(d.year) + d.day - 1);
    for (m in 1 to d.month - 1) {
      n := n^ + days_in_month(~year=d.year, ~month=m);
    };
    n^;
  };
  let compare = (a, b) => Int.compare(ordinal(a), ordinal(b));
  let equal = (a, b) => compare(a, b) == 0;
  let from_ordinal = n => {
    let lo = ref(1);
    let hi = ref(9999);
    while (lo^ < hi^) {
      let mid = (lo^ + hi^ + 1) / 2;
      if (before_year(mid) <= n) {
        lo := mid;
      } else {
        hi := mid - 1;
      };
    };
    let month = ref(1);
    let remaining = ref(n - before_year(lo^) + 1);
    while (remaining^ > days_in_month(~year=lo^, ~month=month^)) {
      remaining := remaining^ - days_in_month(~year=lo^, ~month=month^);
      incr(month);
    };
    {
      year: lo^,
      month: month^,
      day: remaining^,
    };
  };
  let clamp = (lo, hi, n) => max(lo, min(hi, n));
  let add_days = (d, amount) => {
    let n = ordinal(d);
    from_ordinal(n + clamp(- n, ordinal(last) - n, amount));
  };
  let add_months = (d, amount) => {
    let n = (d.year - 1) * 12 + d.month - 1;
    let target = n + clamp(- n, 119987 - n, amount);
    let year = target / 12 + 1;
    let month = target mod 12 + 1;
    {
      year,
      month,
      day: min(d.day, days_in_month(~year, ~month)),
    };
  };
  let add_years = (d, amount) =>
    add_months(d, clamp(1 - d.year, 9999 - d.year, amount) * 12);
  let day_of_week = d => (ordinal(d) + 1) mod 7;
  let pad = (width, n) => {
    let s = string_of_int(n);
    String.make(max(0, width - String.length(s)), '0') ++ s;
  };
  let to_iso = d =>
    pad(4, d.year) ++ "-" ++ pad(2, d.month) ++ "-" ++ pad(2, d.day);
  let of_iso = s =>
    if (String.length(s) != 10 || s.[4] != '-' || s.[7] != '-') {
      None;
    } else {
      let valid = ref(true);
      let number = (start, length) => {
        let result = ref(0);
        for (i in start to start + length - 1) {
          let c = Char.code(s.[i]) - 48;
          if (c < 0 || c > 9) {
            valid := false;
          };
          result := result^ * 10 + c;
        };
        result^;
      };
      let year = number(0, 4);
      let month = number(5, 2);
      let day = number(8, 2);
      valid^ ? make(~year, ~month, ~day) : None;
    };
  let month_grid = (~first_day_of_week=0, ~fixed_weeks=true, d) => {
    let first = {
      ...d,
      day: 1,
    };
    let weekday = (first_day_of_week mod 7 + 7) mod 7;
    let offset = (day_of_week(first) - weekday + 7) mod 7;
    let count =
      fixed_weeks
        ? 6 : (offset + days_in_month(~year=d.year, ~month=d.month) + 6) / 7;
    let start = ordinal(first) - offset;
    Array.init(count, row =>
      Array.init(
        7,
        col => {
          let n = start + row * 7 + col;
          n < 0 || n > ordinal(last) ? None : Some(from_ordinal(n));
        },
      )
    );
  };
};
type date = Date.t;
type direction =
  | Ltr
  | Rtl;
type constraints = {
  min: option(date),
  max: option(date),
  is_unavailable: date => bool,
};
let unconstrained = {
  min: None,
  max: None,
  is_unavailable: _ => false,
};
type labels = {
  month: int => string,
  weekday: int => string,
  weekday_short: int => string,
  previous_month: string,
  next_month: string,
  previous_year: string,
  next_year: string,
  date: date => string,
};
let english_month = m => [|
                           "January",
                           "February",
                           "March",
                           "April",
                           "May",
                           "June",
                           "July",
                           "August",
                           "September",
                           "October",
                           "November",
                           "December",
                         |][max(0, min(11, m - 1))];
let english = {
  month: english_month,
  weekday: d => [|
                  "Sunday",
                  "Monday",
                  "Tuesday",
                  "Wednesday",
                  "Thursday",
                  "Friday",
                  "Saturday",
                |][max(0, min(6, d))],
  weekday_short: d => [|"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"|][max(
                                                                    0,
                                                                    min(6, d),
                                                                    )],
  previous_month: "Previous month",
  next_month: "Next month",
  previous_year: "Previous year",
  next_year: "Next year",
  date: d =>
    string_of_int(d.Date.day)
    ++ " "
    ++ english_month(d.month)
    ++ " "
    ++ string_of_int(d.year),
};
type model = {
  id: string,
  focused: date,
  selected: option(date),
  first_day_of_week: int,
  dir: direction,
  fixed_weeks: bool,
  show_outside_month: bool,
  focus: Ui_focus.t,
};
type msg =
  | Key(Ui_keyboard.event)
  | Select(date)
  | Browse(int)
  | Request_focus
  | Focus_finished(int);
type out_msg =
  | Changed(date)
  | Focused(date)
  | Month_changed(date);
let init =
    (
      ~id,
      ~focus,
      ~selected=?,
      ~first_day_of_week=0,
      ~dir=Ltr,
      ~fixed_weeks=true,
      ~show_outside_month=false,
      (),
    ) => {
  id,
  focused: focus,
  selected,
  first_day_of_week: (first_day_of_week mod 7 + 7) mod 7,
  dir,
  fixed_weeks,
  show_outside_month,
  focus: Ui_focus.none,
};
let focus_date = m => m.focused;
let selected = m => m.selected;
let visible_month = m => {
  ...m.focused,
  Date.day: 1,
};
let set_selected = (selected, m) => {
  ...m,
  selected,
};
let set_focus = (focused, m) => {
  ...m,
  focused,
  focus: Ui_focus.cancel(m.focus),
};
let request_focus = m => {
  ...m,
  focus:
    Ui_focus.element(
      m.focus,
      Ui_attr.item_id(m.id, Date.to_iso(m.focused)),
    ),
};
let focus_grid = _ => Cmd.msg(Request_focus);
let subscriptions = m =>
  Ui_focus.sub(
    ~home="Calendar",
    ~id=m.id,
    ~prevent_scroll=false,
    m.focus,
    (generation, _) =>
    Focus_finished(generation)
  );
let same_month = (a, b) => a.Date.year == b.Date.year && a.month == b.month;
let allowed = (constraints, date) =>
  (
    switch (constraints.min) {
    | None => true
    | Some(min) => Date.compare(date, min) >= 0
    }
  )
  && (
    switch (constraints.max) {
    | None => true
    | Some(max) => Date.compare(date, max) <= 0
    }
  )
  && !constraints.is_unavailable(date);
let browse_allowed = (constraints, amount, m) => {
  let target = Date.add_months(m.focused, amount);
  !same_month(target, m.focused)
  && (
    switch (constraints.min) {
    | None => true
    | Some(min) =>
      amount > 0
      || Date.compare(
           {
             ...target,
             Date.day:
               Date.days_in_month(~year=target.year, ~month=target.month),
           },
           min,
         )
      >= 0
    }
  )
  && (
    switch (constraints.max) {
    | None => true
    | Some(max) =>
      amount < 0
      || Date.compare(
           {
             ...target,
             Date.day: 1,
           },
           max,
         )
      <= 0
    }
  );
};
let focus_outputs = (old, next) =>
  (Date.equal(old.focused, next.focused) ? [] : [Focused(next.focused)])
  @ (
    same_month(old.focused, next.focused)
      ? [] : [Month_changed(visible_month(next))]
  );
let update = (~constraints=unconstrained, ~readonly=false, msg, m) => {
  let move = (date, focus) => {
    let next = {
      ...m,
      focused: date,
    };
    (
      focus
        ? request_focus(next)
        : {
          ...next,
          focus: Ui_focus.cancel(next.focus),
        },
      Cmd.none,
      focus_outputs(m, next),
    );
  };
  let select = date => {
    let next = {
      ...m,
      focused: date,
    };
    if (!readonly && allowed(constraints, date)) {
      let next = {
        ...next,
        selected: Some(date),
      };
      (
        request_focus(next),
        Cmd.none,
        focus_outputs(m, next) @ [Changed(date)],
      );
    } else {
      (request_focus(next), Cmd.none, focus_outputs(m, next));
    };
  };
  switch (msg) {
  | Request_focus => (request_focus(m), Cmd.none, [])
  | Focus_finished(generation) => (
      {
        ...m,
        focus: fst(Ui_focus.finish(m.focus, generation)),
      },
      Cmd.none,
      [],
    )
  | Select(date) =>
    /* Ignore a stale click on a date that is no longer rendered. */
    let grid =
      Date.month_grid(
        ~first_day_of_week=m.first_day_of_week,
        ~fixed_weeks=m.fixed_weeks,
        m.focused,
      );
    let rendered =
      Array.exists(
        row =>
          Array.exists(
            cell =>
              switch (cell) {
              | Some(d) =>
                Date.equal(d, date)
                && (m.show_outside_month || same_month(d, m.focused))
              | None => false
              },
            row,
          ),
        grid,
      );
    rendered ? select(date) : (m, Cmd.none, []);
  | Browse(amount) =>
    browse_allowed(constraints, amount, m)
      ? move(Date.add_months(m.focused, amount), false) : (m, Cmd.none, [])
  | Key(event) =>
    if (!Ui_keyboard.plain(event)) {
      (m, Cmd.none, []);
    } else {
      let week_offset =
        (Date.day_of_week(m.focused) - m.first_day_of_week + 7) mod 7;
      switch (event.key) {
      | "Enter"
      | " " => select(m.focused)
      | "ArrowUp" => move(Date.add_days(m.focused, -7), true)
      | "ArrowDown" => move(Date.add_days(m.focused, 7), true)
      | "Home" => move(Date.add_days(m.focused, - week_offset), true)
      | "End" => move(Date.add_days(m.focused, 6 - week_offset), true)
      | "PageUp" =>
        move(
          event.shift
            ? Date.add_years(m.focused, -1) : Date.add_months(m.focused, -1),
          true,
        )
      | "PageDown" =>
        move(
          event.shift
            ? Date.add_years(m.focused, 1) : Date.add_months(m.focused, 1),
          true,
        )
      | key =>
        switch (
          Ui_nav.horizontal(~dir=m.dir == Rtl ? Ui_nav.Rtl : Ui_nav.Ltr, key)
        ) {
        | Some(amount) => move(Date.add_days(m.focused, amount), true)
        | None => (m, Cmd.none, [])
        }
      };
    }
  };
};
type cell('msg) = {
  date: option(date),
  text: string,
  attrs: list(Html.attr('msg)),
};
type weekday('msg) = {
  text: string,
  attrs: list(Html.attr('msg)),
};
type parts('msg) = {
  root: list(Html.attr('msg)),
  heading: list(Html.attr('msg)),
  heading_text: string,
  grid: list(Html.attr('msg)),
  previous_month: list(Html.attr('msg)),
  next_month: list(Html.attr('msg)),
  previous_year: list(Html.attr('msg)),
  next_year: list(Html.attr('msg)),
  weekdays: array(weekday('msg)),
  weeks: array(array(cell('msg))),
};
let view =
    (
      ~model as m,
      ~constraints=unconstrained,
      ~readonly=false,
      ~today=?,
      ~labels=english,
      ~on_key_down=_ => None,
      ~to_parent,
      ~to_view,
      (),
    ) => {
  let map = attrs => List.map(Tea_vdom.Vdom.map_fact(to_parent), attrs);
  let key =
    Html.on(
      "keydown",
      Tea_vdom.Vdom.Custom_optional(
        Ui_keyboard.custom(event =>
          if (!Ui_keyboard.plain(event)) {
            None;
          } else {
            switch (event.key) {
            | "ArrowLeft"
            | "ArrowRight"
            | "ArrowUp"
            | "ArrowDown"
            | "Home"
            | "End"
            | "PageUp"
            | "PageDown"
            | "Enter"
            | " " => Some((Some(to_parent(Key(event))), true))
            | key =>
              Option.map(msg => (Some(msg), false), on_key_down(key))
            };
          }
        ),
      ),
    );
  let button = (part, label, amount) => {
    let disabled = !browse_allowed(constraints, amount, m);
    map([
      Html.id(Ui_attr.part_id(m.id, part)),
      Html.type_("button"),
      Html.aria("label", label),
      disabled ? Ui_attr.aria_bool("disabled", true) : Html.none,
      Html.tabindex(disabled ? (-1) : 0),
      Ui_attr.data_flag("disabled", disabled),
      Html.on(
        "click",
        Tea_vdom.Vdom.Custom_optional(
          Tea_core.Decode.succeed({
            Tea_vdom.Vdom.message: disabled ? None : Some(Browse(amount)),
            prevent_default: disabled,
            stop_propagation: false,
          }),
        ),
      ),
    ]);
  };
  to_view({
    root: [
      Html.id(m.id),
      Html.attribute("dir", m.dir == Rtl ? "rtl" : "ltr"),
    ],
    heading: [
      Html.id(Ui_attr.part_id(m.id, "heading")),
      Html.aria("live", "polite"),
      Html.aria("atomic", "true"),
    ],
    heading_text:
      labels.month(m.focused.month) ++ " " ++ string_of_int(m.focused.year),
    grid: [
      Html.id(Ui_attr.part_id(m.id, "grid")),
      Html.role("grid"),
      Ui_attr.aria_ids("labelledby", [Ui_attr.part_id(m.id, "heading")]),
      Ui_attr.aria_bool("readonly", readonly),
      Ui_attr.data_flag("readonly", readonly),
    ],
    previous_month: button("previous-month", labels.previous_month, -1),
    next_month: button("next-month", labels.next_month, 1),
    previous_year: button("previous-year", labels.previous_year, -12),
    next_year: button("next-year", labels.next_year, 12),
    weekdays:
      Array.init(
        7,
        i => {
          let d = (m.first_day_of_week + i) mod 7;
          {
            text: labels.weekday_short(d),
            attrs: [
              Html.attribute("scope", "col"),
              Html.attribute("abbr", labels.weekday(d)),
              Html.aria("label", labels.weekday(d)),
            ],
          };
        },
      ),
    weeks:
      Date.month_grid(
        ~first_day_of_week=m.first_day_of_week,
        ~fixed_weeks=m.fixed_weeks,
        m.focused,
      )
      |> Array.map(row =>
           Array.map(
             date => {
               switch (date) {
               | Some(date)
                   when m.show_outside_month || same_month(date, m.focused) =>
                 let active = Date.equal(date, m.focused);
                 let selected =
                   switch (m.selected) {
                   | None => false
                   | Some(d) => Date.equal(date, d)
                   };
                 let current =
                   switch (today) {
                   | None => false
                   | Some(d) => Date.equal(date, d)
                   };
                 let disabled = !allowed(constraints, date);
                 {
                   date: Some(date),
                   text: string_of_int(date.day),
                   attrs:
                     map([
                       Html.id(Ui_attr.item_id(m.id, Date.to_iso(date))),
                       Html.role("gridcell"),
                       Html.tabindex(active ? 0 : (-1)),
                       Html.aria("label", labels.date(date)),
                       selected
                         ? Ui_attr.aria_bool("selected", true) : Html.none,
                       current ? Html.aria("current", "date") : Html.none,
                       disabled
                         ? Ui_attr.aria_bool("disabled", true) : Html.none,
                       Ui_attr.data_flag("active", active),
                       Ui_attr.data_flag("selected", selected),
                       Ui_attr.data_flag("today", current),
                       Ui_attr.data_flag("disabled", disabled),
                       Ui_attr.data_flag(
                         "outside-month",
                         !same_month(date, m.focused),
                       ),
                       Html.prevent_default_on(
                         "click",
                         Tea_core.Decode.succeed((Select(date), true)),
                       ),
                     ])
                     @ [key],
                 };
               | _ => {
                   date: None,
                   text: "",
                   attrs: [
                     Html.role("gridcell"),
                     Ui_attr.aria_bool("disabled", true),
                   ],
                 }
               }
             },
             row,
           )
         ),
  });
};
