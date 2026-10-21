# Example smoke tests

Prerequisites match `test/dom/README.md`: the existing opam switch, Node 22,
and repository-root `npm ci`. No additional packages or bundler.

```sh
dune build @examples/melange @test/examples/runtest
dune build @test/examples/runtest --sandbox=symlink --force
```

The Dune rule depends on the complete local emission and all example emissions,
including their library/runtime imports and copied assets. Each example runs in
a separate Node process with jsdom globals and a manually flushed rAF queue.
The harness imports the actual emitted example entry point, then the emitted
Reason smoke tests. It only bootstraps DOM, frame scheduling, fixture-backed
fetch and asynchronous history traversal; assertions/interactions live in
`smoke.re`.

Coverage: shipped HTML/root asset base (including nested initial paths), counter
increment/decrement; todo's seed button and 1,000-row mount, edit, form add,
trim/blank handling, checkbox toggle, delete, all filters, retained keyed row
identity (zero recreated rows on edit/toggle); JSON request URL/count and decoded
users, internal link interception, detail route, real history.back(), NotFound,
home navigation and cached data across routes.
