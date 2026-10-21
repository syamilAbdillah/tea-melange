open Tea_vdom;
open Bindings;
/* Erasure is confined to this backend. Messages are never inspected: handlers
   produce the input to their lexical tagger slots, then the root dispatch. */
type slot = ref(Obj.t => Obj.t);
type kind =
  | Element
  | Text
  | Tagger(slot)
  | Thunk;
type environment = {
  dispatch: Obj.t => unit,
  /* Controlled nodes whose properties can change without an event of their
     own (options, radios): restored after every patch. */
  always: js_set(mounted),
  /* Other controlled nodes that saw input/change since the last patch. */
  dirty: js_set(mounted),
  /* Consumed by the first element built (Render.create_in). */
  mutable root_dom: option(Dom.node),
  /* One capture listener pair (input/change) on the current root marks
     watched nodes dirty; event paths include detached ancestors. */
  mutable watch_root: option(Dom.node),
  mutable mark: raw => unit,
}
and mounted = {
  mutable dom: Dom.node,
  mutable kind,
  mutable kids: array(mounted),
  mutable descendants: int,
  scope: list(slot),
  env: environment,
  mutable facts: list(Vdom.fact(Obj.t)),
  mutable events: list(listener),
  /* Last-occurrence value/checked/selected properties; [] when the node is
     not controlled. */
  mutable controls: list((string, Vdom.prop_value)),
  mutable control,
}
/* How a controlled node learns about edits that bypass the model. */
and control =
  | Uncontrolled
  | Always
  | Watched
and listener = {
  name: string,
  mutable handler: Vdom.handler(Obj.t),
  mutable callback: raw => unit,
  /* Cleared when the node is discarded: dropped DOM is left to the GC with
     its listeners attached but inert, as removing them would only cost. */
  mutable live: bool,
};
let listeners = ref(0);
let patches = ref(0);
/* Recursion is used up to this depth; deeper subtrees continue on explicit
   work stacks, so arbitrarily deep views never overflow the JS stack. */
let max_depth = 256;
/* Wrapper roots are refreshed postorder when their child root changes. */
let node = m => m.dom;
let rec controls = (facts, acc) =>
  switch (facts) {
  | [] => acc
  | [Vdom.Property(("value" | "checked" | "selected") as k, v), ...rest] =>
    controls(rest, [(k, v), ...List.filter(((k', _)) => k' != k, acc)])
  | [_, ...rest] => controls(rest, acc)
  };
/* value/checked may already hold the new value (the user just typed it);
   skipping the write keeps the caret and avoids the setter's work. */
let set_property = (dom, name, v) =>
  switch (v) {
  | Vdom.String(s) =>
    if (name != "value" || (cast(get(dom, name)): string) !== s) {
      put(dom, name, s);
    }
  | Vdom.Bool(b) =>
    if (name != "checked" || (cast(get(dom, name)): bool) !== b) {
      put(dom, name, b);
    }
  | Vdom.Float(f) => put(dom, name, f)
  };
let route = (m, message) => {
  let message = List.fold_left((v, slot) => slot^(v), message, m.scope);
  m.env.dispatch(message);
};
let fire = (m, handler, event) => {
  let emit = result =>
    switch (result) {
    | Error(_) => ()
    | Ok((message, stop_, prevent_)) =>
      if (stop_) {
        stop(event);
      };
      if (prevent_) {
        prevent(event);
      };
      Option.iter(message => route(m, message), message);
    };
  switch (handler) {
  | Vdom.Normal(d) =>
    emit(
      Result.map(x => (Some(x), false, false), Dom_decode.run(d, event)),
    )
  | Vdom.May_stop_propagation(d) =>
    emit(
      Result.map(
        ((x, b)) => (Some(x), b, false),
        Dom_decode.run(d, event),
      ),
    )
  | Vdom.May_prevent_default(d) =>
    emit(
      Result.map(
        ((x, b)) => (Some(x), false, b),
        Dom_decode.run(d, event),
      ),
    )
  | Vdom.Custom_optional(d) =>
    emit(
      Result.map(
        x => (x.Vdom.message, x.stop_propagation, x.prevent_default),
        Dom_decode.run(d, event),
      ),
    )
  | Vdom.Custom(d) =>
    emit(
      Result.map(
        x => (Some(x.Vdom.message), x.stop_propagation, x.prevent_default),
        Dom_decode.run(d, event),
      ),
    )
  };
};
let rec find_listener = (name, ls) =>
  switch (ls) {
  | [] => None
  | [l, ...rest] => l.name == name ? Some(l) : find_listener(name, rest)
  };
let listen_to = (m, name, handler) =>
  switch (find_listener(name, m.events)) {
  | Some(l) => l.handler = handler
  | None =>
    let l = {
      name,
      handler,
      callback: _ => (),
      live: true,
    };
    l.callback = (
      event =>
        if (l.live) {
          fire(m, l.handler, event);
        }
    );
    m.events = [l, ...m.events];
    listen(m.dom, name, l.callback);
    incr(listeners);
  };
let detach = (m, name) =>
  switch (find_listener(name, m.events)) {
  | None => ()
  | Some(l) =>
    unlisten(m.dom, name, l.callback);
    m.events = List.filter(x => x !== l, m.events);
    decr(listeners);
  };
let local_name = s =>
  switch (String.index_opt(s, ':')) {
  | None => s
  | Some(_) =>
    switch (String.split_on_char(':', s)) {
    | [_prefix, name] => name
    | _ => s
    }
  };
let set_fact = (m, fact) =>
  switch (fact) {
  | Vdom.Attribute("", _) => ()
  | Vdom.Attribute(k, v) => attribute(m.dom, k, v)
  | Vdom.Attribute_ns(ns, k, v) => attribute_ns(m.dom, ns, k, v)
  | Vdom.Property(k, v) => set_property(m.dom, k, v)
  | Vdom.Style(k, v) => style_set(style(m.dom), k, v)
  | Vdom.Event(name, handler) => listen_to(m, name, handler)
  };
let unwatch = m =>
  switch (m.control) {
  | Uncontrolled => ()
  | Always => set_delete(m.env.always, m)
  | Watched =>
    put(m.dom, "__tea_watch", Js.undefined);
    set_delete(m.env.dirty, m);
  };
/* Every user edit of value/checked fires input or change on the edited node
   (as React relies on for controlled inputs). Selecting an option or a radio
   button changes its siblings silently, so those are always restored. */
let track_controlled = m => {
  m.controls = controls(m.facts, []);
  let wanted =
    switch (m.controls) {
    | [] => 0
    | cs =>
      List.exists(((k, _)) => k == "selected", cs)
      || (cast(get(m.dom, "type")): string) == "radio"
        ? 1 : 2
    };
  let current =
    switch (m.control) {
    | Uncontrolled => 0
    | Always => 1
    | Watched => 2
    };
  if (wanted != current) {
    unwatch(m);
    m.control = (
      switch (wanted) {
      | 0 => Uncontrolled
      | 1 =>
        set_add(m.env.always, m);
        Always;
      | _ =>
        put(m.dom, "__tea_watch", m);
        Watched;
      }
    );
  };
};
/* Fresh nodes take facts in order: later duplicates win, as in Facts.diff. */
let is_control =
  fun
  | Vdom.Property("value" | "checked" | "selected", _) => true
  | _ => false;
let rec set_facts = (m, facts, control) =>
  switch (facts) {
  | [] => control
  | [f, ...rest] =>
    set_fact(m, f);
    set_facts(m, rest, control || is_control(f));
  };
let init_facts = (m, facts) =>
  switch (facts) {
  | [] => ()
  | _ =>
    m.facts = facts;
    if (set_facts(m, facts, false)) {
      track_controlled(m);
    };
  };
let rec last_property = (k, facts, found) =>
  switch (facts) {
  | [] => found
  | [Vdom.Property(k', v), ...rest] when k' == k =>
    last_property(k, rest, Some(v))
  | [_, ...rest] => last_property(k, rest, found)
  };
let change = (m, change) =>
  switch (change) {
  | Facts.Set(fact) => set_fact(m, fact)
  | Facts.Remove(key) =>
    switch (key) {
    | Facts.Attribute("") => ()
    | Facts.Attribute(k) => remove_attribute(m.dom, k)
    | Facts.Attribute_ns(ns, k) =>
      remove_attribute_ns(m.dom, ns, local_name(k))
    | Facts.Style(k) => style_remove(style(m.dom), k)
    | Facts.Event(k) => detach(m, k)
    | Facts.Property(k) =>
      switch (last_property(k, m.facts, None)) {
      | Some(Vdom.Bool(_)) => put(m.dom, k, false)
      | Some(Vdom.String(_)) => put(m.dom, k, "")
      | _ => put(m.dom, k, Js.undefined)
      }
    }
  };
/* [m.facts] is only read for properties (removal defaults, controls), so
   attribute/style/event changes need no bookkeeping. */
let rec apply_changes = (m, cs, property) =>
  switch (cs) {
  | [] => property
  | [c, ...rest] =>
    change(m, c);
    apply_changes(
      m,
      rest,
      property
      || (
        switch (c) {
        | Facts.Set(Vdom.Property(_))
        | Facts.Remove(Facts.Property(_)) => true
        | _ => false
        }
      ),
    );
  };
let changes = (m, cs) =>
  if (apply_changes(m, cs, false)) {
    m.facts = Facts.apply(m.facts, cs);
    track_controlled(m);
  };
let make = (env, scope, kind, dom, descendants) => {
  dom,
  kind,
  kids: [||],
  descendants,
  scope,
  env,
  facts: [],
  events: [],
  controls: [],
  control: Uncontrolled,
};
let element_dom = (env, tag, ns) =>
  switch (env.root_dom) {
  | Some(dom) =>
    env.root_dom = None;
    dom;
  | None =>
    switch (ns) {
    | None => element(document, tag)
    | Some(ns) => element_ns(document, ns, tag)
    }
  };
/* Deep subtrees: an explicit stack, as wrappers never allocate DOM nodes. */
let build_stack = (env, scope, vdom) => {
  let result = ref(None);
  let jobs = ref([]);
  let push = j => jobs := [j, ...jobs^];
  let rec schedule = (scope, v, done_) =>
    push(() =>
      switch (v) {
      | Vdom.Text(s) => done_(make(env, scope, Text, text(document, s), 0))
      | Vdom.Node(e) =>
        element_job(
          scope,
          e.tag,
          e.namespace,
          e.facts,
          e.kids,
          e.descendants_count,
          done_,
        )
      | Vdom.Keyed(e) =>
        element_job(
          scope,
          e.tag,
          e.namespace,
          e.facts,
          Array.map(((_, v)) => v, e.kids),
          e.descendants_count,
          done_,
        )
      | Vdom.Thunk(t) =>
        schedule(
          scope,
          Vdom.force(t),
          child => {
            let m = make(env, scope, Thunk, node(child), 0);
            m.kids = [|child|];
            push(() => done_(m));
          },
        )
      | Vdom.Tagger(t) =>
        /* Existential function and child agree by Vdom's constructor invariant. */
        let slot: slot = ref(Obj.magic(t.tagger));
        schedule(
          [slot, ...scope],
          Obj.magic(t.child),
          child => {
            let m =
              make(env, scope, Tagger(slot), node(child), t.descendants);
            m.kids = [|child|];
            push(() => done_(m));
          },
        );
      }
    )
  and element_job = (scope, tag, ns, facts, kids, descendants, done_) => {
    let dom = element_dom(env, tag, ns);
    let m = make(env, scope, Element, dom, descendants);
    /* Facts are applied after children so select.value sees its options. */
    let children = Array.make(Array.length(kids), m);
    push(() => {
      m.kids = children;
      Array.iter(child => append(dom, node(child)), children);
      init_facts(m, facts);
      done_(m);
    });
    for (j in Array.length(kids) - 1 downto 0) {
      schedule(scope, kids[j], child => children[j] = child);
    };
  };
  schedule(scope, vdom, m => result := Some(m));
  let rec loop = () =>
    switch (jobs^) {
    | [] => ()
    | [job, ...rest] =>
      jobs := rest;
      job();
      loop();
    };
  loop();
  Option.get(result^);
};
let rec build_at = (depth, env, scope, v) =>
  if (depth > max_depth) {
    build_stack(env, scope, v);
  } else {
    switch (v) {
    | Vdom.Text(s) => make(env, scope, Text, text(document, s), 0)
    | Vdom.Node(e) =>
      let dom = element_dom(env, e.tag, e.namespace);
      let m = make(env, scope, Element, dom, e.descendants_count);
      let kids = e.kids;
      let n = Array.length(kids);
      if (n > 0) {
        let children = Array.make(n, m);
        for (i in 0 to n - 1) {
          let c = build_at(depth + 1, env, scope, Array.unsafe_get(kids, i));
          append(dom, node(c));
          Array.unsafe_set(children, i, c);
        };
        m.kids = children;
      };
      /* Facts are applied after children so select.value sees its options. */
      init_facts(m, e.facts);
      m;
    | Vdom.Keyed(e) =>
      let dom = element_dom(env, e.tag, e.namespace);
      let m = make(env, scope, Element, dom, e.descendants_count);
      let kids = e.kids;
      let n = Array.length(kids);
      if (n > 0) {
        let children = Array.make(n, m);
        for (i in 0 to n - 1) {
          let (_, kid) = Array.unsafe_get(kids, i);
          let c = build_at(depth + 1, env, scope, kid);
          append(dom, node(c));
          Array.unsafe_set(children, i, c);
        };
        m.kids = children;
      };
      init_facts(m, e.facts);
      m;
    | Vdom.Thunk(t) =>
      let child = build_at(depth + 1, env, scope, Vdom.force(t));
      let m = make(env, scope, Thunk, node(child), 0);
      m.kids = [|child|];
      m;
    | Vdom.Tagger(t) =>
      /* Existential function and child agree by Vdom's constructor invariant. */
      let slot: slot = ref(Obj.magic(t.tagger));
      let child =
        build_at(depth + 1, env, [slot, ...scope], Obj.magic(t.child));
      let m = make(env, scope, Tagger(slot), node(child), t.descendants);
      m.kids = [|child|];
      m;
    };
  };
let build = (env, scope, vdom) => build_at(0, env, scope, vdom);
/* [detach] really removes listeners (the root may stay in the document);
   otherwise the subtree is being discarded and listeners are made inert. */
let release = (detach, m) => {
  switch (m.events) {
  | [] => ()
  | events =>
    List.iter(
      l => {
        if (detach) {
          unlisten(m.dom, l.name, l.callback);
        };
        l.live = false;
        decr(listeners);
      },
      events,
    );
    m.events = [];
  };
  unwatch(m);
  m.control = Uncontrolled;
  m.controls = [];
};
let cleanup_stack = (detach, m) => {
  let rec loop = jobs =>
    switch (jobs) {
    | [] => ()
    | [m, ...rest] =>
      release(detach, m);
      loop(Array.fold_left((jobs, c) => [c, ...jobs], rest, m.kids));
    };
  loop([m]);
};
let rec cleanup_at = (detach, depth, m) =>
  if (depth > max_depth) {
    cleanup_stack(detach, m);
  } else {
    release(detach, m);
    let kids = m.kids;
    for (i in 0 to Array.length(kids) - 1) {
      cleanup_at(detach, depth + 1, Array.unsafe_get(kids, i));
    };
  };
let cleanup = m => cleanup_at(false, 0, m);
let watch = (env, dom) =>
  switch (env.watch_root) {
  | Some(d) when d === dom => ()
  | previous =>
    switch (previous) {
    | Some(d) =>
      unlisten_capture(d, "input", env.mark, true);
      unlisten_capture(d, "change", env.mark, true);
    | None => ()
    };
    env.watch_root = Some(dom);
    listen_capture(dom, "input", env.mark, true);
    listen_capture(dom, "change", env.mark, true);
  };
let attach_root = m => {
  let dom = node(m);
  put(dom, "__tea_root", m);
  watch(m.env, dom);
  dom;
};
let root = dom => {
  let value = get(dom, "__tea_root");
  if (Js.testAny(value)) {
    invalid_arg("tea.dom: expected a Render.create root");
  };
  (cast(value): mounted);
};
let environment = (dispatch, root_dom) => {
  let env = {
    dispatch: Obj.magic(dispatch),
    always: set_make(),
    dirty: set_make(),
    root_dom,
    watch_root: None,
    mark: _ => (),
  };
  env.mark = (
    event => {
      let m = get(get(event, "target"), "__tea_watch");
      if (!Js.testAny(m)) {
        set_add(env.dirty, cast(m): mounted);
      };
    }
  );
  env;
};
let create = (dispatch, vdom) =>
  attach_root(build(environment(dispatch, None), [], Obj.magic(vdom)));
/* Fully detach a root: listeners are removed, not just made inert. */
let dispose = dom => {
  let m = root(dom);
  cleanup_at(true, 0, m);
  switch (m.env.watch_root) {
  | Some(d) =>
    unlisten_capture(d, "input", m.env.mark, true);
    unlisten_capture(d, "change", m.env.mark, true);
    m.env.watch_root = None;
  | None => ()
  };
};
/* Document owns the existing body, not an extra wrapper or a replacement body. */
let create_in = (dispatch, vdom, mount) => {
  set_text(mount, "");
  attach_root(
    build(environment(dispatch, Some(mount)), [], Obj.magic(vdom)),
  );
};
let refresh_count = m =>
  switch (m.kind) {
  | Thunk =>
    m.dom = node(m.kids[0]);
    m.descendants = 0;
  | Tagger(_) =>
    let c = m.kids[0];
    m.dom = node(c);
    m.descendants = 1 + c.descendants;
  | Text => m.descendants = 0
  | Element =>
    let n = ref(0);
    let kids = m.kids;
    for (i in 0 to Array.length(kids) - 1) {
      n := n^ + 1 + Array.unsafe_get(kids, i).descendants;
    };
    m.descendants = n^;
  };
/* Restore controlled properties a user (or script) may have edited. Only
   differing values are written: reads are much cheaper than writes. */
let restore = (dom, (k, v)) =>
  switch (v) {
  | Vdom.String(s) =>
    if ((cast(get(dom, k)): string) !== s) {
      put(dom, k, s);
    }
  | Vdom.Bool(b) =>
    if ((cast(get(dom, k)): bool) !== b) {
      put(dom, k, b);
    }
  | Vdom.Float(f) => put(dom, k, f)
  };
let sync_controls = env => {
  let sync = m => List.iter(restore(m.dom), m.controls);
  set_iter(env.always, sync);
  set_iter(env.dirty, sync);
  set_clear(env.dirty);
};
/* Remove every child at once; much cheaper than one removeChild per row. */
let clear_children = (m, kids) => {
  set_text(node(m), "");
  Array.iter(cleanup, kids);
};
/* Indices (into [seq]) of a longest strictly increasing subsequence of the
   non-negative entries. Patience sorting with predecessor links. */
let longest_increasing = seq => {
  let n = Array.length(seq);
  let tails = Array.make(n, 0);
  let prev = Array.make(n, -1);
  let len = ref(0);
  for (i in 0 to n - 1) {
    let v = seq[i];
    if (v >= 0) {
      let lo = ref(0);
      let hi = ref(len^);
      while (lo^ < hi^) {
        let mid = (lo^ + hi^) lsr 1;
        if (seq[tails[mid]] < v) {
          lo := mid + 1;
        } else {
          hi := mid;
        };
      };
      if (lo^ > 0) {
        prev[i] = tails[lo^ - 1];
      };
      tails[lo^] = i;
      if (lo^ == len^) {
        incr(len);
      };
    };
  };
  let keep = Array.make(n, false);
  if (len^ > 0) {
    let i = ref(tails[len^ - 1]);
    while (i^ >= 0) {
      keep[i^] = true;
      i := prev[i^];
    };
  };
  keep;
};
let reorder = (m, placements) => {
  let sources = m.kids;
  let used = Array.make(Array.length(sources), false);
  let reused = ref(0);
  let n = Array.length(placements);
  /* Old position of each destination, or -1 for fresh nodes. */
  let seq = Array.make(n, -1);
  let increasing = ref(true);
  let last = ref(-1);
  Array.iteri(
    (i, p) =>
      switch (p) {
      | Diff.Reuse(j) =>
        used[j] = true;
        incr(reused);
        seq[i] = j;
        if (j < last^) {
          increasing := false;
        };
        last := j;
      | Diff.Insert(_) => ()
      },
    placements,
  );
  let dom = node(m);
  if (reused^ == 0) {
    clear_children(m, sources);
    let destinations =
      Array.map(
        p =>
          switch (p) {
          | Diff.Insert(v) => build(m.env, m.scope, v)
          | Diff.Reuse(_) => assert(false)
          },
        placements,
      );
    Array.iter(c => append(dom, node(c)), destinations);
    m.kids = destinations;
  } else {
    Array.iteri(
      (i, c) =>
        if (!used[i]) {
          remove(dom, node(c));
          cleanup(c);
        },
      sources,
    );
    /* Snapshot every source handle before changing the child list. */
    let destinations =
      Array.map(
        p =>
          switch (p) {
          | Diff.Reuse(i) => sources[i]
          | Diff.Insert(v) => build(m.env, m.scope, v)
          },
        placements,
      );
    /* Nodes on a longest increasing run of old positions stay put; every
       other node is inserted, right to left, before its successor. */
    let keep =
      if (increasing^) {
        Array.map(j => j >= 0, seq);
      } else {
        longest_increasing(seq);
      };
    for (i in n - 1 downto 0) {
      if (!keep[i]) {
        let anchor =
          if (i + 1 < n) {
            Js.Nullable.return(node(destinations[i + 1]));
          } else {
            Js.Nullable.null;
          };
        insert(dom, node(destinations[i]), anchor);
      };
    };
    m.kids = destinations;
  };
};
let operate = (m, op) =>
  switch (op) {
  | Diff.Redraw(v) =>
    let old = node(m);
    let next = build(m.env, m.scope, v);
    switch (Js.Nullable.toOption(parent(old))) {
    | None => ()
    | Some(p) => replace(p, node(next), old)
    };
    cleanup(m);
    next;
  | Diff.Text(s) =>
    set_text(node(m), s);
    m;
  | Diff.Facts(cs) =>
    changes(m, cs);
    m;
  | Diff.Tagger(f) =>
    switch (m.kind) {
    | Tagger(slot) => slot := Obj.obj(f)
    | _ => invalid_arg("tea.dom: tagger patch on non-tagger")
    };
    m;
  | Diff.Thunk(_) => m /* Walked separately, in its own index space. */
  | Diff.Append(vs) =>
    let dom = node(m);
    let fresh = Array.map(v => build(m.env, m.scope, v), vs);
    Array.iter(c => append(dom, node(c)), fresh);
    m.kids = Array.append(m.kids, fresh);
    m;
  | Diff.Remove_last(n) =>
    let kids = m.kids;
    let keep = Array.length(kids) - n;
    if (keep == 0) {
      clear_children(m, kids);
      m.kids = [||];
    } else {
      let dom = node(m);
      for (i in keep to Array.length(kids) - 1) {
        let c = kids[i];
        remove(dom, node(c));
        cleanup(c);
      };
      m.kids = Array.sub(kids, 0, keep);
    };
    m;
  | Diff.Reorder(placements) =>
    reorder(m, placements);
    m;
  };
/* Patch walk over a sorted plan in old pre-order index space. Children are
   patched before their parent's own operations (postorder), so references
   are resolved before parent mutations; untouched spans are skipped using
   descendant counts. Recursion is bounded: deep subtrees continue on an
   explicit stack ([walk_stack]). */
type cursor = {
  plan: array(Diff.patch(Obj.t)),
  mutable at: int,
};
let pending = (c, limit) =>
  c.at < Array.length(c.plan)
  && Array.unsafe_get(c.plan, c.at).Diff.index <= limit;
let take = (c, index) => {
  let ops = ref([]);
  while (c.at < Array.length(c.plan)
         && Array.unsafe_get(c.plan, c.at).Diff.index == index) {
    ops := [Array.unsafe_get(c.plan, c.at).Diff.operation, ...ops^];
    c.at = c.at + 1;
  };
  List.rev(ops^);
};
let finish = (m, ops) => {
  let current = List.fold_left(operate, m, ops);
  refresh_count(current);
  current;
};
let rec walk_at = (depth, m, index, c) =>
  if (!pending(c, index + m.descendants)) {
    m;
  } else if (depth > max_depth) {
    walk_stack(m, index, c);
  } else {
    let ops = take(c, index);
    switch (m.kind) {
    | Thunk =>
      List.iter(
        fun
        | Diff.Thunk(inner) =>
          m.kids[0] =
            walk_at(
              depth + 1,
              m.kids[0],
              0,
              {
                plan: inner,
                at: 0,
              },
            )
        | _ => (),
        ops,
      )
    | _ =>
      let kids = m.kids;
      let child = ref(index + 1);
      let i = ref(0);
      let n = Array.length(kids);
      while (i^ < n && pending(c, index + m.descendants)) {
        let k = Array.unsafe_get(kids, i^);
        let start = child^;
        child := start + 1 + k.descendants;
        if (pending(c, child^ - 1)) {
          Array.unsafe_set(kids, i^, walk_at(depth + 1, k, start, c));
        };
        incr(i);
      };
    };
    switch (ops) {
    | [] =>
      refresh_count(m);
      m;
    | ops => finish(m, ops)
    };
  }
and walk_stack = (m, index, c) => {
  let result = ref(m);
  let jobs = ref([]);
  let push = j => jobs := [j, ...jobs^];
  let rec walk = (m, index, c, done_) =>
    push(() =>
      if (!pending(c, index + m.descendants)) {
        done_(m);
      } else {
        let ops = take(c, index);
        push(() => done_(finish(m, ops)));
        switch (m.kind) {
        | Thunk =>
          List.iter(
            fun
            | Diff.Thunk(inner) =>
              walk(
                m.kids[0],
                0,
                {
                  plan: inner,
                  at: 0,
                },
                k => m.kids[0] = k)
            | _ => (),
            ops,
          )
        | _ =>
          let child_index = ref(index + 1);
          let positions =
            Array.map(
              k => {
                let p = child_index^;
                child_index := p + 1 + k.descendants;
                p;
              },
              m.kids,
            );
          for (j in Array.length(m.kids) - 1 downto 0) {
            walk(m.kids[j], positions[j], c, k => m.kids[j] = k);
          };
        };
      }
    );
  walk(m, index, c, k => result := k);
  let rec loop = () =>
    switch (jobs^) {
    | [] => ()
    | [j, ...rest] =>
      jobs := rest;
      j();
      loop();
    };
  loop();
  result^;
};
let apply = (dom, plan) => {
  incr(patches);
  let root = root(dom);
  let root =
    walk_at(
      0,
      root,
      0,
      {
        plan: (Obj.magic(plan): array(Diff.patch(Obj.t))),
        at: 0,
      },
    );
  sync_controls(root.env);
  attach_root(root);
};
