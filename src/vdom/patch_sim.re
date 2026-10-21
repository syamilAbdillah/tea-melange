module T = Naive.Tree;
type plan('msg) = {
  patches: array(Diff.patch('msg)),
  by_index: Hashtbl.t(int, list(Diff.operation('msg))),
};
type job('msg) =
  | Visit(T.t('msg), int, plan('msg), T.t('msg) => unit)
  | Finish(unit => unit);
let plan = patches => {
  let by_index = Hashtbl.create(Array.length(patches));
  /* Reverse lists once so same-index operations retain emission order. */
  Array.iter(
    p => {
      let old =
        Option.value(Hashtbl.find_opt(by_index, p.Diff.index), ~default=[]);
      Hashtbl.replace(by_index, p.index, [p.operation, ...old]);
    },
    patches,
  );
  Hashtbl.filter_map_inplace((_, ops) => Some(List.rev(ops)), by_index);
  {
    patches,
    by_index,
  };
};
let touches = (plan, lo, hi) => {
  let left = ref(0);
  let right = ref(Array.length(plan.patches));
  while (left^ < right^) {
    let mid = (left^ + right^) / 2;
    if (plan.patches[mid].Diff.index < lo) {
      left := mid + 1;
    } else {
      right := mid;
    };
  };
  left^ < Array.length(plan.patches) && plan.patches[left^].Diff.index <= hi;
};
let apply_to_tree = (old, patches) =>
  if (Array.length(patches) == 0) {
    old;
  } else {
    let next = ref(Naive.max_uid(old) + 1);
    let render = v => {
      let t = Naive.render(~start_uid=next^, v);
      next := Naive.max_uid(t) + 1;
      t;
    };
    let result = ref(None);
    let jobs = ref([Visit(old, 0, plan(patches), t => result := Some(t))]);
    let push = j => jobs := [j, ...jobs^];
    while (jobs^ != []) {
      switch (jobs^) {
      | [] => ()
      | [job, ...rest] =>
        jobs := rest;
        switch (job) {
        | Finish(f) => f()
        | Visit(t, index, current, set) =>
          if (!touches(current, index, index + t.T.descendants_count)) {
            set(t);
          } else {
            let ops =
              Option.value(
                Hashtbl.find_opt(current.by_index, index),
                ~default=[],
              );
            let redraw =
              List.find_opt(
                fun
                | Diff.Redraw(_) => true
                | _ => false,
                ops,
              );
            switch (redraw) {
            | Some(Diff.Redraw(v)) => set(render(v))
            | _ =>
              let slots = Array.map(k => ref(k), t.kids);
              push(
                Finish(
                  () => {
                    let kind = ref(t.kind);
                    let facts = ref(t.facts);
                    let kids = ref(Array.map(r => r^, slots));
                    List.iter(
                      fun
                      | Diff.Text(s) => kind := T.Text(s)
                      | Diff.Facts(changes) =>
                        facts := Facts.apply(facts^, changes)
                      | Diff.Tagger(f) => kind := T.Tagger(f)
                      | Diff.Append(extra) =>
                        kids := Array.append(kids^, Array.map(render, extra))
                      | Diff.Remove_last(n) =>
                        kids := Array.sub(kids^, 0, Array.length(kids^) - n)
                      | Diff.Reorder(order) => {
                          let sources = kids^;
                          kids :=
                            Array.map(
                              fun
                              | Diff.Reuse(i) => sources[i]
                              | Diff.Insert(v) => render(v),
                              order,
                            );
                        }
                      | Diff.Thunk(_)
                      | Diff.Redraw(_) => (),
                      ops,
                    );
                    let descendants_count =
                      switch (kind^) {
                      | T.Thunk => 0
                      | _ =>
                        Array.fold_left(
                          (n, k) => n + 1 + k.T.descendants_count,
                          0,
                          kids^,
                        )
                      };
                    set({
                      ...t,
                      kind: kind^,
                      facts: facts^,
                      kids: kids^,
                      descendants_count,
                    });
                  },
                ),
              );
              switch (t.kind) {
              | T.Thunk =>
                List.iter(
                  fun
                  | Diff.Thunk(inner) =>
                    push(
                      Visit(t.kids[0], 0, plan(inner), k => slots[0] := k),
                    )
                  | _ => (),
                  ops,
                )
              | _ =>
                let positions = Array.make(Array.length(t.kids), 0);
                let childIndex = ref(index + 1);
                Array.iteri(
                  (i, k) => {
                    positions[i] = childIndex^;
                    childIndex := childIndex^ + 1 + k.T.descendants_count;
                  },
                  t.kids,
                );
                for (offset in 0 to Array.length(t.kids) - 1) {
                  let i = Array.length(t.kids) - 1 - offset;
                  push(
                    Visit(
                      t.kids[i],
                      positions[i],
                      current,
                      k => slots[i] := k,
                    ),
                  );
                };
              };
            };
          }
        };
      };
    };
    Option.get(result^);
  };
