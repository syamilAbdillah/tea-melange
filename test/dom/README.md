# DOM backend tests

Prerequisites: the opam default switch described in `docs/plan.txt`, Node 22,
and `npm install` (or `npm ci`) **at the repository root**. jsdom is the only
npm development dependency; Melange supplies its own emitted JS runtime.

Run `dune build @runtest`. `test/dom/dune` emits ES modules into `js/`, including
an emitted `node_modules/` tree for tea libraries and the Melange runtime. The
runtest action depends on the entire local `melange` alias, **not just the main
emitted file**: otherwise runtime/library imports can be missing on clean builds.

The action runs `node setup.js js/test/dom/test_dom.js` from Dune's test directory.
`setup.js` installs jsdom globals and a manually flushable rAF queue, then imports
the emitted Reason test via an absolute file URL. Node's normal ancestor lookup
finds the repository-root `node_modules/jsdom`, including when the entry point is
under `_build/default`. There is no NODE_PATH override, loader, generated JS patch,
or npm dependency on melange.js. Both ordinary execution and Dune's
`--sandbox=symlink` mode have been verified. `setup.js` contains bootstrapping only;
all assertions, random generators and measurements are Reason.

Coverage:

- Every Attr helper's attribute/property category, typed property resets, input,
  textarea/select value, checked/selected after user edits even with an empty plan.
- CSSOM removal/custom properties, boolean properties/attributes, Attr.none,
  SVG namespaces and qualified xlink removal.
- Raw-event decoder interpretation (prototype getters included), array-likes,
  all handler kinds, default prevention, propagation stopping, listener removal,
  and changing nested Html.map slots while retaining the old handler/DOM.
- 500 seeded random pairs checked against Patch_sim/Naive **and** fresh DOM
  rendering. Canonical serialization ignores the empty style attribute CSSOM
  leaves after the final declaration is removed; form properties are checked
  separately rather than inferred from HTML serialization.
- Nested thunk plans, keyed moves with nested redraws, and 5,000-deep tagger/thunk
  scopes without JavaScript stack overflow.
- App beginner/element/document lifecycle, synchronous initialization, init
  commands, body identity, shutdown cleanup and manual-frame batching.
- Browser scheduler's setTimeout and Date.now fallbacks.
- Strict structural gates: unchanged row/text identities, keyed reorder identity,
  and zero listener growth across 1,000 patches. The 1,000-sample median diff+patch
  and 1,000-row mount timings are reported, not asserted: shared CI host load can
  exceed the <2 ms / <40 ms targets without a correctness regression. See
  `docs/perf.md` for benchmark methodology and recorded budget measurements.

Animator semantics follow the runtime contract: a tick of N messages schedules
one rAF; that frame patches once and schedules one **spare** rAF. Flushing the
spare frame does not patch. This is not a claim of only one rAF for the entire
lifecycle.

Independent cold-mount benchmark: `dune build @bench/dom/bench --force`.
See `docs/perf.md` for recorded measurements.
