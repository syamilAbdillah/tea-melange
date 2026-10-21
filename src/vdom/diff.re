type placement('msg) =
  | Reuse(int)
  | Insert(Vdom.t('msg));
type operation('msg) =
  | Redraw(Vdom.t('msg))
  | Text(string)
  | Facts(list(Facts.change('msg)))
  | Append(array(Vdom.t('msg)))
  | Remove_last(int)
  | Reorder(array(placement('msg)))
  | Thunk(array(patch('msg)))
  | Tagger(Obj.t)
and patch('msg) = {
  index: int,
  operation: operation('msg),
};

type job('msg) =
  | Compare(Vdom.t('msg), Vdom.t('msg), int, ref(list(patch('msg))))
  | End_thunk(int, ref(list(patch('msg))), ref(list(patch('msg))));

let diff = (oldNode, newNode) => {
  let output = ref([]);
  let jobs = ref([Compare(oldNode, newNode, 0, output)]);
  let push = j => jobs := [j, ...jobs^];
  let emit = (out, index, operation) =>
    out :=
      [
        {
          index,
          operation,
        },
        ...out^,
      ];
  let positions = kids => {
    let index = ref(1);
    Array.map(
      k => {
        let i = index^;
        index := i + 1 + Vdom.descendants_count(k);
        i;
      },
      kids,
    );
  };
  let children = (a, b, index, out) => {
    let pos = positions(a);
    let common = min(Array.length(a), Array.length(b));
    if (Array.length(a) < Array.length(b)) {
      emit(
        out,
        index,
        Append(Array.sub(b, common, Array.length(b) - common)),
      );
    } else if (Array.length(a) > Array.length(b)) {
      emit(out, index, Remove_last(Array.length(a) - common));
    };
    for (i in 0 to common - 1) {
      let j = common - 1 - i;
      push(Compare(a[j], b[j], index + pos[j], out));
    };
  };
  let keyedChildren = (a, b, index, out) => {
    let sources = Hashtbl.create(Array.length(a));
    let seen = Hashtbl.create(Array.length(b));
    let duplicate = ref(false);
    Array.iteri(
      (i, (k, _)) => {
        if (Hashtbl.mem(sources, k)) {
          duplicate := true;
        };
        Hashtbl.replace(sources, k, i);
      },
      a,
    );
    Array.iter(
      ((k, _)) => {
        if (Hashtbl.mem(seen, k)) {
          duplicate := true;
        };
        Hashtbl.replace(seen, k, ());
      },
      b,
    );
    let oldKids = Array.map(((_, n)) => n, a);
    if (duplicate^) {
      children(oldKids, Array.map(((_, n)) => n, b), index, out);
    } else {
      let pos = positions(oldKids);
      let changed = ref(Array.length(a) != Array.length(b));
      let plan =
        Array.mapi(
          (i, (k, n)) =>
            switch (Hashtbl.find_opt(sources, k)) {
            | None =>
              changed := true;
              Insert(n);
            | Some(j) =>
              if (i != j) {
                changed := true;
              };
              Reuse(j);
            },
          b,
        );
      if (changed^) {
        emit(out, index, Reorder(plan));
      };
      /* Schedule comparisons in OLD order: patch indices stay sorted even
         when the destination order is completely reversed. */
      let targets = Hashtbl.create(Array.length(b));
      Array.iter(((k, n)) => Hashtbl.replace(targets, k, n), b);
      for (offset in 0 to Array.length(a) - 1) {
        let j = Array.length(a) - 1 - offset;
        let (k, old) = a[j];
        switch (Hashtbl.find_opt(targets, k)) {
        | None => ()
        | Some(n) => push(Compare(old, n, index + pos[j], out))
        };
      };
    };
  };
  while (jobs^ != []) {
    switch (jobs^) {
    | [] => ()
    | [job, ...rest] =>
      jobs := rest;
      switch (job) {
      | End_thunk(index, inner, out) =>
        if (inner^ != []) {
          emit(out, index, Thunk(Array.of_list(List.rev(inner^))));
        }
      | Compare(a, b, index, out) =>
        if (a !== b) {
          switch (a, b) {
          | (Vdom.Text(x), Vdom.Text(y)) =>
            if (x != y) {
              emit(out, index, Text(y));
            }
          | (Vdom.Node(x), Vdom.Node(y)) =>
            if (x.tag != y.tag || x.namespace != y.namespace) {
              emit(out, index, Redraw(b));
            } else {
              let changes = Facts.diff(x.facts, y.facts);
              if (changes != []) {
                emit(out, index, Facts(changes));
              };
              children(x.kids, y.kids, index, out);
            }
          | (Vdom.Keyed(x), Vdom.Keyed(y)) =>
            if (x.tag != y.tag || x.namespace != y.namespace) {
              emit(out, index, Redraw(b));
            } else {
              let changes = Facts.diff(x.facts, y.facts);
              if (changes != []) {
                emit(out, index, Facts(changes));
              };
              keyedChildren(x.kids, y.kids, index, out);
            }
          | (Vdom.Thunk(x), Vdom.Thunk(y)) =>
            let same = ref(Array.length(x.refs) == Array.length(y.refs));
            if (same^) {
              Array.iteri(
                (i, v) =>
                  if (v !== y.refs[i]) {
                    same := false;
                  },
                x.refs,
              );
            };
            if (same^) {
              y.node = x.node;
            } else {
              let inner = ref([]);
              push(End_thunk(index, inner, out));
              push(Compare(Vdom.force(x), Vdom.force(y), 0, inner));
            };
          | (Vdom.Tagger(x), Vdom.Tagger(y)) =>
            if (Obj.repr(x.tagger) !== Obj.repr(y.tagger)) {
              emit(out, index, Tagger(Obj.repr(y.tagger)));
            };
            /* Existential messages are never inspected here. Erasure is safe
               because diff only transports handlers/tagger slots as data. */
            push(
              Compare(
                Obj.magic(x.child),
                Obj.magic(y.child),
                index + 1,
                out,
              ),
            );
          | _ => emit(out, index, Redraw(b))
          };
        }
      };
    };
  };
  Array.of_list(List.rev(output^));
};
