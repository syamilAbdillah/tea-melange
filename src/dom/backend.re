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
  controlled: Hashtbl.t(int, mounted),
  mutable next_id: int,
}
and mounted = {
  mutable dom: Dom.node,
  mutable kind,
  mutable kids: array(mounted),
  mutable descendants: int,
  scope: list(slot),
  env: environment,
  id: int,
  mutable facts: list(Vdom.fact(Obj.t)),
  events: Hashtbl.t(string, listener),
}
and listener = {
  mutable handler: Vdom.handler(Obj.t),
  callback: raw => unit,
};
let listeners = ref(0);
let patches = ref(0);
/* Wrapper roots are refreshed postorder when their child root changes. */
let node = m => m.dom;
let controlled = facts =>
  List.exists(
    f =>
      switch (f) {
      | Vdom.Property("value" | "checked" | "selected", _) => true
      | _ => false
      },
    facts,
  );
let set_property = (dom, name, v) =>
  switch (v) {
  | Vdom.String(s) => put(dom, name, s)
  | Vdom.Bool(b) => put(dom, name, b)
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
      route(m, message);
    };
  switch (handler) {
  | Vdom.Normal(d) =>
    emit(Result.map(x => (x, false, false), Dom_decode.run(d, event)))
  | Vdom.May_stop_propagation(d) =>
    emit(Result.map(((x, b)) => (x, b, false), Dom_decode.run(d, event)))
  | Vdom.May_prevent_default(d) =>
    emit(Result.map(((x, b)) => (x, false, b), Dom_decode.run(d, event)))
  | Vdom.Custom(d) =>
    emit(
      Result.map(
        x => (x.Vdom.message, x.stop_propagation, x.prevent_default),
        Dom_decode.run(d, event),
      ),
    )
  };
};
let detach = (m, name) =>
  switch (Hashtbl.find_opt(m.events, name)) {
  | None => ()
  | Some(l) =>
    unlisten(m.dom, name, l.callback);
    Hashtbl.remove(m.events, name);
    decr(listeners);
  };
let local_name = s =>
  switch (String.split_on_char(':', s)) {
  | [_prefix, name] => name
  | _ => s
  };
let changes = (m, changes) => {
  List.iter(
    change =>
      switch (change) {
      | Facts.Set(fact) =>
        switch (fact) {
        | Vdom.Attribute("", _) => ()
        | Vdom.Attribute(k, v) => attribute(m.dom, k, v)
        | Vdom.Attribute_ns(ns, k, v) => attribute_ns(m.dom, ns, k, v)
        | Vdom.Property(k, v) => set_property(m.dom, k, v)
        | Vdom.Style(k, v) => style_set(style(m.dom), k, v)
        | Vdom.Event(name, handler) =>
          switch (Hashtbl.find_opt(m.events, name)) {
          | Some(l) => l.handler = handler
          | None =>
            let cell = ref(None);
            let callback = event =>
              switch (cell^) {
              | None => ()
              | Some(l) => fire(m, l.handler, event)
              };
            let l = {
              handler,
              callback,
            };
            cell := Some(l);
            Hashtbl.add(m.events, name, l);
            listen(m.dom, name, callback);
            incr(listeners);
          }
        }
      | Facts.Remove(key) =>
        switch (key) {
        | Facts.Attribute("") => ()
        | Facts.Attribute(k) => remove_attribute(m.dom, k)
        | Facts.Attribute_ns(ns, k) =>
          remove_attribute_ns(m.dom, ns, local_name(k))
        | Facts.Style(k) => style_remove(style(m.dom), k)
        | Facts.Event(k) => detach(m, k)
        | Facts.Property(k) =>
          let previous =
            List.find_opt(f => Facts.key(f) == Facts.Property(k), m.facts);
          switch (previous) {
          | Some(Vdom.Property(_, Vdom.Bool(_))) => put(m.dom, k, false)
          | Some(Vdom.Property(_, Vdom.String(_))) => put(m.dom, k, "")
          | _ => put(m.dom, k, Js.undefined)
          };
        }
      },
    changes,
  );
  m.facts = Facts.apply(m.facts, changes);
  if (controlled(m.facts)) {
    Hashtbl.replace(m.env.controlled, m.id, m);
  } else {
    Hashtbl.remove(m.env.controlled, m.id);
  };
};
let make = (env, scope, kind, dom, descendants) => {
  let id = env.next_id;
  env.next_id = id + 1;
  {
    dom,
    kind,
    kids: [||],
    descendants,
    scope,
    env,
    id,
    facts: [],
    events: Hashtbl.create(0),
  };
};
/* Construction uses an explicit stack too: wrappers never allocate DOM nodes. */
let build = (~root_dom=?, env, scope, vdom) => {
  let root_dom = ref(root_dom);
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
    let dom =
      switch (root_dom^) {
      | Some(dom) =>
        root_dom := None;
        dom;
      | None =>
        switch (ns) {
        | None => element(document, tag)
        | Some(ns) => element_ns(document, ns, tag)
        }
      };
    let m = make(env, scope, Element, dom, descendants);
    put(dom, "__tea_element", m);
    /* Facts are applied after children so select.value sees its options. */
    let children = Array.make(Array.length(kids), m);
    push(() => {
      m.kids = children;
      Array.iter(child => append(dom, node(child)), children);
      changes(m, Facts.diff([], facts));
      done_(m);
    });
    for (i in 0 to Array.length(kids) - 1) {
      let j = Array.length(kids) - 1 - i;
      schedule(scope, kids[j], child => children[j] = child);
    };
  };
  schedule(scope, vdom, m => result := Some(m));
  while (jobs^ != []) {
    switch (jobs^) {
    | [] => ()
    | [job, ...rest] =>
      jobs := rest;
      job();
    };
  };
  Option.get(result^);
};
let cleanup = m => {
  let jobs = ref([m]);
  while (jobs^ != []) {
    switch (jobs^) {
    | [] => ()
    | [m, ...rest] =>
      jobs := rest;
      Hashtbl.iter(
        (name, l) => {
          unlisten(m.dom, name, l.callback);
          decr(listeners);
        },
        m.events,
      );
      Hashtbl.clear(m.events);
      Hashtbl.remove(m.env.controlled, m.id);
      Array.iter(child => jobs := [child, ...jobs^], m.kids);
    };
  };
};
let attach_root = m => {
  let dom = node(m);
  put(dom, "__tea_root", m);
  dom;
};
let root = dom => {
  let value = get(dom, "__tea_root");
  if (Js.testAny(value)) {
    invalid_arg("tea.dom: expected a Render.create root");
  };
  (cast(value): mounted);
};
let create = (dispatch, vdom) => {
  let env = {
    dispatch: Obj.magic(dispatch),
    controlled: Hashtbl.create(0),
    next_id: 0,
  };
  attach_root(build(env, [], Obj.magic(vdom)));
};
/* Document owns the existing body, not an extra wrapper or a replacement body. */
let create_in = (dispatch, vdom, mount) => {
  let env = {
    dispatch: Obj.magic(dispatch),
    controlled: Hashtbl.create(0),
    next_id: 0,
  };
  set_text(mount, "");
  attach_root(build(~root_dom=mount, env, [], Obj.magic(vdom)));
};
let refresh_count = m => {
  switch (m.kind) {
  | Tagger(_)
  | Thunk => m.dom = node(m.kids[0])
  | Element
  | Text => ()
  };
  m.descendants = (
    switch (m.kind) {
    | Text
    | Thunk => 0
    | Element
    | Tagger(_) =>
      Array.fold_left((n, c) => n + 1 + c.descendants, 0, m.kids)
    }
  );
};
let sync_controls = env =>
  Hashtbl.iter(
    (_, m) =>
      List.iter(
        f =>
          switch (f) {
          | Vdom.Property(("value" | "checked" | "selected") as k, v) =>
            set_property(m.dom, k, v)
          | _ => ()
          },
        m.facts,
      ),
    env.controlled,
  );
/* A sorted-plan cursor and subtree spans perform one old-index walk.
   Postorder jobs mutate only after references for their children are resolved. */
let apply = (dom, plan) => {
  incr(patches);
  let root_ref = ref(root(dom));
  let jobs = ref([]);
  let push = j => jobs := [j, ...jobs^];
  let rec plan_job = (m, plan, done_) => {
    let cursor = ref(0);
    walk(m, 0, plan, cursor, done_);
  }
  and walk = (m, index, plan, cursor, done_) =>
    push(() => {
      let limit = index + m.descendants;
      if (cursor^ >= Array.length(plan) || plan[cursor^].Diff.index > limit) {
        done_(m);
      } else {
        let ops = ref([]);
        while (cursor^ < Array.length(plan)
               && plan[cursor^].Diff.index == index) {
          ops := [plan[cursor^].Diff.operation, ...ops^];
          incr(cursor);
        };
        let ops = List.rev(ops^);
        let current = ref(m);
        push(() => {
          List.iter(
            op => {
              let m = current^;
              switch (op) {
              | Diff.Redraw(v) =>
                let old = node(m);
                let next = build(m.env, m.scope, v);
                switch (Js.Nullable.toOption(parent(old))) {
                | None => ()
                | Some(p) => replace(p, node(next), old)
                };
                cleanup(m);
                current := next;
              | Diff.Text(s) => set_text(node(m), s)
              | Diff.Facts(cs) => changes(m, cs)
              | Diff.Tagger(f) =>
                switch (m.kind) {
                | Tagger(slot) => slot := Obj.obj(f)
                | _ => invalid_arg("tea.dom: tagger patch on non-tagger")
                }
              | Diff.Thunk(_) => () /* Scheduled below, in its own index space. */
              | Diff.Append(vs) =>
                let fresh = Array.map(v => build(m.env, m.scope, v), vs);
                Array.iter(c => append(node(m), node(c)), fresh);
                m.kids = Array.append(m.kids, fresh);
              | Diff.Remove_last(n) =>
                let keep = Array.length(m.kids) - n;
                for (i in keep to Array.length(m.kids) - 1) {
                  let c = m.kids[i];
                  remove(node(m), node(c));
                  cleanup(c);
                };
                m.kids = Array.sub(m.kids, 0, keep);
              | Diff.Reorder(placements) =>
                let sources = m.kids;
                let used = Array.make(Array.length(sources), false);
                /* Snapshot every source handle before changing the child list. */
                let destinations =
                  Array.map(
                    p =>
                      switch (p) {
                      | Diff.Reuse(i) =>
                        used[i] = true;
                        sources[i];
                      | Diff.Insert(v) => build(m.env, m.scope, v)
                      },
                    placements,
                  );
                Array.iteri(
                  (i, c) =>
                    if (!used[i]) {
                      remove(node(m), node(c));
                      cleanup(c);
                    },
                  sources,
                );
                Array.iteri(
                  (i, c) => {
                    let dom_kids = children(node(m));
                    let at =
                      if (i < Array.length(dom_kids)) {
                        Js.Nullable.return(dom_kids[i]);
                      } else {
                        Js.Nullable.null;
                      };
                    let already_here =
                      switch (Js.Nullable.toOption(at)) {
                      | Some(at) => at === node(c)
                      | None => false
                      };
                    if (!already_here) {
                      insert(node(m), node(c), at);
                    };
                  },
                  destinations,
                );
                m.kids = destinations;
              };
            },
            ops,
          );
          refresh_count(current^);
          done_(current^);
        });
        switch (m.kind) {
        | Thunk =>
          List.iter(
            op =>
              switch (op) {
              | Diff.Thunk(inner) =>
                plan_job(m.kids[0], inner, c => m.kids[0] = c)
              | _ => ()
              },
            ops,
          )
        | _ =>
          let child_index = ref(index + 1);
          let positions =
            Array.map(
              c => {
                let p = child_index^;
                child_index := p + 1 + c.descendants;
                p;
              },
              m.kids,
            );
          for (i in 0 to Array.length(m.kids) - 1) {
            let j = Array.length(m.kids) - 1 - i;
            walk(m.kids[j], positions[j], plan, cursor, c => m.kids[j] = c);
          };
        };
      };
    });
  plan_job(root_ref^, Obj.magic(plan): array(Diff.patch(Obj.t)), m =>
    root_ref := m
  );
  while (jobs^ != []) {
    switch (jobs^) {
    | [] => ()
    | [j, ...rest] =>
      jobs := rest;
      j();
    };
  };
  sync_controls(root_ref^.env);
  attach_root(root_ref^);
};
