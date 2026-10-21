/** Interpret an indexed patch plan over an immutable simulated DOM.
    Source indices always refer to [old], never to partially patched children.
    Keyed Reuse preserves uid; insertion/redraw allocates fresh identities.
    Untouched subtrees are shared. The explicit work stack covers all depths,
    including nested thunk plans. No input tree is mutated. */
let apply_to_tree:
  (Naive.Tree.t('msg), array(Diff.patch('msg))) => Naive.Tree.t('msg);
