# Chromium accessibility tests

```sh
npm ci
dune build
npx playwright-core install --with-deps chromium
npm run test:e2e
npm run test:e2e -- button
TEA_E2E_SPEC=button.spec.mjs npm run test:e2e
TEA_E2E_CHROMIUM=/usr/bin/google-chrome npm run test:e2e
```

The executable defaults to Playwright's managed Chromium. No npm runtime
dependencies, bundler, external server, or browser-test runner are needed.
`playwright-core` and `axe-core` are exact-pinned development dependencies.

Add `specs/<name>.spec.mjs`; discovery is automatic. Export
`async function run(context)`. Each spec gets a fresh browser context, reduced
motion enabled and a 10-second operation timeout / 60-second spec watchdog.
Use `openExample(context, name)` to launch the corresponding emitted
`_build/default/examples/ui/<name>/js` example on an ephemeral local port.
All contexts, browsers and servers are closed on success or failure.

`lib.mjs` provides:

- `openExample(context, name)`: returns a mounted page and records script,
  console and HTTP errors (the runner fails on any).
- `frames(page)`: two real rAFs, for draw plus post-render effects.
- `pressKeys(page, ...keys)`: Playwright keys/chords, frames after each.
- `assertFocus(page, selector)`: exact activeElement assertion.
- `ariaSnapshot(locator, expected)`: exact normalized Playwright ARIA YAML.
- `axe(page)`: injects the local pinned axe bundle and fails on **any** violation.
  No disabled rules, impact threshold or ignored nodes.

Cover every keyboard-table row, pointer use, focus/return, ARIA snapshot and
axe in each component state. Avoid fixed sleeps, especially for animation.
`button.spec.mjs` is the reference. `browser.spec.mjs` additionally exercises
the emitted browser primitives with real dialog modality, ResizeObserver,
matchMedia changes, finished/cancelled/removed animations and scroll leases.
Its direct public Sub leaf lifecycle checks complement runtime-managed jsdom
tests; component specs should normally drive examples rather than leaf records.
