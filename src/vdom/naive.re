module Tree = {
  type kind =
    | Text(string)
    | Element(string, option(string))
    | Thunk
    | Tagger(Obj.t);
  type t('msg) = {
    uid: int,
    kind,
    facts: list(Vdom.fact('msg)),
    kids: array(t('msg)),
    descendants_count: int,
  };
};
type render_job('msg) =
  | Visit(Vdom.t('msg), Tree.t('msg) => unit)
  | Finish(unit => unit);
let render = (~start_uid=0, node) => {
  let next = ref(start_uid);
  let result = ref(None);
  let jobs = ref([Visit(node, t => result := Some(t))]);
  let push = job => jobs := [job, ...jobs^];
  while (jobs^ != []) {
    switch (jobs^) {
    | [] => ()
    | [job, ...rest] =>
      jobs := rest;
      switch (job) {
      | Finish(f) => f()
      | Visit(n, set) =>
        let uid = next^;
        incr(next);
        let (kind, facts, children) =
          switch (n) {
          | Vdom.Text(s) => (Tree.Text(s), [], [||])
          | Vdom.Node(e) => (
              Tree.Element(e.tag, e.namespace),
              e.facts,
              e.kids,
            )
          | Vdom.Keyed(e) => (
              Tree.Element(e.tag, e.namespace),
              e.facts,
              Array.map(((_, k)) => k, e.kids),
            )
          | Vdom.Thunk(t) => (Tree.Thunk, [], [|Vdom.force(t)|])
          | Vdom.Tagger(t) =>
            /* Only the message parameter is erased; the tree shape and
               closures are preserved without ever invoking a tagger. */
            (Tree.Tagger(Obj.repr(t.tagger)), [], [|Obj.magic(t.child)|])
          };
        let slots = Array.make(Array.length(children), None);
        push(
          Finish(
            () =>
              set({
                Tree.uid,
                kind,
                facts,
                kids: Array.map(Option.get, slots),
                descendants_count: Vdom.descendants_count(n),
              }),
          ),
        );
        for (offset in 0 to Array.length(children) - 1) {
          let i = Array.length(children) - 1 - offset;
          push(Visit(children[i], t => slots[i] = Some(t)));
        };
      };
    };
  };
  Option.get(result^);
};
let walk = (tree, f) => {
  let jobs = ref([tree]);
  while (jobs^ != []) {
    switch (jobs^) {
    | [] => ()
    | [n, ...rest] =>
      jobs := rest;
      f(n);
      Array.iter(k => jobs := [k, ...jobs^], n.Tree.kids);
    };
  };
};
let max_uid = tree => {
  let n = ref(-1);
  walk(tree, t => n := max(n^, t.Tree.uid));
  n^;
};
let created = (old, fresh) => {
  let ids = Hashtbl.create(32);
  walk(old, t => Hashtbl.replace(ids, t.Tree.uid, ()));
  let count = ref(0);
  walk(fresh, t =>
    if (!Hashtbl.mem(ids, t.Tree.uid)) {
      incr(count);
    }
  );
  count^;
};
let equal = (a, b) => {
  let ok = ref(true);
  let jobs = ref([(a, b)]);
  while (ok^ && jobs^ != []) {
    switch (jobs^) {
    | [] => ()
    | [(a, b), ...rest] =>
      jobs := rest;
      let sameKind =
        switch (a.Tree.kind, b.Tree.kind) {
        | (Tree.Text(a), Tree.Text(b)) => a == b
        | (Tree.Element(a, nsA), Tree.Element(b, nsB)) =>
          a == b && nsA == nsB
        | (Tree.Thunk, Tree.Thunk) => true
        | (Tree.Tagger(a), Tree.Tagger(b)) => a === b
        | _ => false
        };
      if (!sameKind
          || !Facts.equal(a.facts, b.facts)
          || Array.length(a.kids) != Array.length(b.kids)) {
        ok := false;
      } else {
        Array.iteri((i, k) => jobs := [(k, b.kids[i]), ...jobs^], a.kids);
      };
    };
  };
  ok^;
};
