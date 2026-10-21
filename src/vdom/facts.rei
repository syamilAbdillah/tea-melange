/** Separate DOM namespaces for fact removal. */
type key =
  | Attribute(string)
  | Attribute_ns(string, string)
  | Property(string)
  | Style(string)
  | Event(string);
/** [Set(Event(...))] updates an existing listener's mutable handler slot;
    only an absent listener needs attaching. [Remove(Event(...))] detaches it. */
type change('msg) =
  | Set(Vdom.fact('msg))
  | Remove(key);
/** Identity of a fact, including its namespace. */
let key: Vdom.fact('msg) => key;
/** Closure-safe fact comparison; event handlers use physical identity. */
let same: (Vdom.fact('msg), Vdom.fact('msg)) => bool;
/** Explicit sets and removals. Duplicate fact keys use the last occurrence. */
let diff:
  (list(Vdom.fact('msg)), list(Vdom.fact('msg))) => list(change('msg));
/** Pure interpretation of fact updates. Fact order is unspecified. */
let apply:
  (list(Vdom.fact('msg)), list(change('msg))) => list(Vdom.fact('msg));
/** Order-independent, last-occurrence-wins equality. */
let equal: (list(Vdom.fact('msg)), list(Vdom.fact('msg))) => bool;
