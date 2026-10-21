# Pure virtual DOM patch plans (P1)

Written from scratch from the public Elm VirtualDom algorithm description;
no melange-tea source was copied or ported.

## Backend contract

- Every patch index addresses the **old** virtual tree. Plans are sorted by
  pre-order index; multiple operations may share an index.
- A backend can collect DOM references in one old-tree walk, skipping spans
  with `Vdom.descendants_count` when no patch lies in them. Collect references
  before changing child order or replacing any DOM nodes.
- Taggers occupy a virtual index but have no DOM wrapper. Their child starts
  at the next index and addresses the same DOM root with a nested message-map
  scope. A `Tagger` patch replaces the function in that scope. Existential
  message erasure is confined to the tagger cases in Diff and Naive; the
  erased function in a patch must only be used by message-scope machinery.
- Thunks occupy one virtual index in the outer plan. A `Thunk` operation opens
  a separate zero-based plan over the old forced tree, also sharing its real
  DOM root. Even nested thunk plans are processed with explicit stacks.
- Resolve all `Reorder(Reuse(old_position))` sources before mutation. Patch
  retained children using old indices **before** applying the parent's child
  operations. Every retained source is referenced at most once. Unreferenced
  sources are removed; `Insert` alone allocates a fresh child. This handles
  insertion, deletion, adjacent swaps, and arbitrary permutations in O(n).
- Duplicate keys in either sibling array disable key matching for that pair:
  children use positional common-prefix diff, then append/remove-last. This
  intentionally makes no key-identity guarantee for duplicate keys.
- A facts `Set(Event(name, handler))` updates the listener's mutable handler
  slot, attaching a listener only if none exists. This works across handler
  variants as well as decoder changes. `Remove(Event(name))` detaches it.
  Other removal keys distinguish attributes, namespaced attributes, properties
  and styles; the browser backend chooses their appropriate unset/reset action.
  Duplicate fact keys have last-occurrence-wins semantics.

Diff, full reference rendering, equality and simulated patch application use
explicit work stacks at **all** depths, rather than introducing a second path
only at depth 512. This avoids a platform-dependent recursive threshold.

## Native verification

`dune build @runtest` runs 10,000 fixed-seed random old/new pairs, targeted
operation/fact-removal/thunk-cache tests, identity checks for keyed moves and a
1,000-row single update, and separate 5,000-deep element/tagger/thunk cases.

`dune exec bench/vdom/bench_vdom.exe` measures diff plus simulated application
for a one-row change in 1,000 keyed rows (1,000 samples, not in runtest).
A post-format native run measured **0.5381 ms median**, with zero new identities.
This is a simulator/native measurement, not a browser benchmark.
