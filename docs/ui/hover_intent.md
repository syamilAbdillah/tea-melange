# Hover intent

`Tea_ui.Hover_intent` (`Tea.Ui.Hover_intent`) decides whether a mouse or pen
really hovers a trigger or its content. **The consumer owns semantics,
positioning, visibility and styling.** It is a building block, not a Tooltip.

Related [APG tooltip pattern](https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/):
hovered content stays open while the pointer is over the trigger **or** content.
APG specifies no standalone hover-intent role or keyboard interaction. A Tooltip
consumer must additionally implement `role=tooltip`, `aria-describedby`, focus
opening, blur dismissal and Escape without moving focus. Interactive hover
content needs different semantics (for example, a nonmodal dialog), not tooltip.

## Integration

```reason
module H = Tea.Ui.Hover_intent;
/* Parent model field: hover: H.model. */
let hover = H.init(~open_delay_ms=450, ~close_delay_ms=300, ());

/* In the parent's Hover(intent) update branch: */
let (hover, cmd, out) = H.update(~disabled, intent, model.hover);
/* Commit hover; Cmd.map(intent => Hover(intent), cmd).
   Handle H.Intent_open / H.Intent_close in out. */

/* In the parent's subscriptions; key is unique within this runtime: */
Sub.map(intent => Hover(intent), H.subscriptions(~key="tea-preview", model.hover));

/* In the parent's view (semantics deliberately omitted here): */
H.view(
  ~model=model.hover,
  ~disabled,
  ~to_parent=intent => Hover(intent),
  ~to_view=parts => Html.div([], [
    Html.button(parts.trigger_attrs @ [Html.id("tea-preview")],
      [Html.text("Preview")]),
    Html.div(parts.content_attrs @ [Html.hidden(!parts.is_open)],
      [Html.text("A cool, mineral cup.")]),
  ]),
  (),
);
```

Own `pointerenter`, `pointerleave`, `pointercancel` only; do not stack another
handler for those names. Append nonconflicting attributes and consumer handlers
freely. `trigger_attrs` and `content_attrs` are available independently for
composing with other components' parts. All handlers are mapped with
`Vdom.map_fact` into the parent's message type. Neither function reads a
model-dependent next state in an event closure.

When disabling an instance, call `close_now` in that same parent update and
commit its returned model. Pass the **current** disabled flag to every `update`
and attribute builder as well: this guards queued events from a stale enabled
render and any already-queued timer completion. Disabled clears intent state
silently; consumers update their own visibility when changing disabled.

`open_now` and `close_now` have the conventional `(model, command)` shape and
never emit out messages. Use them for consumer-owned focus, blur and Escape.
Focus persistence is also consumer-owned: a Tooltip may combine focus state
with hover intent rather than letting a pointer leave hide a focused tooltip.
A fresh pointerenter after `close_now` can start a new opening; an obsolete
completion cannot reopen it. Unrelated keys and focus never affect this module.

## Delays and lifecycle

- Negative delays clamp to zero. A zero delay transitions in the event update,
  without a timer; a positive delay starts a private `setTimeout` task.
- Opening waits while either region remains hovered. Leaving both before the
  deadline cancels the opening, with no spurious close intent.
- Both regions share one closing timer. The close delay covers a physical gap;
  crossing it to either region cancels the close. Duplicate enters/leaves do not
  reset an existing deadline. Overlapping/nested regions work too.
- Actual state transitions emit exactly one `Intent_open` or `Intent_close`.
  Repeated events, stale completions and setters emit none.
- Window blur and pointer exit (`pointerout` with null relatedTarget) clear both
  regions and use the same close grace; they cancel an unopened delay. Hiding
  the document closes immediately. Touch pointer events do not start or end
  hover; mouse and pen do. Pointer cancellation acts like leaving that part.
- **Always include `subscriptions`.** Timers run as private CPS tasks owned by
  a subscription, not an uncancellable command. A changing generation is part
  of the timer subscription identity and its completion message. Obsolete
  messages are rejected in `update`; removed subscriptions also call
  `clearTimeout` synchronously. `close_now` takes effect when the parent commits
  its result and the runtime reconciles subscriptions. Shutdown and removal of
  an instance cancel tasks and listeners even before the next frame.
- `update` currently returns `Cmd.none`; the command slot remains for normal
  stateful-submodel composition. Keeping the cancellable task in `subscriptions`
  provides the shutdown disposer that `Cmd`/`Task` alone do not expose.

No global mutable state, DOM containment assumptions, focus effects, document
locks or geometry. Trigger and content may be portalled independently. One model
represents one pair; parents store keyed collections, not this component.

## Warm-up groups

```reason
/* One immutable value in the parent model, independent of the members. */
let group = H.Group.init(~skip_delay_ms=1800, ());
let (hover, cmd, out) = H.update(~group=model.group, intent, model.hover);
let group = List.fold_left((group, out) => switch (out) {
| H.Intent_open => H.Group.record_open(~key, group)
/* On Intent_close, use the new model, not the previous frame's model: */
| H.Intent_close =>
  switch (H.last_close_time(hover)) {
  | Some(time_stamp) => H.Group.record_close(~key, ~time_stamp, group)
  | None => group
  }
}, model.group, out);
/* Commit group; pass it to every member's next update. When a member
   unmounts or closes programmatically, call H.Group.forget(~key, group). */
```

Without a group, opening is cold. While any member is recorded open, the group
is warm: moving the pointer to a sibling opens it immediately, even though the
first member is still inside its close delay (consumers that show one popup at
a time close the others, listed by `Group.open_keys`). After the last member
closes, a close less than `skip_delay_ms` ago skips the open delay. At exactly the boundary it
is cold; backwards timestamps are cold. Zero/negative skip disables warm-up.
Groups are passed, never stored in an intent model or mutated. Independent
groups cannot warm each other. Only record actual closes; the sample above
belongs **inside** the `Intent_close` branch, not every update.

Use the same monotonic time origin as `PointerEvent.timeStamp`:
`last_close_time` from pointer/environment closes uses browser
`performance.now()`. For a programmatic close, the parent may explicitly call
`Group.record_close` with a timestamp from its own event/effect. Setters do not
implicitly warm a group. Group state is consulted on entry; a subsequent group
change does not retroactively shorten an opening already in progress.

## Public API

```reason
module Group: {
  type t;
  let init: (~skip_delay_ms: int, unit) => t;
  let record_close: (~key: string=?, ~time_stamp: float, t) => t;
  let record_open: (~key: string, t) => t;
  let forget: (~key: string, t) => t;
  let open_keys: t => list(string);
};
type model;
type msg;
type out_msg = Intent_open | Intent_close;
let init: (~open_delay_ms: int, ~close_delay_ms: int, unit) => model;
let is_open: model => bool;
let last_close_time: model => option(float);
let update:
  (~disabled: bool=?, ~group: Group.t=?, msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: (~key: string, model) => Tea_core.Sub.t(msg);
let open_now: model => (model, Tea_core.Cmd.t(msg));
let close_now: model => (model, Tea_core.Cmd.t(msg));
let trigger_attrs:
  (~model: model, ~disabled: bool=?, ~to_parent: msg => 'parent, unit) =>
  list(Tea_html.Html.attr('parent));
let content_attrs:
  (~model: model, ~disabled: bool=?, ~to_parent: msg => 'parent, unit) =>
  list(Tea_html.Html.attr('parent));
type parts('parent) = {
  trigger_attrs: list(Tea_html.Html.attr('parent)),
  content_attrs: list(Tea_html.Html.attr('parent)),
  is_open: bool,
};
let view:
  (~model: model, ~disabled: bool=?, ~to_parent: msg => 'parent,
   ~to_view: parts('parent) => Tea_html.Html.t('parent), unit) =>
  Tea_html.Html.t('parent);
```

Defaults: enabled, no group. `subscriptions` requires a stable, runtime-unique
key, ordinarily the consumer's document-unique id prefix. This primitive does
not generate ids, so `init` intentionally needs no id. Separate runtimes may
reuse a key. Reinitializing a live instance requires removing its subscriptions
first; do not replace a model under a retained subscription identity. Removing
and later remounting a pending model starts its delay again; unmount normally
means dropping that model as well.

## Keyboard, ARIA and styling

| Input | Hover intent | Consumer example |
| --- | --- | --- |
| Tab / Shift+Tab | Untouched; never prevent default | Native focus order; focus opens, blur closes |
| Escape | Untouched | Calls `close_now`; focus stays on trigger |
| Enter / Space | Untouched | Native button activation calls `open_now` |
| Other keys / Ctrl / Alt / Meta / IME | Untouched | Native behavior; no hover intents |
| Mouse / pen enter | Open after delay, or immediately when warm | Shows labelled disclosure region |
| Leave both regions | Close after shared grace | Hides region with `hidden` |
| Touch | Ignore | A completed tap may separately focus/activate the consumer |

The component sets **no ARIA, role, id, tabindex, native disabled or hidden**.
Consumers must supply them as appropriate. Both bundles expose mutually
exclusive `data-open=""` / `data-closed=""` and optional `data-disabled=""`.
No inline styles and no CSS dependency for correctness. An application must
hide/unmount content itself when its visibility state is closed.

## Example, verification and limits

`examples/ui/hover_intent/` demonstrates delayed and immediate hover cards,
disabled state, crossing the content gap, native focus/blur/activation/Escape
composition, and two warm-up-group actions. The actions are a labelled group
of native buttons, **not** an ARIA toolbar promising a roving-arrow keyboard
model. This avoids putting another component's semantics into this primitive.

```sh
dune build @test/ui/hover_intent/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs hover_intent
```

jsdom mounts the real program and controls its real timer boundary to prove
physical cancellation, generation rejection, rapid batched events, disabled
changes, simultaneous instances, visibility/window exit and shutdown cleanup.
Chromium drives the emitted example with real mouse movements and elapsed-time
checks, real gap crossings, warm-up expiry, consumer keyboard focus and trusted
emulated touch. ARIA snapshots and unfiltered axe cover closed/open/disabled
states. No browser polyfill, runtime dependency or CSS is required by the module.

As with other tea.browser sources, events refer to the current document/window;
cross-document portals are not supported. This is a temporal grace period,
not a geometric safe-polygon algorithm for submenu corridors. It does not
subscribe to arbitrary DOM removal: a parent removing a trigger/content pair
must also remove its submodel subscriptions (normal TEA lifecycle). Semantic
Escape dismissal, focus persistence, touch tap behavior and WCAG hover-content
dismissibility remain the consumer's responsibility.
