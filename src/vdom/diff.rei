/** A destination child either reuses an old child position or is freshly
    rendered. Unreferenced old positions are removed. [Reuse] is a move when
    its old position differs from the destination; resolve all sources before
    mutating the DOM. Duplicate keys fall back to positional diffing. */
type placement('msg) =
  | Reuse(int)
  | Insert(Vdom.t('msg));
/** Operations at an OLD-tree pre-order index. Taggers count as a node with
    one child; thunks count as a leaf in their containing plan. [Thunk] holds
    a separate plan starting at index zero over the old forced subtree.
    [Tagger] replaces a message-map slot, not the real DOM node. Its erased
    function has the existential input type of the corresponding Vdom tagger;
    only the backend's tagger machinery may cast it back.
    Apply child patches before Append/Remove_last/Reorder, using old indices.
    Redraw is terminal: there are no deeper patches for that subtree. */
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
/** Pure diff, apart from memoising forced thunks. Sorted pre-order patches
    allow an addDomNodes-style single walk, skipping untouched subtrees with
    Vdom.descendants_count. Physical node identity skips all work. Uses an
    explicit work stack at every depth (including beyond 512). */
let diff: (Vdom.t('msg), Vdom.t('msg)) => array(patch('msg));
