# tea-melange · 0.1.0

The Elm Architecture for Melange, written in Reason: one model, one pure update,
one pure view, and effects that return messages. A small keyed/lazy virtual DOM
patches the browser once per animation frame. The pure core also runs natively,
so application logic and views can be tested without a browser or bundler.

## Install

Requires OCaml 5.4, Dune ≥3.18, Reason ≥3.17 and Melange 6.0.1-54.
The tested toolchain is recorded in [the plan](docs/plan.txt).

```sh
opam pin add tea git+https://github.com/syamilAbdillah/tea-melange.git
```

Use `(libraries tea)` and `(preprocess (pps melange.ppx))` in your
`melange.emit` stanza. `Tea` is the browser-facing umbrella; individual libraries
remain available for smaller dependency graphs. `tea.test` is native-only.

## A complete counter

Mount into `<div id="app"></div>`. This is the plan's counter, verbatim after
`open Tea;`, and is compiled as `examples/counter/main.re`.

```reason
open Tea;

/* counter.re */
type msg =
  | Increment
  | Decrement
  | Set(int);

let update = (model, msg) =>
  switch (msg) {
  | Increment => model + 1
  | Decrement => model - 1
  | Set(n) => n
  };

let view = model =>
  Html.div(
    [],
    [
      Html.button([Html.on_click(Decrement)], [Html.text("−")]),
      Html.span([], [Html.text(string_of_int(model))]),
      Html.button([Html.on_click(Increment)], [Html.text("+")]),
    ],
  );

let () = App.beginner(~init=0, ~update, ~view, ~node="#app", ());
```

`beginner` returns unit for the original sample. Use `element`, `document`, or
`application` for effectful programs and an explicit runtime handle with
`dispatch`, `model`, and idempotent `shutdown`. The lower-level
`Tea_dom.App.beginner` also returns a handle when needed.

## Build and run

From a checkout with the opam switch active and Node 22:

```sh
npm ci
./scripts/check-no-ml.sh
./scripts/check-fmt.sh
dune build
dune build @runtest --force
dune build @doc
dune build @bench
```

Examples emit browser ES modules, their Melange runtime, and static assets.
Melange emits bare package imports as well as relative imports: each HTML page
includes a native browser import map pointing at its emitted `node_modules/`
tree. A root base URL keeps module/asset resolution correct on deep SPA routes.
No bundler or npm runtime dependency is required. Run one example at a time:

```sh
node scripts/serve.js counter
node scripts/serve.js todo
node scripts/serve.js fetch_routing
```

Open `http://127.0.0.1:8000/` after starting any command above. The server
prints its URL. It serves the emitted dependency tree and falls back
to index.html for non-file routes, so refreshing `/users/1` works. Counter is the
minimal beginner; todo demonstrates controlled forms, keyed lazy rows and a
1,000-item seed; fetch_routing fetches the bundled users.json with explicit
loading/error/success states and routes without reloading. Todo persists nothing.

Generated API docs: `_build/default/_doc/_html/index.html`.

## Module map

| Library | Public modules | Targets |
| --- | --- | --- |
| `tea` | `Tea.App`, all modules below except internals/test utilities | Melange |
| `tea.core` | Cmd, Sub, Task, Decode, Program, Runtime, Platform, Random | native + Melange |
| `tea.vdom` | Vdom, Diff, Facts, Naive, Patch_sim | native + Melange |
| `tea.html` | Html, Attr/Attributes, Events, Keyed, Svg | native + Melange |
| `tea.dom` | App, Browser_platform, Render, Patch, Debug | Melange |
| `tea.http` | Http | Melange |
| `tea.url` | Url (including Parser and Query) | native + Melange |
| `tea.nav` | Navigation, Application | Melange |
| `tea.effects` | Time, Random, Animation_frame | Melange |
| `tea.test` | Render, Query, Simulate | native only |

`Tea.Random` combines the pure seeded generators with effectful `generate` and
`seed_cmd`. `Tea.App.application` integrates navigation without adding a
`tea.nav` dependency to `tea.dom`. Link requests become messages; your update
chooses whether to push a URL or load an external page.

## Principles and decisions

- The model is the only application state. Update and view are pure; commands
  and subscriptions describe effects; messages are the only input.
- **ADR-01:** virtual DOM is the sole renderer; no SSR or hydration in v0.1.
- **ADR-02 (superseded):** do not port melange-tea. Its repository license is
  LGPL despite contradictory opam metadata. All implementations are original;
  see the [license audit](docs/license-audit.md).
- **ADR-03:** Reason `.re`/`.rei` sources only; use Dune's built-in Reason dialect.
- **ADR-04:** Elm-style three-state animation-frame batching, including a spare
  frame. Many messages in one tick produce one patch.
- **ADR-05:** function DSL, not JSX or an additional PPX.
- **ADR-06:** `Runtime.Make(Platform.S)` is a functor, not a Dune virtual module;
  native tests need no browser implementation linked in.

## Further reading

[Technical plan](docs/plan.txt) · [Runtime contract](docs/runtime-contract.md) ·
[Effects and URL semantics](docs/effects.md) · [Performance](docs/perf.md) ·
[DOM test wiring](test/dom/README.md) · [Release checklist](docs/done-checklist.md) ·
[Changes](CHANGES.md) · [MIT license](LICENSE)
