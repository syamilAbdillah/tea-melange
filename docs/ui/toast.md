# Toast

A headless, immutable notification queue. Each visible entry owns its own
[Animation](animation.md); queued entries have no running auto-dismiss clock.
There is no APG toast pattern: this combines [live alerts](https://www.w3.org/WAI/ARIA/apg/patterns/alert/)
and [native buttons](https://www.w3.org/WAI/ARIA/apg/patterns/button/).

## Render the live regions before inserting content

Always render **all three** persistent containers: the named region, its polite
status lane, and its assertive alert lane. Never conditionally mount a live
region together with its first toast: assistive technology can miss that first
announcement. Toast's initial subscription delays queue admission across a paint,
including when `push` is called in the application's init. Keep the containers
mounted for the submodel's lifetime. Recreate the model on a fresh mount.

The parent stores the actual title, description and domain action data, indexed
by `content_key`. Toast stores only key references, priority, timing and lifecycle.
Retain content until `Dismissed`, **not** until a dismissal is requested, because
the entry remains mounted during its leave animation.

```reason
open Tea;
module T = Ui.Toast;
type msg = Toast(T.msg);
/* content_for is a parent-owned lookup returning (title, description). */
let notifications = (toast, content_for) =>
  T.view(
    ~model=toast,
    ~to_parent=message => Toast(message),
    ~to_view=parts => {
      let lane = urgency =>
        List.filter_map((p: T.toast_parts(msg)) => {
          if (p.urgency != urgency) { None; } else {
            let (title, description) = content_for(p.content_key);
            Some((p.key, Html.div(p.root, [
              Html.h3(p.title, [Html.text(title)]),
              Html.p(p.description, [Html.text(description)]),
              Html.button(p.close_button, [Html.text("Dismiss")]),
            ])));
          };
        }, parts.toasts);
      Html.div(parts.region, [
        Html.Keyed.div(parts.polite, lane(T.Normal)),
        Html.Keyed.div(parts.assertive, lane(T.Urgent)),
      ]);
    },
    (),
  );
```

Use keyed children in each lane to preserve identity and focus. Always render
title and description elements (an empty description is fine) so the root's
ARIA idrefs resolve. Apply each bundle once, without overriding its handlers,
ids, native-button semantics, animation flags or hidden property.

Initialize with `T.init(~id="notifications", ())`. Map every returned command
with `Cmd.map(message => Toast(message), command)`, and forward
`Sub.map(message => Toast(message), T.subscriptions(model.toast))` even when
empty or leaving. Route `Toast(message)` through `T.update`, store its new model,
map its command, and process its output list. Programmatic `push`, `dismiss`
and `dismiss_all` use the same model/command integration.

## Timing, actions and urgency

- Default duration: 5000ms **after Entered**, not after insertion or push.
  Finite values below 5000ms are clamped upward, including zero/negative values.
  Five seconds is a minimum, not a guarantee of sufficient reading time.
  Choose longer durations or persistence for long/important content.
- `~duration=None` makes an entry persistent.
- `~has_actions=true` forces persistence even if a finite duration was supplied.
  This avoids a time limit on an action under
  [WCAG 2.2.1](https://www.w3.org/WAI/WCAG22/Understanding/timing-adjustable.html).
  A parent rendering action parts without declaring has_actions must still
  supply duration=None; finite-duration actions are disabled defensively.
- Each action's nonblank `alt_text` describes an **equivalent way to perform the
  action outside the toast**, e.g. "Restore the item from the archive page."
  This is an accessible description, not the button label. Blank alternatives
  disable the action. Toasts must never be the only way to complete an action.
- `action_attrs(~key="undo", ~alt_text="Restore from the archive.", ())` applies
  to a native button. Distinct action keys make distinct DOM ids. All actions
  emit `Action(toast_key)`; applications with multiple distinct operations can
  compose their domain choice in the surrounding UI/data design. The queue
  then leaves with reason Action; `Dismissed(key, Action)` arrives after Left.
- Hovering the **region**, focusing anywhere inside it, or hiding the document
  pauses **all** timers, including newly promoted entries. Overlapping pause
  causes compose; all must clear before resuming. Remaining time is preserved,
  not reset. Touch pointerenter does not simulate hover.
- Use Normal for ordinary information/success. Urgent alone goes in the
  assertive lane and can interrupt speech. Reserve urgency for genuinely
  time-critical errors; repeated assertive messages are disruptive.

Timers are subscription-owned, physically cleared on pause/dismiss/replacement/
unmount/shutdown. Generation and instance tokens reject raced callbacks.
No mutable cells live inside the model. The initial document-hidden state is
read after mounting; subsequent visibility events use Browser_events.

## Queue and duplicate policy

Default `max_visible=3`, clamped to at least one. Waiting admission is FIFO
regardless of display order. Leaving entries continue consuming their slots
until Animation emits Left. Only then can a waiting entry enter. Oldest_first
is the default; Newest_first reverses visible stacking order. Since the two
priority lanes are distinct DOM containers, DOM/Tab order is lane-local, not
a globally interleaved urgency order. Hotkey focus still selects the most
recently admitted push among currently visible, non-leaving entries.

A duplicate key updates the existing entry **without moving/remounting it**.
It resets the duration, replaces content_key, and reverses an in-flight leave
without emitting a dismissal for that interrupted leave. A visible entry keeps
its original urgency and queue position to avoid relocating a focused DOM
subtree. A waiting entry may change urgency before first admission.

Changed text is a normal text mutation in the existing live region and may be
re-announced (`aria-relevant="additions text"`). Identical text is **not** forcibly
replayed; no fake removal/reinsertion is used. Announcement details depend on
the screen reader/browser. Tests verify live DOM structure, not spoken output.

Omitting key generates a collision-free stable key; inspect keys/queued_keys
to discover it. Explicit keys are usually best for parent data ownership.
Keys are sanitized injectively for ids, including spaces, Unicode and literal
escape-looking strings. Each region needs a document-unique id prefix and a
distinct hotkey if multiple regions are mounted.

`dismiss` ignores unknown/already leaving keys. Queued cancellation finishes
through Left on a missing element without mounting or taking a visible slot.
`dismiss_all` marks queued entries cancelled too, so they cannot sneak in while
other entries leave. All dismissals, including Programmatic, emit Dismissed once,
after Left; it is a lifecycle notification rather than a domain setter change.

## Keyboard

| Input | Behavior |
| --- | --- |
| F8 (default; configurable exact chord) | Focus most recently pushed visible non-leaving toast; empty queue focuses region. Remember previous focus. |
| Tab / Shift+Tab | Native navigation through buttons. Never prevent default or stop propagation. After hotkey entry, Tab-out returns to the remembered element after native traversal. |
| Plain Escape on toast | Start leaving focused toast; after hotkey entry also return focus. Stop propagation and prevent default only when handled. |
| Plain Escape on empty hotkey-focused region | Return focus without dismissing anything. |
| Enter / Space on buttons | Native activation. Action emits Action immediately; close leaves with Close_button. |
| Other keys / Ctrl+Alt+Meta shortcuts / IME | Unclaimed keys propagate; configured hotkey is the explicit exact-chord exception. Repeated/composing hotkey events are ignored. |
| Pointer | Native buttons; region hover pauses timers. No swipe gesture is claimed. |

A normal (non-hotkey) close repairs lost focus to another visible toast, or the
empty region. Intentional pointer/programmatic departures to connected elements
are not stolen. Browser_events.restore_focus remembers the original element
and restores only if connected and safe. If it has been removed, the parent
must choose a logical fallback. Tab-out restoration intentionally happens
after the browser's native focus move; Tab's default is never cancelled.

The view's `~on_key_down` hook receives otherwise unhandled plain keys bubbling
through the region, including from buttons/toasts, but not Tab, IME or modified
shortcuts. It returns parent messages, without preventing or stopping events.
Do not append a competing keydown fact. Claimed Escape never reaches an
enclosing Dialog/Popover; tests use an actual ancestor listener, not the
post-dispatch cancelBubble property (which browsers reset).

## ARIA, data and transitions

- Region: role=region, configurable aria-label (default Notifications), tabindex=-1.
- Persistent lanes: role=status/live=polite and role=alert/live=assertive;
  aria-atomic=false, aria-relevant="additions text".
- Toast: role=group, labelled/described by required title/description parts;
  tabindex=-1 for hotkey focus, never an extra sequential Tab stop.
- Close: native button labelled "Dismiss notification"; action buttons have
  aria-description=alt_text. All buttons are disabled while leaving.
- Presence flags: data-paused and Animation's data-enter/data-leave/
  data-transition/data-closed. False means absent, not the string "false".
- data-urgency="normal|urgent", data-index (zero-based visible ordering), and
  geometry-only custom property --tea-toast-index.

No CSS is required for correctness. Always retain leaving DOM until Left.
For example:

```css
.toast { transform: translateX(0); }
.toast[data-transition] { transition: transform 180ms ease; }
.toast[data-closed] { transform: translateX(1rem); }
@media (prefers-reduced-motion: reduce) {
  .toast[data-transition] { transition: none; }
}
```

The standalone example demonstrates all toast kinds, a persistent action with an
outside equivalent, ten queued messages, in-place replacement, programmatic
dismissal, both orderings, alternate label/limit/hotkey, stacking data and real
enter/leave CSS.

## Full public API

```reason
type urgency =
  | Normal
  | Urgent;
type order =
  | Oldest_first
  | Newest_first;
type reason =
  | Timeout
  | Close_button
  | Action
  | Escape
  | Programmatic;
type out_msg =
  | Dismissed(string, reason)
  | Action(string);
type hotkey = {
  key: string,
  ctrl: bool,
  alt: bool,
  meta: bool,
  shift: bool,
};
type model;
type msg;
let init:
  (
    ~id: string,
    ~label: string=?,
    ~max_visible: int=?,
    ~order: order=?,
    ~hotkey: hotkey=?,
    unit
  ) =>
  model;
let keys: model => list(string);
let queued_keys: model => list(string);
let paused: model => bool;
let push:
  (
    model,
    ~key: string=?,
    ~duration: option(int)=?,
    ~urgency: urgency,
    ~has_actions: bool=?,
    string
  ) =>
  (model, Tea_core.Cmd.t(msg));
let dismiss: (model, string) => (model, Tea_core.Cmd.t(msg));
let dismiss_all: model => (model, Tea_core.Cmd.t(msg));
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
type toast_parts('parent) = {
  key: string,
  content_key: string,
  urgency,
  phase: Animation.phase,
  index: int,
  persistent: bool,
  root: list(Tea_html.Html.attr('parent)),
  title: list(Tea_html.Html.attr('parent)),
  description: list(Tea_html.Html.attr('parent)),
  close_button: list(Tea_html.Html.attr('parent)),
  action_attrs:
    (~key: string, ~alt_text: string, unit) =>
    list(Tea_html.Html.attr('parent)),
};
type parts('parent) = {
  region: list(Tea_html.Html.attr('parent)),
  polite: list(Tea_html.Html.attr('parent)),
  assertive: list(Tea_html.Html.attr('parent)),
  toasts: list(toast_parts('parent)),
  paused: bool,
  queued: int,
};
let view:
  (
    ~model: model,
    ~to_parent: msg => 'parent,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
```

## Private platform capabilities and limits

The module privately binds monotonic performance.now, cancellable setTimeout/
clearTimeout, initial document.hidden, DOM lookup/containment, target/
relatedTarget, and focusin/focusout boundary listeners. Its custom capture
hotkey subscription uses Ui_keyboard.decoder via Dom_decode, and calls
preventDefault/stopPropagation only for the configured chord. Browser_events'
keydown intentionally cannot prevent defaults, which necessitates that private
source. Visibility and focus restoration still reuse Browser_events, effects
reuse Browser_dom, and each lifecycle reuses Animation. No browser/foundation
or other component source was changed.

**Swipe is explicitly omitted**, rather than shipping partial pointer capture,
threshold or cancellation behavior. No data-swipe or --tea-toast-swipe-x/y are
emitted, and the reason type contains only implemented dismissal sources.
Hover-expanded stacking is also omitted.
DOM ids/focus are document-local, not shadow-root or iframe aware.
Animation's inherited 5000ms watchdog can end deliberately paused/infinite or
longer visual transitions; no Animation defect or missing capability was found.

## Tests and commands

```sh
dune build @test/ui/toast/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs toast
```

jsdom mounts real Tea programs and dispatches real pointer/key/focus/visibility
events with a deterministic, restored clock fixture. It checks ARIA/idrefs,
FIFO limits and cancellations, both orderings, exact timer remainders and
overlapping pauses, duplicate reversal/reset, stale generation/instance rejection,
minimum durations, action safety, hotkey/focus return, event layering and physical
timer/listener/observer disposal on stop/unmount/shutdown.

Chromium drives the actual example, verifies persistent live-region nodes and
lane text, snapshots, native keyboard/pointer controls, Tab-out and focus return,
custom chords, duplicate identity, ten-entry queue, stacking geometry, deliberate
non-claiming of swipe, and real CSS leave completion. Playwright's clock covers
hover/focus/visibility pause/resume and persistent actions deterministically.
Document visibility is a property/event fixture (headless tabs do not reliably
background); it exercises the real Browser_events source, not a Toast test hook.
Axe is unfiltered in empty, normal, urgent, action, queued, alternate and restored
states, including after real visual transitions.

Hotkey entry uses a cancellable post-render focus request. A focus movement
before its frame supersedes entry, including moving away and back, and shutdown
cancels it. Ordinary focus repair after a focused toast disappears uses the same
ownership and does not override a later destination. The existing Tab/Escape
return bridges retain their command ordering
with the opener restoration lease; they are separate from hotkey entry.
