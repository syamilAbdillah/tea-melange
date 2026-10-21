# DOM benchmark

Run `npm ci` at the repository root, then:

```
dune build @bench/dom/bench --force
```

This uses the shared jsdom bootstrap in `test/dom/setup.js` and emits/runs
`bench_dom.re` in a fresh Node process. It reports a cold 1,000-row keyed mount
(with one listener per row), followed by 1,000 alternating one-row diff+patch
samples and their median. No fake DOM is substituted. The stricter correctness,
identity, leak and timing assertions run under `dune build @runtest`.
