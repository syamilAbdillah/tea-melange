module D = Tea_ui.Calendar.Date;
let count = ref(0);
let check = (name, condition) => {
  incr(count);
  if (!condition) {
    failwith(name);
  };
};
let date = s =>
  switch (D.of_iso(s)) {
  | Some(d) => d
  | None => failwith(s)
  };
let iso = (s, operation, expected) =>
  check(s ++ " -> " ++ expected, D.to_iso(operation(date(s))) == expected);
let () = {
  List.iter(
    s => check("strict invalid ISO: " ++ s, D.of_iso(s) == None),
    [
      "",
      "2024-1-01",
      "2024-01-1",
      "2024/01/01",
      " 2024-01-01",
      "2024-01-01 ",
      "2024-01-01Z",
      "2024-+1-01",
      "202a-01-01",
      "0000-01-01",
      "10000-01-01",
      "0001-00-01",
      "0001-13-01",
      "0001-01-00",
      "2023-02-29",
      "1900-02-29",
      "2100-02-29",
      "9999-12-32",
    ],
  );
  check(
    "invalid constructors",
    D.make(~year=0, ~month=1, ~day=1) == None
    && D.make(~year=10000, ~month=1, ~day=1) == None
    && D.make(~year=2024, ~month=2, ~day=30) == None,
  );
  check(
    "leap centuries",
    D.leap_year(2000)
    && !D.leap_year(1900)
    && !D.leap_year(2100)
    && D.leap_year(2400),
  );
  check(
    "invalid month length",
    D.days_in_month(~year=2024, ~month=0) == 0
    && D.days_in_month(~year=2024, ~month=13) == 0,
  );
  iso("2024-01-31", d => D.add_months(d, 1), "2024-02-29");
  iso("2023-01-31", d => D.add_months(d, 1), "2023-02-28");
  iso("2024-03-31", d => D.add_months(d, -1), "2024-02-29");
  iso("2024-02-29", d => D.add_years(d, 1), "2025-02-28");
  iso("2024-02-29", d => D.add_years(d, -4), "2020-02-29");
  iso("0001-01-01", d => D.add_days(d, -2147483647), "0001-01-01");
  iso("9999-12-31", d => D.add_days(d, 2147483647), "9999-12-31");
  iso("2024-01-31", d => D.add_months(d, 2147483647), "9999-12-31");
  iso("2024-01-31", d => D.add_months(d, -2147483647), "0001-01-31");
  iso("2024-02-29", d => D.add_years(d, 2147483647), "9999-02-28");
  iso("2024-02-29", d => D.add_years(d, -2147483647), "0001-02-28");
  check(
    "bounds order",
    D.compare(D.first, D.last) < 0
    && D.compare(D.last, D.first) > 0
    && D.equal(D.first, date("0001-01-01")),
  );
  /* Entire supported domain: all valid dates, strict parse/print, weekday
     continuity and next-day arithmetic. No JS Date reference or timezone. */
  let serial = ref(0);
  let prior = ref(D.first);
  for (year in 1 to 9999) {
    for (month in 1 to 12) {
      let length = D.days_in_month(~year, ~month);
      check(
        "day beyond month invalid",
        D.make(~year, ~month, ~day=length + 1) == None,
      );
      for (day in 1 to length) {
        let d = Option.get(D.make(~year, ~month, ~day));
        check(
          "accessors",
          D.year(d) == year && D.month(d) == month && D.day(d) == day,
        );
        check(
          "full-domain ISO round trip",
          switch (D.of_iso(D.to_iso(d))) {
          | Some(parsed) => D.equal(d, parsed)
          | None => false
          },
        );
        check("weekday continuity", D.day_of_week(d) == (serial^ + 1) mod 7);
        if (serial^ > 0) {
          check("full-domain next day", D.equal(D.add_days(prior^, 1), d));
          check(
            "full-domain previous day",
            D.equal(D.add_days(d, -1), prior^),
          );
          check("ordering", D.compare(prior^, d) < 0);
        };
        prior := d;
        incr(serial);
      };
      let d = Option.get(D.make(~year, ~month, ~day=length));
      List.iter(
        amount => {
          let next = D.add_months(d, amount);
          let target =
            max(0, min(119987, (year - 1) * 12 + month - 1 + amount));
          check(
            "month target",
            D.year(next) == target
            / 12
            + 1
            && D.month(next) == target
            mod 12
            + 1,
          );
          check(
            "month clamp",
            D.day(next)
            == min(
                 length,
                 D.days_in_month(~year=D.year(next), ~month=D.month(next)),
               ),
          );
        },
        [(-25), (-13), (-12), (-1), 0, 1, 12, 13, 25],
      );
      List.iter(
        amount => {
          let next = D.add_years(d, amount);
          check(
            "year target",
            D.year(next) == max(1, min(9999, year + amount))
            && D.month(next) == month,
          );
          check(
            "year clamp",
            D.day(next)
            == min(length, D.days_in_month(~year=D.year(next), ~month)),
          );
        },
        [(-400), (-100), (-4), (-1), 0, 1, 4, 100, 400],
      );
    };
  };
  check("domain length", serial^ == 3652059);
  /* A complete Gregorian 400-year cycle covers every leap/weekday layout.
     Check all week starts, fixed/variable grids, contiguous cells and bounds. */
  for (year in 1 to 400) {
    for (month in 1 to 12) {
      let d = Option.get(D.make(~year, ~month, ~day=1));
      for (first in 0 to 6) {
        List.iter(
          fixed => {
            let grid =
              D.month_grid(~first_day_of_week=first, ~fixed_weeks=fixed, d);
            let weeks = Array.length(grid);
            check(
              "week count",
              fixed ? weeks == 6 : weeks >= 4 && weeks <= 6,
            );
            let dates = ref(0);
            let previous = ref(None);
            Array.iter(
              row => {
                check("seven columns", Array.length(row) == 7);
                Array.iteri(
                  (col, cell) =>
                    switch (cell) {
                    | None =>
                      check(
                        "only lower domain padding",
                        year == 1 && month == 1,
                      )
                    | Some(date) =>
                      check(
                        "weekday column",
                        D.day_of_week(date) == (first + col) mod 7,
                      );
                      if (D.year(date) == year && D.month(date) == month) {
                        incr(dates);
                      };
                      switch (previous^) {
                      | Some(prior) =>
                        check(
                          "grid continuity",
                          D.equal(D.add_days(prior, 1), date),
                        )
                      | None => ()
                      };
                      previous := Some(date);
                    },
                  row,
                );
              },
              grid,
            );
            check(
              "every day exactly once",
              dates^ == D.days_in_month(~year, ~month),
            );
          },
          [false, true],
        );
      };
    };
  };
  List.iter(
    d => {
      for (first in (-7) to 13) {
        let grid = D.month_grid(~first_day_of_week=first, d);
        let seen = ref(0);
        Array.iter(
          row =>
            Array.iter(
              cell =>
                switch (cell) {
                | None => ()
                | Some(date) =>
                  check(
                    "bounded grid",
                    D.compare(date, D.first) >= 0
                    && D.compare(date, D.last) <= 0,
                  );
                  incr(seen);
                },
              row,
            ),
          grid,
        );
        check("boundary month complete", seen^ >= 31);
      }
    },
    [D.first, D.last],
  );
  /* Dates around US/EU DST transitions remain ordinary civil increments. */
  List.iter(
    s => {
      let d = date(s);
      check(
        "DST irrelevant",
        D.equal(D.add_days(D.add_days(d, 1), -1), d),
      );
    },
    ["2024-03-10", "2024-03-31", "2024-10-27", "2024-11-03"],
  );
  print_endline(
    "calendar dates: "
    ++ string_of_int(count^)
    ++ " assertions passed across "
    ++ string_of_int(serial^)
    ++ " dates",
  );
};
