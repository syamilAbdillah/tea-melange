# Accessible headless UI

`tea.ui` supplies semantics, keyboard behavior and presence data attributes;
your application owns state and presentation. It depends on `tea.browser` for
post-render DOM effects and lifecycle-owned subscriptions. The umbrella exposes
`Tea.Ui`, `Tea.Browser_dom` and `Tea.Browser_events` as module aliases.

## Build, test, run

Use the repository's opam switch, Node 22 and root `npm ci`.

```sh
dune build
dune build @runtest --force
dune build @doc
npx playwright-core install --with-deps chromium
npm run test:e2e
node scripts/serve.js ui/button
```

Open http://127.0.0.1:8000/. Examples are built by
`dune build @examples/melange`. For an existing Chromium installation:

```sh
TEA_E2E_CHROMIUM=/usr/bin/google-chrome npm run test:e2e
npm run test:e2e -- button
TEA_E2E_SPEC=browser npm run test:e2e
```

The no-ML and formatting gates are `./scripts/check-no-ml.sh` and
`./scripts/check-fmt.sh`. Format changed Reason sources with `refmt --in-place`.

## Conventions

[spec.md](spec.md) is normative. In particular:

- One module and documented interface per component. The library discovers new
  modules automatically; never edit its Dune stanza to add a component.
- Stateless parts are attribute bundles in the parent's message type. Stateful
  messages carry intent; update derives results from the latest model.
- Collections stay in the parent and use stable keys, never array indices.
- Apply each part's owned event handlers once. Compose through documented hooks.
- Use document-unique id prefixes, private `Ui_attr` derivation and explicit
  ARIA idrefs. Presence data flags carry styling state.
- Prefer native controls. Disabled and read-only differ; no CSS is required for
  correctness. Preserve Tab, browser shortcuts and IME composition.
- Effects are lazy and deferred through the shared frame scheduler. Runtime
  updates request their draw before executing commands and subscription effects.
- Subscription identity is `(home,key)`; kept sources receive current taggers.
  Return `Sub.none` or shut down to dispose. Flush the closing render before
  asserting restored focus/styles.

## Components

| Component | Model | Documentation | Example |
| --- | --- | --- | --- |
| Button | Parent-owned | [Button](button.md) | `ui/button` |

Foundation APIs and implementation decisions: [foundation.md](foundation.md).
Test conventions: [jsdom](../../test/ui/README.md),
[Chromium](../../test/ui-e2e/README.md),
[examples](../../examples/ui/README.md).
