type handler('msg) =
  | Normal(Tea_core.Decode.t('msg))
  | May_stop_propagation(Tea_core.Decode.t(('msg, bool)))
  | May_prevent_default(Tea_core.Decode.t(('msg, bool)))
  | Custom(Tea_core.Decode.t(custom('msg)))
and custom('msg) = {
  message: 'msg,
  stop_propagation: bool,
  prevent_default: bool,
};

type prop_value =
  | String(string)
  | Bool(bool)
  | Float(float);

type fact('msg) =
  | Attribute(string, string)
  | Attribute_ns(string, string, string)
  | Property(string, prop_value)
  | Style(string, string)
  | Event(string, handler('msg));

type t('msg) =
  | Text(string)
  | Node(element('msg, t('msg)))
  | Keyed(element('msg, (string, t('msg))))
  | Thunk(thunk('msg))
  | Tagger(tagger('a, 'msg)): t('msg)
and element('msg, 'kid) = {
  tag: string,
  namespace: option(string),
  facts: list(fact('msg)),
  kids: array('kid),
  descendants_count: int,
}
and thunk('msg) = {
  refs: array(Obj.t),
  render: unit => t('msg),
  mutable node: option(t('msg)),
}
and tagger('a, 'msg) = {
  tagger: 'a => 'msg,
  child: t('a),
  /** Same meaning as [element.descendants_count]. */
  descendants: int,
};

let descendants_count: type msg. t(msg) => int =
  fun
  | Text(_) => 0
  | Node({descendants_count, _}) => descendants_count
  | Keyed({descendants_count, _}) => descendants_count
  | Thunk(_) => 0
  | Tagger({descendants, _}) => descendants;

let text = s => Text(s);

let node = (~namespace=?, tag, facts, kids) => {
  let kids = Array.of_list(kids);
  let descendants_count =
    Array.fold_left((acc, kid) => acc + 1 + descendants_count(kid), 0, kids);
  Node({tag, namespace, facts, kids, descendants_count});
};

let keyed = (~namespace=?, tag, facts, kids) => {
  let kids = Array.of_list(kids);
  let descendants_count =
    Array.fold_left((acc, (_, kid)) => acc + 1 + descendants_count(kid), 0, kids);
  Keyed({tag, namespace, facts, kids, descendants_count});
};

let map = (tagger, child) =>
  Tagger({tagger, child, descendants: 1 + descendants_count(child)});

let lazy1 = (f, a) =>
  Thunk({refs: [|Obj.repr(f), Obj.repr(a)|], render: () => f(a), node: None});
let lazy2 = (f, a, b) =>
  Thunk({
    refs: [|Obj.repr(f), Obj.repr(a), Obj.repr(b)|],
    render: () => f(a, b),
    node: None,
  });
let lazy3 = (f, a, b, c) =>
  Thunk({
    refs: [|Obj.repr(f), Obj.repr(a), Obj.repr(b), Obj.repr(c)|],
    render: () => f(a, b, c),
    node: None,
  });

let force = thunk =>
  switch (thunk.node) {
  | Some(n) => n
  | None =>
    let n = thunk.render();
    thunk.node = Some(n);
    n;
  };

let attribute = (k, v) => Attribute(k, v);
let attribute_ns = (ns, k, v) => Attribute_ns(ns, k, v);
let property = (k, v) => Property(k, v);
let style = (k, v) => Style(k, v);
let on = (name, h) => Event(name, h);

let map_handler = (f, h) =>
  switch (h) {
  | Normal(d) => Normal(Tea_core.Decode.map(f, d))
  | May_stop_propagation(d) =>
    May_stop_propagation(Tea_core.Decode.map(((m, b)) => (f(m), b), d))
  | May_prevent_default(d) =>
    May_prevent_default(Tea_core.Decode.map(((m, b)) => (f(m), b), d))
  | Custom(d) =>
    Custom(
      Tea_core.Decode.map(
        c =>
          {
            message: f(c.message),
            stop_propagation: c.stop_propagation,
            prevent_default: c.prevent_default,
          },
        d,
      ),
    )
  };

let map_fact = (f, fact) =>
  switch (fact) {
  | Attribute(k, v) => Attribute(k, v)
  | Attribute_ns(ns, k, v) => Attribute_ns(ns, k, v)
  | Property(k, v) => Property(k, v)
  | Style(k, v) => Style(k, v)
  | Event(name, h) => Event(name, map_handler(f, h))
  };
