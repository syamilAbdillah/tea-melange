# v0.1.0 definition-of-done checklist

Maps the seven items in `docs/plan.txt`, “Definisi selesai v0.1.0”, to evidence.

1. **Three runnable browser examples — complete.** `examples/counter/main.re`
   is the plan counter after `open Tea;` (with Elm's
   `update(msg, model)` order). `examples/todo/main.re`
   supports add/edit/toggle/delete, all/active/done filters, a “Seed 1,000 items”
   button, `Keyed.ul` and `Html.lazy1`. `examples/fetch_routing/main.re` uses
   Http + Decode + Url.Parser for `/`, `/users`, `/users/:id` and NotFound.
   Each emission includes index.html, browser import maps and static assets.
   `scripts/serve.js` serves the emission with SPA fallback. A recursive HTTP
   import-resolution check resolved all 76 fetch_routing modules and checked
   `/users/1`, `/missing` and users.json. `test/examples/smoke.re` drives all
   three real emitted programs in jsdom, including a 1,000-row toggle with zero
   recreated rows and mocked-fetch history.back. No full graphical-browser
   automation was used; jsdom does not measure layout/paint.

2. **Native and jsdom tests green — complete.** `dune build @runtest --force`:
   `test/core/runtime_test.re` (178 runtime/Task assertions), native HTML/view
   tests, `test/vdom/test_vdom.re`, `test/dom/test_dom.re` (500 randomized DOM
   pairs plus forms/events/scopes/lifecycle), HTTP/effects/URL/navigation tests,
   `test/application/application_test.re` (internal/external links, modifiers,
   init push, push/replace/back, cleanup/remount) and all three example smokes.
   Example tests also pass with `--sandbox=symlink`.

3. **10,000 random trees match the naive reference — complete.**
   `test/vdom/test_vdom.re` prints “10000 deterministic random old/new pairs:
   OK”; targeted tests cover keyed identity and 5,000-deep elements, taggers
   and thunks. DOM randomized cases independently compare against fresh render.

4. **All five performance budgets — complete for the recorded integration run.**
   `dune build @bench --force` runs native vdom and real jsdom benchmarks.
   Diff+patch median **0.4786 ms** (<2 ms); independent unprofiled cold mount
   **34.4011 ms** (<40 ms); 100 messages/tick **1 patch**; recreated row/text
   nodes **0**; listener growth across 1,000 patches **0** (1,000 actual
   registrations before/after, zero after disposal). See `docs/perf.md` for
   methodology and all observations. The final registration-profiler cold mount
   also passes (**34.9259 ms**); earlier profiled runs exceeded 40 ms and are
   retained in the report. Timings are host-load sensitive.

5. **No melange-webapi dependency — complete.** `dune-project`, generated
   `tea.opam`, and library Dune stanzas depend on Melange, melange.dom and
   melange.js only (plus project libraries). `docs/license-audit.md` supersedes
   ADR-02: no LGPL melange-tea code is copied. MIT LICENSE includes the Elm
   algorithm attribution.

6. **No hand-written .ml/.mli — complete.** `scripts/check-no-ml.sh` checks
   src, examples, bench and test; `.github/workflows/ci.yml` enforces it along
   with `scripts/check-fmt.sh`. All sources are .re/.rei, using Dune's built-in
   Reason dialect; JavaScript is confined to test harnesses and the static
   example server. Formatting checks use temporary output and never edit files.

7. **Public API odoc and complete README example — complete.**
   `dune build @doc` succeeds without warnings and produces
   `_build/default/_doc/_html/index.html` for all public libraries, including
   tea.test (native-only) and the Tea umbrella. `.rei` files describe the public
   APIs; core command/subscription/decoder/vdom helpers have per-item comments.
   `docs/index.mld` is the package landing page. README includes installation,
   full compiled counter, exact example commands, module map, principles and
   ADR-02/ADR-06. `CHANGES.md`, MIT LICENSE, dune-project and generated tea.opam
   mark version **0.1.0**.

## Integration notes

- `Tea.App.beginner` intentionally returns unit for the original top-level
  `let () = ...` sample; `Tea_dom.App.beginner` retains its existing handle API.
- Application lives in tea.nav and is re-exported as Tea.App.application;
  tea.dom does not depend on tea.nav. The Random umbrella preserves pure seed
  generators and adds effectful generation. Animation_frame is also exported.
- No existing library behavior was changed: existing-source changes are
  documentation, mandatory refmt normalization, and tea.nav's added dependency
  on tea.dom. New application behavior has dedicated regression coverage.
- Cold measurement is separated from listener profiling; earlier profiled runs
  exceeded the cold budget on this host, though both pass in the final run. Neither browser layout/paint nor
  a future Melange 7.x compiler is claimed tested; CI carries the planned 7.x
  placeholder comment.
