# Benchmarks

After root `npm ci`:

```sh
opam exec -- dune build @bench --force
# Or independently:
opam exec -- dune build @bench/dom/bench @bench/vdom/bench --force
```

The recursive root alias runs native pure-core diff/simulator samples and a
fresh Node process with real jsdom. DOM benchmarks/assertions are Reason;
`setup.js` only bootstraps jsdom, queues deterministic rAF callbacks and observes
actual event-listener registrations independently of library debug counters.

The DOM suite first measures a cold 1,000-row mount in an unprofiled fresh
process (`cold_setup.js` / `cold_mount.re`). A second fresh process reports
profiled mount and the median of 1,000 alternating single-row diff+patch updates. Rows have `data-uid` and click
listeners. Outside timing, every patch checks row/text identity, uid and per-row
listener counts. It also checks disposal and dispatches 100 messages in one tick
through the real app runtime, counting actual patch calls and rAF requests.

Structural/listener/batching regressions fail the action; host-dependent timings
are reported without a hard threshold assertion. See `docs/perf.md` for all five
plan gates, measured results (including misses), reproduction details and limits.
