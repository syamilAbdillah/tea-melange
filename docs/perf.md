# DOM performance — P2

Measured with Node 22.22.2, jsdom 30.1.3, OCaml 5.4.1, Melange 6.0.1-54,
Dune 3.23.1, Reason 3.17.3. These are jsdom measurements, not browser claims.
Implementation is original Reason; no melange-tea code or fake DOM was used.

Commands (after repository-root `npm ci`):

```
dune build @runtest --force
dune build @bench/dom/bench --force
```

Recorded post-format runs:

| Gate | Result | Budget |
| --- | ---: | ---: |
| Diff + patch, 1 changed row / 1,000 keyed rows; 1,000 samples, median | 0.5243 ms | <2 ms |
| Initial 1,000-row mount during DOM test process | 20.3273 ms | <40 ms |
| Independent fresh-process cold 1,000-row mount | 30.1968 ms | <40 ms |
| Independent diff + patch median, 1,000 samples | 0.4218 ms | <2 ms |
| 100 messages in one tick | 1 initial rAF request, 1 patch | 1 patch |
| Recreated nodes on single-row update | 0 (all row and text identities checked) | 0 |
| Recreated rows on keyed reversal | 0 (all 1,000 identities checked) | 0 |
| Listener growth over 1,000 alternating patches | 0 (1,000 before / 1,000 after) | 0 |
| Active managed listeners after all tests dispose/shutdown | 0 | 0 |

These final measurements followed `dune clean`, formatting and a full rebuild.
The DOM tests enforce both time budgets and all identity/listener/batching gates. The standalone benchmark reports numbers
without asserting a host-dependent cold-start time; its measured cold mount also
met the budget. Rows have an event listener, so handler-slot updates are included.

Timing includes Diff.diff and Patch.apply, not view construction. Views are
prepared in advance and alternate the changed row in each sample. Other rows
are physically shared. Mount timing includes real DOM creation, facts and
listener setup, not jsdom process/bootstrap initialization.

The runtime deliberately requests a spare rAF **inside** the drawing frame.
Thus the tick schedules one frame, drawing schedules one spare, and flushing the
spare adds zero patches (see docs/runtime-contract.md). Tests assert both counts.

Patch resolves old-index references in a single span-skipping shadow/DOM walk,
then executes child operations before parent placement mutations. Untouched
subtrees are skipped using cached descendant counts. Controlled form properties
are separately indexed per root and restored after every apply, including empty
plans: user edits cannot leave value/checked/selected inconsistent with an
unchanged model. This adds O(number of controlled elements), not a second full
DOM traversal, and does not affect the keyed-list benchmark (no form controls).
