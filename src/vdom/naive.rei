/** Pure simulated DOM. Wrapper nodes model message scopes and lazy scopes;
    they are not real DOM elements. Identity [uid] is ignored by [equal]. */
module Tree: {
  /** Kind of a simulated node. Tagger functions retain physical identity. */
  type kind =
    | Text(string)
    | Element(string, option(string))
    | Thunk
    | Tagger(Obj.t);
  /** Immutable node. [descendants_count] uses Vdom's indexing convention:
      a thunk's forced child lives in a separate index space. */
  type t('msg) = {
    uid: int,
    kind,
    facts: list(Vdom.fact('msg)),
    kids: array(t('msg)),
    descendants_count: int,
  };
};
/** Independent full renderer, using an explicit stack. Optional starting
    identity makes allocations deterministic without global mutable state. */
let render: (~start_uid: int=?, Vdom.t('msg)) => Tree.t('msg);
/** Iterative semantic equality, including facts/handlers and tagger slots,
    excluding allocation identities and cached indexing counts. */
let equal: (Tree.t('msg), Tree.t('msg)) => bool;
/** Largest identity in the whole tree, including forced thunk scopes. */
let max_uid: Tree.t('msg) => int;
/** Count identities in [new] absent from [old], including wrappers. */
let created: (Tree.t('msg), Tree.t('msg)) => int;
