# Browser examples

Build all three with the existing opam switch (no bundler or npm build step):

```sh
dune build @examples/melange
node scripts/serve.js counter 8000
# or: node scripts/serve.js todo 8000
# or: node scripts/serve.js fetch_routing 8000
```

Open http://127.0.0.1:8000/. Stop the server before switching examples.
Each server serves its complete `_build/default/examples/NAME/js/` emission:
the copied HTML, ES modules, library/runtime modules, and static JSON.
The HTML's import map resolves Melange's package imports directly to the emitted
node_modules tree; no CDN, loader, generated-JS rewrite, or bundler is needed.
It works independently of the shell's working directory. Extensionless paths
fall back to the example's HTML, so /users/1 can be reloaded directly.
Missing module/assets return 404, not HTML.

- **counter**: the plan's beginner sample after `open Tea;`, using Elm's `update(msg, model)` order.
- **todo**: an element program; click “Seed 1,000 items” for the keyed-list
  demo, or start adding to the empty model. Add with the form,
  edit inline, toggle checkboxes, delete, and filter All/Active/Completed.
  Stable IDs feed `Keyed.ul`; stable row records feed `Html.lazy1`.
- **fetch_routing**: an application program with /, /users, /users/:id and
  NotFound routes parsed by `Url.Parser`. `Http.get` decodes the local
  `users.json`, with loading, failure/retry, and unknown-user states.
  Navigation uses real links and history, not hash routes.

Run the jsdom interaction smoke tests:

```sh
dune build @test/examples/runtest
dune build @test/examples/runtest --sandbox=symlink --force
```
