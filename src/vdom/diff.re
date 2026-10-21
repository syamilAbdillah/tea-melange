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

/* Monomorphic string table: no polymorphic hashing, and no Stdlib.Hashtbl
   (whose randomisation drags Random/Int64/Digest into browser bundles). */
module Keys = Tea_core.String_table;

let same_namespace = (a: option(string), b: option(string)) =>
  switch (a, b) {
  | (None, None) => true
  | (Some(a), Some(b)) => a == b
  | _ => false
  };

/* Recursion is used up to this depth. The child at exactly this depth is
   compared by draining a local work stack, on which everything deeper is
   pushed, so deep trees never overflow the JS stack and patches stay in
   pre-order. */
let max_depth = 256;

let diff = (oldNode, newNode) => {
  let output = ref([]);
  let jobs = ref([]);
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
  let rec same_refs = (x, y, i) =>
    i < 0 || x[i] === y[i] && same_refs(x, y, i - 1);
  /* Compare one child at depth [d]. Callers in push mode (d > max_depth)
     visit siblings last-to-first so the stack pops them in order. */
  let rec visit = (d, x, y, index, out) =>
    if (x === y) {
      ();
    } else if (d < max_depth) {
      compare(d, x, y, index, out);
    } else if (d == max_depth) {
      push(Compare(x, y, index, out));
      drain();
    } else {
      push(Compare(x, y, index, out));
    }
  and drain = () =>
    switch (jobs^) {
    | [] => ()
    | [job, ...rest] =>
      jobs := rest;
      switch (job) {
      | End_thunk(index, inner, out) =>
        switch (inner^) {
        | [] => ()
        | patches =>
          emit(out, index, Thunk(Array.of_list(List.rev(patches))))
        }
      | Compare(a, b, index, out) => compare(max_depth + 1, a, b, index, out)
      };
      drain();
    }
  /* Positional children, in old pre-order index space. */
  and children = (depth, a, b, index, out) => {
    let la = Array.length(a);
    let lb = Array.length(b);
    let common = min(la, lb);
    if (la < lb) {
      emit(out, index, Append(Array.sub(b, common, lb - common)));
    } else if (la > lb) {
      emit(out, index, Remove_last(la - common));
    };
    let d = depth + 1;
    if (d <= max_depth) {
      let child = ref(index + 1);
      for (j in 0 to common - 1) {
        let x = Array.unsafe_get(a, j);
        visit(d, x, Array.unsafe_get(b, j), child^, out);
        child := child^ + 1 + Vdom.descendants_count(x);
      };
    } else if (common > 0) {
      let pos = Array.make(common, 0);
      let child = ref(index + 1);
      for (j in 0 to common - 1) {
        Array.unsafe_set(pos, j, child^);
        child := child^ + 1 + Vdom.descendants_count(Array.unsafe_get(a, j));
      };
      for (j in common - 1 downto 0) {
        visit(
          d,
          Array.unsafe_get(a, j),
          Array.unsafe_get(b, j),
          Array.unsafe_get(pos, j),
          out,
        );
      };
    };
  }
  and keyed_children = (depth, a, b, index, out) => {
    let la = Array.length(a);
    let lb = Array.length(b);
    /* Matching keys at both ends pair positionally; only the middle needs
       key tables. */
    let start = ref(0);
    while (start^ < la
           && start^ < lb
           && fst(Array.unsafe_get(a, start^))
           == fst(Array.unsafe_get(b, start^))) {
      incr(start);
    };
    let start = start^;
    let ea = ref(la);
    let eb = ref(lb);
    while (ea^ > start
           && eb^ > start
           && fst(Array.unsafe_get(a, ea^ - 1))
           == fst(Array.unsafe_get(b, eb^ - 1))) {
      decr(ea);
      decr(eb);
    };
    let ea = ea^;
    let eb = eb^;
    /* Key lookups in the middle: linear scans for small middles (single
       insert/remove/move), hash tables otherwise. -1 when absent. */
    let small = (ea - start) * (eb - start) <= 64;
    let rec scan = (k: string, arr, i, until) =>
      if (i >= until) {
        (-1);
      } else if (fst(Array.unsafe_get(arr, i)) == k) {
        i;
      } else {
        scan(k, arr, i + 1, until);
      };
    let duplicate = ref(false);
    let table = (arr, until) => {
      let t = Keys.create(until - start);
      for (i in start to until - 1) {
        let k = fst(Array.unsafe_get(arr, i));
        if (small) {
          if (scan(k, arr, i + 1, until) >= 0) {
            duplicate := true;
          };
        } else {
          if (Keys.mem(t, k)) {
            duplicate := true;
          };
          Keys.replace(t, k, i);
        };
      };
      t;
    };
    let lookup = (t, k, arr, until) =>
      if (small) {
        scan(k, arr, start, until);
      } else {
        switch (Keys.find_opt(t, k)) {
        | None => (-1)
        | Some(i) => i
        };
      };
    /* New counterpart of old child [j]; the old child itself when its key
       is gone (physically equal, so nothing is compared). */
    let targets = ref(None);
    let target = j => {
      let old = snd(Array.unsafe_get(a, j));
      if (j < start || start == ea && start == eb) {
        snd(Array.unsafe_get(b, j));
      } else if (j >= ea) {
        snd(Array.unsafe_get(b, j - ea + eb));
      } else {
        switch (targets^) {
        | None => old
        | Some(t) =>
          let i = lookup(t, fst(Array.unsafe_get(a, j)), b, eb);
          i < 0 ? old : snd(Array.unsafe_get(b, i));
        };
      };
    };
    /* Compare in OLD order: patch indices stay sorted even when the
       destination order is completely reversed. */
    let compare_all = () => {
      let d = depth + 1;
      if (d <= max_depth) {
        let child = ref(index + 1);
        for (j in 0 to la - 1) {
          let x = snd(Array.unsafe_get(a, j));
          visit(d, x, target(j), child^, out);
          child := child^ + 1 + Vdom.descendants_count(x);
        };
      } else if (la > 0) {
        let pos = Array.make(la, 0);
        let child = ref(index + 1);
        for (j in 0 to la - 1) {
          Array.unsafe_set(pos, j, child^);
          child :=
            child^ + 1 + Vdom.descendants_count(snd(Array.unsafe_get(a, j)));
        };
        for (j in la - 1 downto 0) {
          visit(
            d,
            snd(Array.unsafe_get(a, j)),
            target(j),
            Array.unsafe_get(pos, j),
            out,
          );
        };
      };
    };
    if (start == ea && start == eb) {
      /* Same keys in the same order. */
      compare_all();
    } else if (start == ea && ea == la) {
      emit(out, index, Append(Array.map(snd, Array.sub(b, la, lb - la))));
      compare_all();
    } else if (start == eb && eb == lb) {
      emit(out, index, Remove_last(la - lb));
      compare_all();
    } else {
      let sources = table(a, ea);
      let t = table(b, eb);
      if (duplicate^) {
        children(depth, Array.map(snd, a), Array.map(snd, b), index, out);
      } else {
        let plan =
          Array.init(lb, i =>
            if (i < start) {
              Reuse(i);
            } else if (i >= eb) {
              Reuse(i - eb + ea);
            } else {
              let (k, n) = Array.unsafe_get(b, i);
              let j = lookup(sources, k, a, ea);
              j < 0 ? Insert(n) : Reuse(j);
            }
          );
        emit(out, index, Reorder(plan));
        targets := Some(t);
        compare_all();
      };
    };
  }
  and compare = (depth, a, b, index, out) =>
    switch (a, b) {
    | (Vdom.Text(x), Vdom.Text(y)) =>
      if (x != y) {
        emit(out, index, Text(y));
      }
    | (Vdom.Node(x), Vdom.Node(y)) =>
      if (x.tag != y.tag || !same_namespace(x.namespace, y.namespace)) {
        emit(out, index, Redraw(b));
      } else {
        switch (Facts.diff(x.facts, y.facts)) {
        | [] => ()
        | changes => emit(out, index, Facts(changes))
        };
        children(depth, x.kids, y.kids, index, out);
      }
    | (Vdom.Keyed(x), Vdom.Keyed(y)) =>
      if (x.tag != y.tag || !same_namespace(x.namespace, y.namespace)) {
        emit(out, index, Redraw(b));
      } else {
        switch (Facts.diff(x.facts, y.facts)) {
        | [] => ()
        | changes => emit(out, index, Facts(changes))
        };
        keyed_children(depth, x.kids, y.kids, index, out);
      }
    | (Vdom.Thunk(x), Vdom.Thunk(y)) =>
      let n = Array.length(x.refs);
      if (n == Array.length(y.refs) && same_refs(x.refs, y.refs, n - 1)) {
        y.node = x.node;
      } else {
        let inner = ref([]);
        let d = depth + 1;
        if (d <= max_depth) {
          visit(d, Vdom.force(x), Vdom.force(y), 0, inner);
          switch (inner^) {
          | [] => ()
          | patches =>
            emit(out, index, Thunk(Array.of_list(List.rev(patches))))
          };
        } else {
          push(End_thunk(index, inner, out));
          push(Compare(Vdom.force(x), Vdom.force(y), 0, inner));
        };
      };
    | (Vdom.Tagger(x), Vdom.Tagger(y)) =>
      if (Obj.repr(x.tagger) !== Obj.repr(y.tagger)) {
        emit(out, index, Tagger(Obj.repr(y.tagger)));
      };
      /* Existential messages are never inspected here. Erasure is safe
         because diff only transports handlers/tagger slots as data. */
      visit(
        depth + 1,
        Obj.magic(x.child),
        Obj.magic(y.child),
        index + 1,
        out,
      );
    | _ => emit(out, index, Redraw(b))
    };
  visit(0, oldNode, newNode, 0, output);
  Array.of_list(List.rev(output^));
};
