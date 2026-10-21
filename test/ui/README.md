# Component jsdom tests

Each component adds its own directory and Dune rule. No central registry or
shared-file edit is needed: `dune build @runtest` discovers recursive aliases.

```lisp
; test/ui/<name>/dune (replace widget with your module name)
(melange.emit
 (target js)
 (alias melange)
 (modules widget_test)
 (module_systems es6)
 (libraries tea ui_test_support)
 (preprocess (pps melange.ppx)))

(rule
 (alias runtest)
 (deps (alias melange) ../setup.js js/test/ui/widget/widget_test.js)
 (action (run node %{dep:../setup.js} %{dep:js/test/ui/widget/widget_test.js})))
```

For a directory literally named widget, export a `completed` Promise from
`widget_test.re` (use `Js.Promise.resolve()` for synchronous tests).
The rule depends on the **whole emission alias**, including dependencies,
not just the entry file. See [button](button/button_test.re) for a full example.

`setup.js` installs jsdom document/window, KeyboardEvent, MouseEvent, FocusEvent,
MutationObserver and PointerEvent (a MouseEvent subclass only when absent).
It exposes manual rAF/cancellation, `teaFlushFrame`, frame counts and document/
window listener counts. Listener accounting distinguishes capture, deduplicates
registrations and handles once/AbortSignal cleanup. It primes jsdom's internal
selector/style listeners first. Unsupported layout/dialog/media/animation APIs
are not silently polyfilled.

The optional ResizeObserver fixture is explicitly installed/restored by tests.
It only delivers supplied dimensions; it does not simulate layout. Real layout,
dialog modality, media changes and animations are checked in Chromium.

`Test_support` is an optional Melange-only test library for node lookup, real
event constructors, frame flushes, checks and Promise ticks. Components can
also bind their own APIs. `frame()` flushes draw plus post-render effects/spare
frame. `key` intentionally does not flush, permitting batched-intent tests.
jsdom does **not** synthesize browser keyboard clicks or implement Tab traversal:
test those in Chromium, not with misleading native-keyboard assertions.

Mount a real `Tea.App.element`, dispatch actual events, inspect ARIA/idrefs/
data attributes/focus/out messages and call `shutdown`. Assert no root-managed
listeners via `Tea_dom.Debug.active_listeners()`; the bootstrap independently
fails on document/window listener leaks and unrecovered html/body style
attributes (scroll locks). A 10-second watchdog rejects hung async tests.
Tests must restore fixture globals and initial document styles.

Pure helpers in `helpers/` compile copies of the private source/interfaces.
This tests the actual algorithms exhaustively without exposing internals from
`tea.ui`. Later components must not copy or mutate those helpers.

```sh
dune build @test/ui/button/runtest --force
dune build @runtest --sandbox=symlink --force
```
