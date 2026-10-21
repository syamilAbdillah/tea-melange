# P5 effects

All implementations are original Reason source. No melange-webapi, npm package,
platform implementation, or copied melange-tea code is used.

## Libraries and entry points

- `tea.core`: `Tea_core.Random`, pure generators over explicit seeds. Xorshift32
  uses `Int32` operations (including logical shifts), never host-width integer
  arithmetic. Zero is normalized to seed one. Integers use rejection sampling
  over unsigned 32-bit samples; bounds must fit signed 32-bit on both targets.
  Invalid bounds/counts/weights raise `Invalid_argument`. This is not a
  cryptographic generator. Native and Node execute the **same** test module,
  including the recorded seed-42 integer sequence
  `[432, 348, 59, 16, 556, 134, 840, 334, 979, 639]` for bounds 0..999.
- `tea.effects`: `Tea_effects.Time`, `Random`, `Animation_frame`. The clock
  binding is private. `Time.now_task` reads Unix epoch milliseconds when run;
  `Time.every` requires a positive finite interval. Random commands acquire
  fresh seeds from `Math.random` at execution time. Animation timestamps are
  monotonic frame timestamps, not Unix epoch timestamps; the first delta is
  zero. Each source cancels its outstanding callback on unsubscribe, including
  unsubscribe from inside a delivered frame.
- `tea.http`: `Tea_http.Http`. `get`, `post`, and `request` are commands;
  `get_task`, `post_task`, and `request_task` are lazy text-producing tasks.
  `decode_json` composes with Task combinators for typed JSON tasks. Expectations
  turn either success or error into a message. `request` and task constructors
  end with `()` to make optional timeout arguments erasable. Timeout is in ms,
  with zero disabling it; it covers both fetch and reading the body. Timeout
  aborts via AbortController; completion is guarded against late resolution.
  Callback exceptions are not swallowed by the transport's Promise catch.
  Explicit Content-Type headers override body defaults. Non-2xx statuses become
  `Bad_status`; malformed URLs, fetch failures, JSON syntax, and decoder failures
  retain separate errors. `Http.Json.decode_value` recursively interprets the
  GADT directly over raw JSON via `Js.Json.classify` (typeof/array detection)
  and `Js.Dict`. It checks own fields and uses prototype-free dictionaries for
  portable-value conversion. Integer JSON decoding rejects values outside the
  signed 32-bit range, preventing native/JS wrapping inconsistencies.
- `tea.url`: `Tea_url.Url`, native + Melange. Absolute HTTP(S) URLs only; no
  credentials. Encoded path/query/fragment are retained for serialization;
  host case and empty paths are normalized. Bracketed IPv6 authorities and
  numeric ports are supported. This is a small pure URL parser, not a complete
  WHATWG URL implementation/normalizer. `Parser` consumes percent-decoded path
  segments; one trailing slash is accepted, interior empty segments are not
  dropped. `slash(a,b)` pairs results and `map(f,p)` transforms them (a
  value-oriented API rather than Elm's curried constructor typing). Query
  parsers select the first matching parameter, decode `+` to space, and return
  None for missing/invalid values. Integer segments/query values are signed
  decimal 32-bit values. Fragment parsing percent-decodes without changing `+`.
  Percent decoding preserves UTF-8 bytes and rejects malformed hex escapes.
- `tea.nav`: `Tea_nav.Navigation`. History commands notify `on_url_change`
  listeners using the internal `tea:navigation` event; popstate is also
  observed. `load` and `reload` perform full navigations. `current_url` requires
  an HTTP(S) page. Link interception installs one delegated root listener;
  modified/non-primary/prevented clicks, downloads, and non-self targets retain
  browser behavior. Same-origin HTTP(S) links are Internal, all others External.
  The returned disposer removes the listener. P6 adds `Tea_nav.Application`
  using `Tea_dom.App.Internal.start`; the Tea umbrella exposes it as
  `Tea.App.application`. The browser platform remains owned by tea.dom.

Subscription identities follow the binding runtime contract:
`Time.every` / `string_of_float(ms)`, `AnimationFrame` / `frame` or `delta`,
`Navigation` / `url`. Starts deliver raw payloads, not pre-tagged messages, so
kept subscriptions always use the runtime's latest tagger.

## Usage sketches

```reason
let generator = Tea_core.Random.list(3, Tea_core.Random.int(1, 6));
let (dice, next_seed) = Tea_core.Random.step(generator, Tea_core.Random.initial_seed(42));
let command = Tea_effects.Random.generate(dice => Rolled(dice), generator);
let timer = Tea_effects.Time.every(1000., timestamp => Tick(timestamp));
let fetch = Tea_http.Http.get(
  ~url="/api/count",
  ~expect=Tea_http.Http.expect_json(result => Loaded(result), Tea_core.Decode.field("count", Tea_core.Decode.int)),
);
module P = Tea_url.Url.Parser;
let user_route = P.map((((), id)) => id, P.slash(P.s("users"), P.int));
```

## Node test pattern (for task D)

The root package.json already has `"type":"module"`; emit **es6**, not CommonJS.
Generated entry-point paths include their workspace-relative source directory:

```lisp
(melange.emit
 (target js)
 (alias effects-js)
 (modules effects_node)
 (module_systems es6)
 (libraries tea.core tea.effects random_cases)
 (preprocess (pps melange.ppx)))

(rule
 (alias runtest)
 (deps (alias effects-js) js/test/effects/effects_node.js)
 (action (run node %{dep:js/test/effects/effects_node.js})))
```

The emit alias dependency is important: it reruns Node when an emitted
**dependency**, not just the entry-point JS, changes. Dune generates all JS and
runtime dependencies under the target directory. No npm install is necessary.
For tests shared between native and JS, put cases in a `(library ... (modes
native melange))`, then call its `run()` from distinct native and emitted
runners. Do not assign the same Reason module directly to both a native test
and a melange.emit stanza: that creates duplicate preprocessing rules.

HTTP tests use Node's real fetch for a local `data:` URL and replace global
fetch for deterministic network/status/timeout/post cases; they never use the
internet. Navigation tests use Node EventTarget and a small explicit DOM/history
fixture, with no jsdom dependency. Effects tests instantiate the real
`Runtime.Make` with a setTimeout frame platform, observe interval delivery and
fresh taggers, unsubscribe by model state, and assert the model remains unchanged
after cleanup. The Node animation test deliberately exercises the 16ms fallback.
Async tests have watchdogs so failed completion cannot silently pass.

## Plan adjustments

The allowed P5 additions are `tea.effects`, pure `Tea_core.Random`, and the
separate pure `tea.url` library. Time zones are intentionally omitted. No
Browser_platform is defined. Parser composition uses the named `slash` function
(no custom infix). No existing core/runtime or other agents' files were changed.
The explicit interfaces contain odoc comments for every exported item.
