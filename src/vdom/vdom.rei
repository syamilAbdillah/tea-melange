/** Virtual DOM nodes: plain values produced by a pure [view].

    Five cases (ADR-01): [Text], [Node], [Keyed], [Thunk] (lazy) and
    [Tagger] (message map). Every element-like node caches
    [descendants_count] — the number of nodes below it in pre-order
    (Elm semantics: each child counts 1 + its own descendants; a [Tagger]
    counts 1 + its child's descendants; a [Thunk] counts as 0 until forced,
    the diff handles it specially). Patch application uses the count to skip
    subtrees without patches. Construct nodes only through the functions
    below so the count stays correct. */

/** Event handler: what to decode from the DOM event, as data. */
type handler('msg) =
  | Normal(Tea_core.Decode.t('msg))
  /** decoded bool = stopPropagation */
  | May_stop_propagation(Tea_core.Decode.t(('msg, bool)))
  /** decoded bool = preventDefault */
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

/** Facts: everything set on an element. The DOM backend owns the mapping;
    in particular [value], [checked], [selected] must be [Property]. */
type fact('msg) =
  | Attribute(string, string)
  | Attribute_ns(string, string, string) /* namespace, name, value */
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
  /** Physical identity of the view function + its arguments. The thunk is
      skipped iff both arrays have equal length and every element is [==]. */
  refs: array(Obj.t),
  render: unit => t('msg),
  /** Filled by the diff/renderer the first time the thunk is forced. */
  mutable node: option(t('msg)),
}
and tagger('a, 'msg) = {
  tagger: 'a => 'msg,
  child: t('a),
  /** Same meaning as [element.descendants_count]. */
  descendants: int,
};

let text: string => t('msg);
let node: (~namespace: string=?, string, list(fact('msg)), list(t('msg))) => t('msg);
let keyed:
  (~namespace: string=?, string, list(fact('msg)), list((string, t('msg)))) => t('msg);
let map: ('a => 'msg, t('a)) => t('msg);

let lazy1: ('a => t('msg), 'a) => t('msg);
let lazy2: (('a, 'b) => t('msg), 'a, 'b) => t('msg);
let lazy3: (('a, 'b, 'c) => t('msg), 'a, 'b, 'c) => t('msg);

/** Force a thunk (memoised in [thunk.node]). */
let force: thunk('msg) => t('msg);

/** Pre-order count of nodes strictly below [t] (0 for text). */
let descendants_count: t('msg) => int;

/** Fact helpers. */
let attribute: (string, string) => fact('msg);
let attribute_ns: (string, string, string) => fact('msg);
let property: (string, prop_value) => fact('msg);
let style: (string, string) => fact('msg);
let on: (string, handler('msg)) => fact('msg);
let map_fact: ('a => 'b, fact('a)) => fact('b);
let map_handler: ('a => 'b, handler('a)) => handler('b);
