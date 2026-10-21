open Tea_vdom;
let check = (name, condition) =>
  if (!condition) {
    failwith(name);
  };
let contains = (patches, pred) =>
  Array.exists(p => pred(p.Diff.operation), patches);
let verify = (name, a, b) => {
  let old = Naive.render(a);
  let patches = Diff.diff(a, b);
  let actual = Patch_sim.apply_to_tree(old, patches);
  check(name, Naive.equal(actual, Naive.render(b)));
  (old, actual, patches);
};
let text = Vdom.text;
let node = kids => Vdom.node("div", [], kids);
let keylist = pairs => Vdom.keyed("ul", [], pairs);
let plus = x => x + 1;
let minus = x => x - 1;
let event = Vdom.Normal(Tea_core.Decode.succeed(1));
let allFacts = [
  Vdom.attribute("title", "old"),
  Vdom.attribute_ns("svg", "href", "old"),
  Vdom.property("checked", Vdom.Bool(true)),
  Vdom.style("color", "red"),
  Vdom.on("click", event),
];
let () = {
  let a = node([text("same")]);
  check("physical skip", Array.length(Diff.diff(a, a)) == 0);
  let (_, _, p) = verify("text", text("a"), text("b"));
  check(
    "text patch",
    contains(
      p,
      fun
      | Diff.Text("b") => true
      | _ => false,
    ),
  );
  let (_, _, p) = verify("kind redraw", text("a"), node([]));
  check(
    "redraw patch",
    contains(
      p,
      fun
      | Diff.Redraw(_) => true
      | _ => false,
    ),
  );
  let (_, _, p) =
    verify(
      "tag redraw",
      Vdom.node("p", [], [text("a")]),
      node([text("b")]),
    );
  check("redraw terminal", Array.length(p) == 1);
  let (_, _, p) =
    verify(
      "namespace redraw",
      Vdom.node(~namespace="svg", "div", [], []),
      node([]),
    );
  check("namespace terminal", Array.length(p) == 1);
  let (_, _, p) =
    verify("append", node([text("a")]), node([text("b"), text("c")]));
  check(
    "append patch",
    contains(
      p,
      fun
      | Diff.Append(_) => true
      | _ => false,
    ),
  );
  let (_, _, p) =
    verify("remove-last", node([text("a"), text("b")]), node([]));
  check(
    "remove-last patch",
    contains(
      p,
      fun
      | Diff.Remove_last(2) => true
      | _ => false,
    ),
  );
  let (_, _, p) =
    verify("fact removals", Vdom.node("div", allFacts, []), node([]));
  check(
    "all fact removals",
    contains(
      p,
      fun
      | Diff.Facts(changes) =>
        List.length(changes) == 5
        && List.for_all(
             fun
             | Facts.Remove(_) => true
             | _ => false,
             changes,
           )
      | _ => false,
    ),
  );
  let newEvent = Vdom.Normal(Tea_core.Decode.succeed(2));
  let (_, _, p) =
    verify(
      "event swap",
      Vdom.node("div", [Vdom.on("click", event)], []),
      Vdom.node("div", [Vdom.on("click", newEvent)], []),
    );
  check(
    "handler set not remove",
    contains(
      p,
      fun
      | Diff.Facts([Facts.Set(Vdom.Event("click", _))]) => true
      | _ => false,
    ),
  );
  let (_, _, p) =
    verify(
      "nested taggers",
      Vdom.map(plus, Vdom.map(plus, text("a"))),
      Vdom.map(minus, Vdom.map(minus, text("b"))),
    );
  check(
    "tagger patch",
    contains(
      p,
      fun
      | Diff.Tagger(_) => true
      | _ => false,
    ),
  );
  check(
    "preorder tagger child",
    Array.map(p => p.Diff.index, p) == [|0, 1, 2|],
  );
  let calls = ref(0);
  let render = s => {
    incr(calls);
    text(s);
  };
  let a = Vdom.lazy1(render, "same");
  let old = Naive.render(a);
  let b = Vdom.lazy1(render, "same");
  let p = Diff.diff(a, b);
  check("thunk refs skip", calls^ == 1 && Array.length(p) == 0);
  check(
    "thunk cache transferred",
    switch (a, b) {
    | (Vdom.Thunk(a), Vdom.Thunk(b)) => a.node === b.node
    | _ => false
    },
  );
  ignore(Naive.render(b));
  check("transferred cache avoids render", calls^ == 1);
  check("skip apply shared", Patch_sim.apply_to_tree(old, p) === old);
  let (_, _, p) =
    verify(
      "thunk update",
      Vdom.lazy1(render, "a"),
      Vdom.lazy1(render, "b"),
    );
  check(
    "thunk local patch",
    contains(
      p,
      fun
      | Diff.Thunk(inner) => Array.length(inner) == 1 && inner[0].index == 0
      | _ => false,
    ),
  );
  let row = k => (k, node([text(k)]));
  let a = keylist(List.map(row, ["a", "b", "c"]));
  let b = keylist(List.map(row, ["b", "a", "c"]));
  let (old, fresh, p) = verify("swap", a, b);
  check(
    "reorder patch",
    contains(
      p,
      fun
      | Diff.Reorder(_) => true
      | _ => false,
    ),
  );
  check(
    "move identity",
    old.kids[0].uid == fresh.kids[1].uid
    && old.kids[1].uid == fresh.kids[0].uid,
  );
  check("swap creates zero", Naive.created(old, fresh) == 0);
  let (old, fresh, _) =
    verify(
      "insert remove move",
      a,
      keylist(List.map(row, ["c", "d", "a"])),
    );
  check(
    "reuse on combined edit",
    old.kids[2].uid == fresh.kids[0].uid
    && old.kids[0].uid == fresh.kids[2].uid,
  );
  ignore(
    verify(
      "duplicate fallback",
      keylist([row("a"), row("a"), row("b")]),
      keylist([row("b"), row("a")]),
    ),
  );
  let rows = changed =>
    keylist(
      List.init(1000, i =>
        (
          string_of_int(i),
          node([text(i == changed ? "changed" : string_of_int(i))]),
        )
      ),
    );
  let (old, fresh, _) = verify("1000 row update", rows(-1), rows(500));
  check("one row recreates zero", Naive.created(old, fresh) == 0);
  let makeDeep = wrap => {
    let a = ref(text("a"));
    let b = ref(text("b"));
    for (_ in 1 to 5000) {
      a := wrap(a^);
      b := wrap(b^);
    };
    (a^, b^);
  };
  List.iter(
    ((name, wrap)) => {
      let (a, b) = makeDeep(wrap);
      ignore(verify(name, a, b));
    },
    [
      ("5000 elements", n => node([n])),
      ("5000 taggers", n => Vdom.map(plus, n)),
      ("5000 thunks", n => Vdom.lazy1(n => n, n)),
    ],
  );
  print_endline(
    "targeted patches, identity reuse and 5000-deep element/tagger/thunk paths: OK",
  );
};

let rng = Random.State.make([|0x746561, 0x5041|]);
let random = n => Random.State.int(rng, n);
let word = () => string_of_int(random(8));
let handler = () =>
  switch (random(4)) {
  | 0 => Vdom.Normal(Tea_core.Decode.succeed(random(20)))
  | 1 =>
    Vdom.May_stop_propagation(
      Tea_core.Decode.succeed((random(20), random(2) == 0)),
    )
  | 2 =>
    Vdom.May_prevent_default(
      Tea_core.Decode.succeed((random(20), random(2) == 0)),
    )
  | _ =>
    Vdom.Custom(
      Tea_core.Decode.succeed({
        Vdom.message: random(20),
        stop_propagation: true,
        prevent_default: false,
      }),
    )
  };
let facts = () => {
  let options = [
    Vdom.attribute("title", word()),
    Vdom.attribute_ns("ns", "href", word()),
    Vdom.property("value", Vdom.String(word())),
    Vdom.property("checked", Vdom.Bool(random(2) == 0)),
    Vdom.property("number", Vdom.Float(float_of_int(random(10)))),
    Vdom.style("color", word()),
    Vdom.on("click", handler()),
  ];
  List.filter(_ => random(2) == 0, options);
};
let lazyRender = n => n;
let rec generate = depth =>
  if (depth == 0) {
    text(word());
  } else {
    let children = () => List.init(random(5), _ => generate(depth - 1));
    switch (random(6)) {
    | 0 => text(word())
    | 1 => Vdom.node(~namespace="svg", "g", facts(), children())
    | 2 => Vdom.node("div", facts(), children())
    | 3 =>
      Vdom.keyed(
        "ul",
        facts(),
        List.mapi(
          (i, n) => (random(5) == 0 ? "duplicate" : string_of_int(i), n),
          children(),
        ),
      )
    | 4 => Vdom.lazy1(lazyRender, generate(depth - 1))
    | _ => Vdom.map(random(2) == 0 ? plus : minus, generate(depth - 1))
    };
  };
/* Correlated pairs exercise deep edits and keyed permutations, rather than
   allowing independent roots to reduce most random cases to redraw. */
let rec pair = depth =>
  if (depth == 0) {
    (generate(1), generate(1));
  } else {
    switch (random(6)) {
    | 0 => (generate(3), generate(3))
    | 1 =>
      let (a, b) = pair(depth - 1);
      (Vdom.node("div", facts(), [a]), Vdom.node("div", facts(), [b]));
    | 2 =>
      let (a, b) = pair(depth - 1);
      (Vdom.map(plus, a), Vdom.map(random(2) == 0 ? plus : minus, b));
    | 3 =>
      let (a, b) = pair(depth - 1);
      (Vdom.lazy1(lazyRender, a), Vdom.lazy1(lazyRender, b));
    | 4 =>
      let a = List.init(random(6), _ => generate(2));
      let b = List.init(random(6), _ => generate(2));
      (Vdom.node("div", facts(), a), Vdom.node("div", facts(), b));
    | _ =>
      let source =
        Array.init(1 + random(8), i => (string_of_int(i), generate(2)));
      let old = Vdom.keyed("ul", facts(), Array.to_list(source));
      let destination = Array.copy(source);
      for (i in 0 to Array.length(destination) - 1) {
        let j = random(Array.length(destination));
        let tmp = destination[i];
        destination[i] = destination[j];
        destination[j] = tmp;
      };
      let retained =
        List.filter(_ => random(4) != 0, Array.to_list(destination));
      let retained =
        List.map(
          ((k, n)) => (k, random(4) == 0 ? generate(2) : n),
          retained,
        );
      let retained =
        random(2) == 0 ? [("insert", generate(2)), ...retained] : retained;
      (old, Vdom.keyed("ul", facts(), retained));
    };
  };
let () = {
  for (i in 1 to 10000) {
    let (a, b) = pair(4);
    ignore(verify("random pair " ++ string_of_int(i), a, b));
  };
  print_endline("10000 deterministic random old/new pairs: OK");
};
