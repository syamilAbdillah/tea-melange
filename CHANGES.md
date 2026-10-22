# Changes

## Unreleased

**Behaviour change: render/effect ordering.** After committing an update, the
runtime now requests its draw before running commands and diffing subscriptions.
Frame callbacks scheduled by those effects run after the DOM patch, including
when the animator reuses a spare frame. Initialization still renders synchronously.

- Add `tea.ui` accessible headless components, exposed as `Tea.Ui`: Button,
  Input, Textarea, Checkbox, Fieldset, Switch_ (Switch), Select, Meter, Progress,
  Nav, Disclosure, Animation, Anchor, Hover_intent, Radio_group, Tabs, Slider,
  Calendar, Drag_and_drop, File_drop, Virtual_list, Dialog, Popover, Menu,
  Listbox, Combobox, Tooltip, Toast and Date_picker. Applications own content,
  styling and parent collections; stateful components expose immutable
  submodels, mapped commands/subscriptions and semantic output messages.
- Add `tea.browser` (`Browser_dom`, `Browser_events`, also exposed by `Tea`):
  lazy post-render focus/blur, geometry, scrolling, native dialog and bounded
  animation-wait tasks; lifecycle-owned key/outside-pointer, resize/scroll,
  visibility, media-query and element-resize subscriptions, reference-counted
  scroll locks and conditional focus restoration.
- UI behavior: handled Escape stops at the inner overlay; native Tab traversal,
  IME and unclaimed browser shortcuts are preserved. Modal Dialog requires
  native `<dialog>` support; anchored top-layer popups use native Popover API
  where available, without moving DOM nodes. Keep mounted models/subscriptions
  through enter/leave; animation waits have a 5000 ms watchdog. Use unique ids
  and documented part elements/handlers. Meter requires a visible label or
  `aria_label`. Toast has **no swipe-to-dismiss** (no reserved Swipe reason);
  action toasts persist, and finite durations are at least 5000 ms. Calendar
  and Date_picker use parent-supplied civil dates, not the device clock.
- Ship runnable examples for every component, the composed Fieldwork workspace
  showcase, a complete UI guide, jsdom lifecycle/leak tests and Chromium keyboard/
  focus/axe coverage, including nested Escape integration in the showcase.
- Add `Sub.filter_map` for fresh, rejecting subscription taggers, and
  `Vdom.Custom_optional` for event flags without a synthetic message.

Performance and size. On the lustre-labs TodoMVC benchmark used by Foldkit's
performance page, an optimized tea-melange TodoMVC now edges out optimized Elm and the
baseline version beats baseline Elm; that TodoMVC bundle shrank from 26.8 to
13.1 KiB min+gzip. The js-framework-benchmark style suite (`bench/jsfb`) spends
2.3–2.7× less JavaScript time.

**Behaviour change: controlled inputs.** `value`/`checked` are re-synced after
the node fires `input`/`change` (every user edit does, as React relies on), or
when the node is patched. A script assigning `.value` without an event is no
longer reverted on unrelated renders. Options (`selected`) and radio buttons,
which change without events of their own, are still restored on every patch.

- Diff recurses (bounded depth, explicit stack below) instead of allocating a
  job per node; small keyed middles use linear scans.
- `value`/`checked` writes are skipped when the DOM already holds the value.
- Discarded subtrees leave their listeners inert instead of removing each one;
  `Render.dispose` still detaches everything.
- No `Stdlib.Hashtbl` (whose randomisation pulls Random/Int64/Digest) or
  `Fun.protect` (which pulls all of Printf) in the runtime; `Tea.App` and
  `Tea.Random` are module aliases so unused entry points tree-shake. New
  `Tea_core.String_table`.

- `Facts.diff`/`apply` compare short fact lists in place instead of building
  two polymorphic hashtables per node; fresh nodes apply facts directly.
- Keyed diff trims matching key prefixes/suffixes, emits `Append`/`Remove_last`
  for tail-only changes and uses a string-specialised table for the rest.
- Keyed reorders move only nodes outside a longest increasing subsequence
  (a swap is two moves instead of ~n); removing every child clears at once.
- Build, cleanup and patch walks recurse up to a bounded depth and continue on
  explicit stacks below it, so deep views remain stack-safe.
- No polymorphic comparison or bounds-checked array access in hot loops;
  per-node listener hashtables and DOM expandos removed; `Html.node` no longer
  copies attribute lists without `Attr.none`.

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
