# Performance — P6

Measured on this machine: Intel Core Ultra 5 125H (18 logical CPUs), Linux
7.2.5-100.fc43.x86_64, Node 22.22.2, jsdom 30.1.3, OCaml 5.4.1,
Melange 6.0.1-54, Dune 3.23.1, Reason 3.17.3, opam `default` switch.
Dune default/dev profile; no toolchain installation, CPU pinning or forced GC.
Other agents were working in the shared checkout, so this is not an isolated
performance-lab result. These are **real jsdom measurements, not browser/layout
or paint measurements**. Previous P2 numbers are superseded, not reasserted.

## All five §performance gates from `docs/plan.txt`

Results below are from the integration **root `@bench`** invocation. Cold mount
runs in its own fresh process without the listener profiler; the second fresh
process verifies identity/listener/batching gates with registration profiling.

| Scenario | Measured result | Target | Outcome |
| --- | --- | --- | --- |
| Diff + patch, 1 changed item / 1,000 keyed rows | **0.4786 ms median**, 1,000 alternating samples | <2 ms | Pass |
| Initial mount, 1,000 keyed rows, single unprofiled cold run | **34.4011 ms** | <40 ms | Pass |
| 100 messages synchronously in one tick | **1 patch**; 1 initial rAF, 2 including idle spare | 1 patch | Pass |
| Nodes recreated during update | **0** row/text replacements; root retained; checked after every patch | 0 | Pass |
| Listener accumulation after 1,000 patches | **0**; 1,000 actual registrations before/after; max per-node growth 0 | 0 | Pass |

All five budgets pass in the final integration run. Cold mount with the actual
listener-registration profiler enabled takes **34.9259 ms** in that run and also
passes; earlier profiled runs exceeded 40 ms. The latency gate uses the same
1,000-row view in a separate unprofiled fresh process, not a warmed-up render or
a best-of-N sample. Cold timing remains host-load sensitive.

## Reproduce

After repository-root `npm ci` (already completed for these runs):

```sh
opam exec -- dune build @bench --force
# Individual aliases, also useful while unrelated directories are under construction:
opam exec -- dune build @bench/dom/bench @bench/vdom/bench --force
```

The recursive root `@bench` selects both `bench/vdom` (native) and `bench/dom`
(Melange + fresh Node/jsdom process). `--force` reruns alias actions even when
artifacts are current; compilation/bootstrap are not included in timed intervals.
`opam exec -- refmt --in-place bench/dom/bench_dom.re` formatted the new Reason
instrumentation before final runs. Both scoped aliases and the final root
`@bench` invocation completed with exit 0. `git diff --check -- bench docs/perf.md`
also passed.

Earlier root runs executed **both** suites but exited 1 while concurrent agents
were creating example Dune rules and `Application_test`. Those out-of-scope
integration blockers were resolved before the final root run; no files outside
`bench/` and this document were edited for this deliverable. Concurrent Dune
processes also rejected two invocations with a build lock; those aborted
invocations produced no samples and are not counted below.

### Recorded runs (no best-run selection)

The original runs below include the registration profiler during cold mount.
The integration run adds `cold_mount.re` + `cold_setup.js` to separate latency
measurement from the structural profiler. Both processes use the same keyed
row shape (data-uid, click listener, text), with no warmup or forced GC.

Each entry is a separate fresh Node process. Native and DOM actions may run in
parallel under Dune. All identity/listener/batching assertions passed in every
completed benchmark process.

| Run | Cold DOM mount (ms) | DOM diff+patch median (ms) | Native diff+simulated apply median (ms) |
| --- | ---: | ---: | ---: |
| First instrumentation run, before formatting | 45.2540 | 0.5206 | 0.7581 |
| Post-format root run 1 | 39.7314 | 0.4857 | 0.5959 |
| Post-format root run 2 | 51.8370 | 0.5424 | 0.8590 |
| Post-format root run 3 | 40.8390 | 0.5398 | 0.7390 |
| Scoped run, exit 0 | 45.4065 | 0.5024 | 0.7555 |
| Agent final root `@bench`, exit 0 | 42.2673 | 0.5195 | 0.6170 |
| Integration root before separating profiler, exit 0 | 50.6959 | 0.5471 | 0.6630 |
| Integration root with separate unprofiled cold process, exit 0 | 41.3942 (profiled); 32.5565 unprofiled | 0.5004 | 0.6411 |
| Final all-checks root run, exit 0 | **34.9259 profiled; 34.4011 unprofiled** | **0.4786** | **0.5380** |

## Methodology and limits

- `bench/dom/cold_mount.re` measures only the first Render.create in an
  unprofiled fresh process, using the same row shape. This is the cold-mount
  latency gate; `cold_setup.js` only installs jsdom globals.
- `bench/dom/bench_dom.re` builds two virtual keyed lists in advance. Every row
  has `data-uid` and a click listener; row 500 changes text. All other row values
  are physically shared. Mount is the first framework render in the process:
  `Render.create` includes DOM creation, attributes, handler setup and bookkeeping,
  but excludes virtual-view construction, Node/jsdom initialization and attachment
  to the document (the rendered root is detached).
- Each of 1,000 samples times only `Diff.diff` + `Patch.apply` with
  `Browser_platform.now()` (`performance.now` here). Old/new views alternate so
  every sample is a real update. No warmup or outliers are removed. The median
  is the mean of sorted samples 499 and 500 (zero-based).
- After **every** timed patch, compare root, all 1,000 row identities, all 1,000
  text-node identities and each `data-uid`. Identity checks and listener scans
  are outside timing, but their allocations/GC can affect subsequent samples.
  Identity observations do not prove zero temporary allocation internally.
- `bench/dom/setup.js` wraps jsdom's actual `addEventListener` and
  `removeEventListener`, tracking callback/type/capture triples per target with
  a WeakMap. Reason checks each row has one listener initially, maximum per-row
  growth over all patches is zero, and total remains 1,000. This is independent
  of the library's debug counter. The tracker is for these persistent click
  listeners, not general `once`/AbortSignal lifecycle instrumentation. Wrapping
  adds some overhead to mount; it is included rather than subtracted.
- Disposal is also checked to remove all actual row registrations. A separate
  real `App.beginner` receives 100 messages before a deterministic queued rAF
  flush. `Debug.patch_count` counts actual `Patch.apply` calls, not view calls.
  One rAF is scheduled by the tick, drawing requests one spare, and flushing
  both yields exactly one patch. Final model is 100; shutdown leaves zero managed
  listeners. No fake DOM or fake patch implementation is used.
- `bench/vdom/bench_vdom.re` is a separate native pure-core diagnostic: 1,000
  diff + `Patch_sim.apply_to_tree` samples, median using `Unix.gettimeofday`.
  Its fresh view rebuilds all rows (unlike the structurally shared DOM case),
  and each sample starts from the same immutable simulated tree. It asserts
  zero recreated simulated nodes. This wall-clock measurement is **not** a DOM
  measurement and is not substituted into the five-gate table.

Timing budgets are reported, not asserted by the standalone benchmark: a slow
or contended host must not turn numbers into an invented pass. Structural,
listener, cleanup and batching regressions fail the benchmark action.
