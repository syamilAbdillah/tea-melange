module N = Ui_nav;
module T = Ui_typeahead;
module A = Ui_attr;
module K = Ui_keyboard;
module D = Tea_core.Decode;
module V = Tea_vdom.Vdom;
let assertions = ref(0);
let check = (label, pass) => {
  incr(assertions);
  if (!pass) {
    failwith(label);
  };
};
let navigation = () => {
  for (n in 0 to 8) {
    for (mask in 0 to 1 lsl n - 1) {
      let disabled = Array.init(n, i => mask land 1 lsl i != 0);
      let enabled = List.filter(i => !disabled[i], List.init(n, i => i));
      let head = values =>
        switch (values) {
        | [] => None
        | [v, ..._] => Some(v)
        };
      check("first", N.first(disabled) == head(enabled));
      check("last", N.last(disabled) == head(List.rev(enabled)));
      for (from in (-2) to n + 1) {
        let valid = from >= 0 && from < n && !disabled[from];
        check(
          "repair",
          N.repair(~from, disabled) == (valid ? Some(from) : head(enabled)),
        );
        List.iter(
          forward => {
            let ordered = forward ? enabled : List.rev(enabled);
            let start = from < 0 || from >= n ? forward ? (-1) : n : from;
            let beyond =
              List.filter(i => forward ? i > start : i < start, ordered);
            List.iter(
              wrap => {
                let expected =
                  switch (head(beyond)) {
                  | Some(_) as found => found
                  | None => wrap ? head(ordered) : valid ? Some(from) : None
                  };
                let actual =
                  forward
                    ? N.next(~wrap, ~from, disabled)
                    : N.prev(~wrap, ~from, disabled);
                check("exhaustive next/prev", actual == expected);
              },
              [true, false],
            );
            List.iter(
              size => {
                let at = max(0, min(n - 1, from));
                let jump = max(1, min(n, size));
                let target =
                  forward ? min(n - 1, at + jump) : max(0, at - jump);
                let past =
                  List.filter(
                    i => forward ? i >= target : i <= target,
                    ordered,
                  );
                let expected =
                  switch (head(past)) {
                  | Some(_) as found => found
                  | None => head(List.rev(ordered))
                  };
                check(
                  "exhaustive page",
                  N.page(~size, ~forward, ~from, disabled) == expected,
                );
              },
              [(-1), 0, 1, 2, n, max_int],
            );
          },
          [true, false],
        );
      };
    };
  };
  List.iter(
    ((key, ltr, rtl)) => {
      check("horizontal LTR", N.horizontal(~dir=N.Ltr, key) == ltr);
      check("horizontal RTL", N.horizontal(~dir=N.Rtl, key) == rtl);
    },
    [
      ("ArrowLeft", Some(-1), Some(1)),
      ("ArrowRight", Some(1), Some(-1)),
      ("ArrowUp", None, None),
      ("Home", None, None),
    ],
  );
};
let typeahead = () => {
  let labels = [|
    "Apple",
    "apricot",
    "Àlmond",
    "Banana",
    "Blueberry",
    "Cherry",
  |];
  let disabled = [|false, true, false, false, false, false|];
  let feed = (~time_stamp, ~key, ~from, state) =>
    T.feed(~time_stamp, ~key, ~from, ~labels, ~disabled, state);
  let (state, found) = feed(~time_stamp=0., ~key="A", ~from=-1, T.empty);
  check(
    "case insensitive first",
    found == Some(0) && T.buffer(state) == "a",
  );
  let (state, found) = feed(~time_stamp=1., ~key="a", ~from=0, state);
  check("repeat cycles/skips disabled/diacritics", found == Some(2));
  let (state, found) = feed(~time_stamp=2., ~key="a", ~from=2, state);
  check("repeat wraps", found == Some(0));
  let (state, found) = feed(~time_stamp=3., ~key="p", ~from=0, state);
  check(
    "growing prefix retains match",
    found == Some(0) && T.buffer(state) == "ap",
  );
  let (state, found) = feed(~time_stamp=4., ~key="b", ~from=0, state);
  check(
    "failed prefix retries character",
    found == Some(3) && T.buffer(state) == "b",
  );
  let (state, found) = feed(~time_stamp=503., ~key="l", ~from=3, state);
  check(
    "499ms retains buffer",
    found == Some(4) && T.buffer(state) == "bl",
  );
  let (state, found) = feed(~time_stamp=1003., ~key="c", ~from=4, state);
  check("500ms resets", found == Some(5) && T.buffer(state) == "c");
  let (state, found) = feed(~time_stamp=1., ~key="a", ~from=5, state);
  check("backwards timestamp resets", found == Some(0));
  List.iter(
    key => {
      let (next, found) = feed(~time_stamp=2., ~key, ~from=0, state);
      check(
        "noncharacters ignored",
        found == None
        && T.buffer(next) == T.buffer(state)
        && T.last_time_stamp(next) == T.last_time_stamp(state),
      );
    },
    ["", "ArrowDown", "Enter", "Dead", "ab"],
  );
  let (state, found) = feed(~time_stamp=2., ~key="z", ~from=0, state);
  check(
    "unmatched buffered",
    found == None
    && T.buffer(state) == "z"
    && T.last_time_stamp(state) == Some(2.),
  );
  List.iter(
    ((source, expected)) =>
      check("Latin-1 folds", T.normalize(source) == expected),
    [
      ("ÀÁÂÃÄÅàáâãäå", "aaaaaaaaaaaa"),
      ("ÆæÇç", "aeaecc"),
      ("ÈÉÊËèéêë", "eeeeeeee"),
      ("ÌÍÎÏìíîï", "iiiiiiii"),
      ("ÐðÑñ", "ddnn"),
      ("ÒÓÔÕÖØòóôõöø", "oooooooooooo"),
      ("ÙÚÛÜùúûü", "uuuuuuuu"),
      ("ÝýÿÞþß", "yyyththss"),
      ("CAFÉ", "cafe"),
      ("東京", "東京"),
      ("é", "é"),
    ],
  );
  for (n in 0 to 6) {
    for (mask in 0 to 1 lsl n - 1) {
      let disabled = Array.init(n, i => mask land 1 lsl i != 0);
      let labels = Array.init(n, i => i mod 2 == 0 ? "Apple" : "Banana");
      for (from in (-1) to n) {
        List.iter(
          key => {
            let (_, found) =
              T.feed(
                ~time_stamp=0.,
                ~key,
                ~from,
                ~labels,
                ~disabled,
                T.empty,
              );
            let candidates =
              List.init(n, offset => (max(0, from + 1) + offset) mod n);
            let candidates = from + 1 >= n ? List.init(n, i => i) : candidates;
            let expected =
              List.find_opt(
                i =>
                  !disabled[i]
                  && String.starts_with(
                       ~prefix=String.lowercase_ascii(key),
                       String.lowercase_ascii(labels[i]),
                     ),
                candidates,
              );
            check("exhaustive typeahead", found == expected);
          },
          ["a", "b", "z"],
        );
      };
    };
  };
  let (_, found) =
    T.feed(
      ~time_stamp=0.,
      ~key="a",
      ~from=-1,
      ~labels=[|"Apple"|],
      ~disabled=[||],
      T.empty,
    );
  check("missing flags disabled", found == None);
  let (state, _) =
    T.feed(
      ~timeout=10.,
      ~time_stamp=0.,
      ~key="a",
      ~from=-1,
      ~labels,
      ~disabled,
      T.empty,
    );
  let (state, _) =
    T.feed(
      ~timeout=10.,
      ~time_stamp=10.,
      ~key="c",
      ~from=0,
      ~labels,
      ~disabled,
      state,
    );
  check("custom timeout", T.buffer(state) == "c");
  let (_, emoji) =
    T.feed(
      ~time_stamp=0.,
      ~key="🍵",
      ~from=-1,
      ~labels=[|"🍵 tea"|],
      ~disabled=[|false|],
      T.empty,
    );
  check("Unicode scalar key", emoji == Some(0));
};
let attributes = () => {
  check(
    "true presence flag",
    A.data_flag("open", true) == V.Attribute("data-open", ""),
  );
  check("false absence", A.data_flag("open", false) == Tea_html.Attr.none);
  check(
    "ARIA bool",
    A.aria_bool("checked", false) == V.Attribute("aria-checked", "false")
    && A.aria_bool("checked", true) == V.Attribute("aria-checked", "true"),
  );
  check(
    "ARIA empty ids",
    A.aria_ids("labelledby", ["", ""]) == Tea_html.Attr.none,
  );
  check(
    "ARIA ids",
    A.aria_ids("labelledby", ["label", "", "help"])
    == V.Attribute("aria-labelledby", "label help"),
  );
  check("part id", A.part_id("menu", "button") == "menu-button");
  let keys = ["", "_", "_0020", " ", "a-b", "a_b", "a/b", "é", "🍵"];
  List.iter(
    key =>
      List.iter(
        other =>
          check(
            "collision-free key ids",
            String.equal(key, other)
            || !String.equal(A.safe_key(key), A.safe_key(other)),
          ),
        keys,
      ),
    keys,
  );
  check("item id", A.item_id("menu", "tea") == "menu-item-tea");
  for (code in 0 to 255) {
    let encoded = A.safe_key(String.make(1, Char.chr(code)));
    check(
      "safe alphabet",
      String.for_all(
        c =>
          switch (c) {
          | 'A' .. 'Z'
          | 'a' .. 'z'
          | '0' .. '9'
          | '_'
          | '-' => true
          | _ => false
          },
        encoded,
      ),
    );
  };
};
let keyboard = () => {
  for (mask in 0 to 127) {
    let event: K.event = {
      key: mask land 64 != 0 ? "Tab" : "Enter",
      shift: mask land 1 != 0,
      ctrl: mask land 2 != 0,
      alt: mask land 4 != 0,
      meta: mask land 8 != 0,
      composing: mask land 16 != 0,
      repeat: mask land 32 != 0,
      time_stamp: 42.,
    };
    let available = mask land (2 lor 4 lor 8 lor 16 lor 64) == 0;
    check(
      "protected keyboard combinations",
      K.available(event) == available,
    );
    let value =
      D.Value.Object([
        ("key", D.Value.String(event.key)),
        ("timeStamp", D.Value.Number(42.)),
        ("shiftKey", D.Value.Bool(event.shift)),
        ("ctrlKey", D.Value.Bool(event.ctrl)),
        ("altKey", D.Value.Bool(event.alt)),
        ("metaKey", D.Value.Bool(event.meta)),
        ("isComposing", D.Value.Bool(event.composing)),
        ("repeat", D.Value.Bool(event.repeat)),
      ]);
    check("all keyboard fields", D.run(K.decoder, value) == Ok(event));
    let handled = D.run(K.custom(_ => Some((None, true))), value);
    check(
      "prevent only handled safe keys",
      switch (handled) {
      | Ok(result) =>
        available
        && result.prevent_default
        && result.message == None
        && !result.stop_propagation
      | Error(_) => !available
      },
    );
    check(
      "unhandled rejected",
      Result.is_error(D.run(K.custom(_ => None), value)),
    );
  };
  check(
    "legacy composition",
    switch (
      D.run(
        K.decoder,
        D.Value.Object([
          ("key", D.Value.String("Enter")),
          ("timeStamp", D.Value.Number(1.)),
          ("keyCode", D.Value.Number(229.)),
        ]),
      )
    ) {
    | Ok(e) => e.composing
    | Error(_) => false
    },
  );
};
let () = {
  navigation();
  typeahead();
  attributes();
  keyboard();
  print_endline(
    "UI helpers: " ++ string_of_int(assertions^) ++ " assertions passed",
  );
};
