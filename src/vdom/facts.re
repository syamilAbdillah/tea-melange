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

let same = (a, b) =>
  switch (a, b) {
  | (Vdom.Event(_, a), Vdom.Event(_, b)) => a === b
  | (Vdom.Attribute(_, a), Vdom.Attribute(_, b)) => a == b
  | (Vdom.Attribute_ns(_, _, a), Vdom.Attribute_ns(_, _, b)) => a == b
  | (Vdom.Property(_, a), Vdom.Property(_, b)) => a == b
  | (Vdom.Style(_, a), Vdom.Style(_, b)) => a == b
  | _ => false
  };

/* Last occurrence wins, as when setting DOM facts in order. */
let table = facts => {
  let t = Hashtbl.create(16);
  List.iter(f => Hashtbl.replace(t, key(f), f), facts);
  t;
};
let diff = (oldFacts, newFacts) => {
  let a = table(oldFacts);
  let b = table(newFacts);
  let out = ref([]);
  Hashtbl.iter(
    (k, _) =>
      if (!Hashtbl.mem(b, k)) {
        out := [Remove(k), ...out^];
      },
    a,
  );
  Hashtbl.iter(
    (k, v) =>
      switch (Hashtbl.find_opt(a, k)) {
      | Some(old) when same(old, v) => ()
      | _ => out := [Set(v), ...out^]
      },
    b,
  );
  List.rev(out^);
};
let apply = (facts, changes) => {
  let t = table(facts);
  List.iter(
    fun
    | Set(v) => Hashtbl.replace(t, key(v), v)
    | Remove(k) => Hashtbl.remove(t, k),
    changes,
  );
  Hashtbl.fold((_, v, acc) => [v, ...acc], t, []);
};
let equal = (a, b) => diff(a, b) == [];
