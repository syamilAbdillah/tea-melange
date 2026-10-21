type key =
  | Attribute(string)
  | Attribute_ns(string, string)
  | Property(string)
  | Style(string)
  | Event(string);

type change('msg) =
  | Set(Vdom.fact('msg))
  | Remove(key);

let key =
  fun
  | Vdom.Attribute(k, _) => Attribute(k)
  | Vdom.Attribute_ns(ns, k, _) => Attribute_ns(ns, k)
  | Vdom.Property(k, _) => Property(k)
  | Vdom.Style(k, _) => Style(k)
  | Vdom.Event(k, _) => Event(k);

let same_value = (a: Vdom.prop_value, b: Vdom.prop_value) =>
  switch (a, b) {
  | (String(a), String(b)) => a == b
  | (Bool(a), Bool(b)) => a == b
  | (Float(a), Float(b)) => a == b
  | _ => false
  };
let same = (a, b) =>
  switch (a, b) {
  | (Vdom.Event(_, a), Vdom.Event(_, b)) => a === b
  | (Vdom.Attribute(_, a), Vdom.Attribute(_, b)) => a == b
  | (Vdom.Attribute_ns(_, _, a), Vdom.Attribute_ns(_, _, b)) => a == b
  | (Vdom.Property(_, a), Vdom.Property(_, b)) => same_value(a, b)
  | (Vdom.Style(_, a), Vdom.Style(_, b)) => a == b
  | _ => false
  };

/* Allocation-free key comparisons for the small-list paths. */
let same_key = (a, b) =>
  switch (a, b) {
  | (Vdom.Attribute(x, _), Vdom.Attribute(y, _))
  | (Vdom.Property(x, _), Vdom.Property(y, _))
  | (Vdom.Style(x, _), Vdom.Style(y, _))
  | (Vdom.Event(x, _), Vdom.Event(y, _)) => x == y
  | (Vdom.Attribute_ns(n, x, _), Vdom.Attribute_ns(m, y, _)) =>
    n == m && x == y
  | _ => false
  };
let has_key = (k, f) =>
  switch (k, f) {
  | (Attribute(x), Vdom.Attribute(y, _))
  | (Property(x), Vdom.Property(y, _))
  | (Style(x), Vdom.Style(y, _))
  | (Event(x), Vdom.Event(y, _)) => x == y
  | (Attribute_ns(n, x), Vdom.Attribute_ns(m, y, _)) => n == m && x == y
  | _ => false
  };
/* [f] is the last occurrence of its key when no later fact shares it. */
let rec exists_key = (f, facts) =>
  switch (facts) {
  | [] => false
  | [g, ...rest] => same_key(f, g) || exists_key(f, rest)
  };
let last = (f, rest) => !exists_key(f, rest);
/* Last occurrence of [f]'s key in [facts]. */
let rec find_last = (f, facts, found) =>
  switch (facts) {
  | [] => found
  | [g, ...rest] => find_last(f, rest, same_key(f, g) ? Some(g) : found)
  };

/* Fact lists are short; quadratic scans beat hashing below this size. */
let small = 32;

/* Large lists: a string table keyed by a namespaced encoding of each key.
   Last occurrence wins, as when setting DOM facts in order. */
let key_string =
  fun
  | Attribute(k) => "a" ++ k
  | Attribute_ns(ns, k) => "n" ++ ns ++ " " ++ k
  | Property(k) => "p" ++ k
  | Style(k) => "s" ++ k
  | Event(k) => "e" ++ k;
module T = Tea_core.String_table;
let table = facts => {
  let t = T.create(16);
  List.iter(f => T.replace(t, key_string(key(f)), f), facts);
  t;
};
let diff_large = (oldFacts, newFacts) => {
  let a = table(oldFacts);
  let b = table(newFacts);
  let out = ref([]);
  T.iter(
    (k, f) =>
      if (!T.mem(b, k)) {
        out := [Remove(key(f)), ...out^];
      },
    a,
  );
  T.iter(
    (k, v) =>
      switch (T.find_opt(a, k)) {
      | Some(old) when same(old, v) => ()
      | _ => out := [Set(v), ...out^]
      },
    b,
  );
  List.rev(out^);
};
/* Same keys in the same positions (the usual re-render): last occurrences
   coincide, so only changed last occurrences need setting. */
let rec aligned = (a, b) =>
  switch (a, b) {
  | ([], []) => true
  | ([x, ...a], [y, ...b]) => same_key(x, y) && aligned(a, b)
  | _ => false
  };
let rec diff_aligned = (a, b, out) =>
  switch (a, b) {
  | ([x, ...a], [y, ...b]) =>
    diff_aligned(
      a,
      b,
      x === y || same(x, y) || !last(y, b) ? out : [Set(y), ...out],
    )
  | _ => List.rev(out)
  };
let diff_small = (a, b) => {
  let rec removals = (rest, out) =>
    switch (rest) {
    | [] => out
    | [f, ...rest] =>
      removals(
        rest,
        last(f, rest) && !exists_key(f, b)
          ? [Remove(key(f)), ...out] : out,
      )
    };
  let rec sets = (rest, out) =>
    switch (rest) {
    | [] => out
    | [f, ...rest] =>
      let out =
        if (!last(f, rest)) {
          out;
        } else {
          switch (find_last(f, a, None)) {
          | Some(old) when same(old, f) => out
          | _ => [Set(f), ...out]
          };
        };
      sets(rest, out);
    };
  List.rev(sets(b, removals(a, [])));
};
let diff = (oldFacts, newFacts) =>
  if (oldFacts === newFacts) {
    [];
  } else if (aligned(oldFacts, newFacts)) {
    diff_aligned(oldFacts, newFacts, []);
  } else if (List.length(oldFacts) <= small && List.length(newFacts) <= small) {
    diff_small(oldFacts, newFacts);
  } else {
    diff_large(oldFacts, newFacts);
  };
/* Drop all but the last occurrence of each key; returns [facts] itself when
   there are no duplicates. */
let rec unique = facts =>
  switch (facts) {
  | [] => facts
  | [f, ...rest] =>
    let rest' = unique(rest);
    if (exists_key(f, rest')) {
      rest';
    } else if (rest' === rest) {
      facts;
    } else {
      [f, ...rest'];
    };
  };
let apply = (facts, changes) =>
  if (changes === []) {
    facts;
  } else if (List.length(facts) > small) {
    let t = table(facts);
    List.iter(
      fun
      | Set(v) => T.replace(t, key_string(key(v)), v)
      | Remove(k) => T.remove(t, key_string(k)),
      changes,
    );
    T.fold((_, v, acc) => [v, ...acc], t, []);
  } else {
    List.fold_left(
      (facts, change) =>
        switch (change) {
        | Set(v) => [v, ...List.filter(f => !same_key(f, v), facts)]
        | Remove(k) => List.filter(f => !has_key(k, f), facts)
        },
      unique(facts),
      changes,
    );
  };
let equal = (a, b) =>
  switch (diff(a, b)) {
  | [] => true
  | _ => false
  };
