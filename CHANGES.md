# Changes

## 0.1.0

First release of tea-melange, an original MIT-licensed implementation in Reason.

- Pure model-first TEA runtime: FIFO messages, commands, identity-diffed
  subscriptions, tasks, platform functor and animation-frame batching.
- Stack-safe virtual DOM with keyed reuse, lazy views, message mapping, native
  reference renderer and deterministic property tests.
- HTML/SVG function DSL, typed attributes and decoded event descriptions.
- Browser mount/patch lifecycle: beginner, element, document and application.
- HTTP with typed JSON decoding, URL parser and history navigation, time,
  portable seeded random generators and animation-frame subscriptions.
- `Tea` umbrella, combining pure and effectful Random APIs.
- Runnable counter, 1,000-row keyed todo and fetch/routing examples; native and
  jsdom integration/smoke tests; measured performance budgets and API docs.
- OCaml 5.4 / Melange 6 baseline, Node 22 CI, Reason-only and formatting gates.

No SSR, hydration, JSX, persistence or time-travel debugger in this release.

`update` takes the message first, as in Elm (`update: ('msg, 'model) => …`), so
replaying a log is a plain fold:
`List.fold_left((model, msg) => fst(update(msg, model)), init, log)`.
