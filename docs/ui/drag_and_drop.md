# Drag and drop

A parent-controlled sortable/movable collection with pointer events and a complete
button-based keyboard alternative. There is no APG drag-and-drop pattern. We use
[APG button semantics](https://www.w3.org/WAI/ARIA/apg/patterns/button/), ordinary
lists and a live region, **not** listbox roles (cards may contain other controls).
The [APG rearrangeable listbox example](https://www.w3.org/WAI/ARIA/apg/patterns/listbox/examples/listbox-rearrangeable/)
informs focus retention and live confirmation, but its selection keyboard table
is deliberately not applied to this non-selection component.

## Integration

```reason
module Drag = Tea_ui.Drag_and_drop;
/* In init: */
let drag = Drag.init(~id="tasks", ());
/* In the parent's update branch: */
let (drag, command, notifications) = Drag.update(~containers, message, drag);
/* Map command and subscriptions into your parent message type. */
let subscriptions = Tea_core.Sub.map(msg => Drag_message(msg), Drag.subscriptions(drag));
```

Collections remain entirely in the parent. Pass the same current collection to
`update` and `view`. Container keys are unique within an instance; item keys are
unique **across all containers**, stable over reorder and never derived from an
index. The parent handles `Moved` by removing `key` from its current source and
inserting it at `to_index` in the target. This index is measured **after removal**;
there is no extra adjustment for moving down the same list. A no-op drop also
emits `Moved`. `Drag_started` fires only after lift/threshold, not on press.

Render native `ul`/`li` (or another suitable semantic structure), applying
`parts.container(key)`, `parts.item(key)` and `parts.handle(key)` to each part.
Handles must be native buttons with meaningful accessible names. Always render
`parts.instructions` with `instructions_text` and the initially empty
`parts.live` with `live_text`. Live regions must stay mounted, not hidden with
`hidden` or `display:none`. A visually hidden, accessibility-preserving class is
optional. Politeness defaults to polite; `assertive=true` selects assertive.

Parts own the event handlers listed in the interface: don't append duplicate
handlers for the same event. Append non-conflicting class/label attributes.
Handles intercept activation to avoid browser scroll and duplicate native clicks;
AT click activation remains supported. Ctrl/Alt/Meta chords and IME are not
claimed. Buttons are disabled when the item or containing collection is disabled.
Disabled containers cannot receive drops; disabled items cannot be lifted, but
remain physical insertion boundaries and are counted in announcement positions.
Use `drag_from_item=false` for cards containing links, inputs or other interactive
controls: only the handle then owns pointer initiation.

## Preview, placeholder and overlay

Do not reorder, remove, absolutely position or transform the real items during a
preview: their rectangles are the collision reference. Keep the dragged item
mounted and reserve its original layout space. Style `data-dragging` with opacity
or outline, and use `preview` to paint a non-layout-shifting insertion indicator.
The application can render an `aria-hidden` absolutely/fixed-positioned overlay
using `parts.overlay`, which supplies `--tea-drag-x` and `--tea-drag-y` in CSS
pixels from the initial press. These variables also appear on item parts. The
component does not position or create an overlay. Never duplicate a focusable
button inside it. On drop, apply `Moved` before the next render; the post-render
focus command finds the moved item's stable handle id, including cross-container
moves that remount it.

Pointer movement starts a drag after the configurable maximum-axis distance
(default six CSS pixels). Touch additionally waits 250ms by default. Movement
past the threshold before the delay cancels, preserving scrolling. A scoped
non-passive touchmove fallback cancels browser panning only for the active item's
single touch once the delay has elapsed; no consumer touch-action CSS is required.
Short presses remain clicks. Pointer identities are checked for movement,
release and cancellation. Release freezes the coordinates and measures after the
render; later hover cannot change the committed target. A lifecycle-owned capture
click guard suppresses the drag-generated click before item actions can run,
including browser clicks retargeted to a common ancestor. Escape/programmatic
cancellation retains this pointer-identity-scoped guard through the original
release/click. A new press clears it; after release a frame expires it if the
browser generates no click. No element-local suppression marker can swallow a
later unrelated control action. Pointer cancellation and window blur dispose it
immediately, and shutdown always removes every listener.

Rectangles are read in one post-render batch, coalesced to the latest intent.
Capture scroll, resize and actual auto-scroll steps request fresh measurements.
Near a 32px edge the nearest scrollable ancestor scrolls by 12px per frame; if it
cannot scroll further an ancestor/document can scroll instead. Stationary pointers
continue scrolling. Pending measurement and auto-scroll frames are cancelled when
the interaction ends or the application shuts down.

## Keyboard table

| Key | Idle | Lifted |
| --- | --- | --- |
| Tab / Shift+Tab | Native focus traversal | Cancel, then native traversal; never prevent default |
| Space / Enter | Lift focused handle | Drop; restore moved handle focus after render |
| Escape | Unchanged | Cancel and restore original handle focus |
| Up / Down | Unchanged | Previous/next insertion slot in vertical lists |
| Left / Right | Unchanged | Previous/next slot in horizontal lists; reversed in RTL |
| Shift+Left / Shift+Right | Unchanged | Previous/next enabled **vertical** container; RTL reversed |
| Shift+Up / Shift+Down | Unchanged | Previous/next enabled **horizontal** container |
| Repeated Space / Enter | No lift | No drop |
| Ctrl/Alt/Meta chords, IME | Unchanged | Unchanged |

Movement clamps rather than wraps. Cross-container movement skips disabled
containers, retains the insertion index where possible, and accepts empty lists.
Focus remains on the source handle during preview. The physical collection is
unchanged until drop, so Escape restores by discarding the preview. The submodel
repairs shrinking destinations and falls back to the current source if a keyboard
destination disappears. Removing/disabling the source cancels on the next intent.

## ARIA and data

- Native button type=button, disabled when appropriate.
- `aria-describedby` on handles resolves to the instructions part.
- Labeled containers (`aria-label`); use an appropriate semantic list/group tag.
- Live region `aria-live=polite|assertive`, `aria-atomic=true`.
- No deprecated `aria-grabbed` or `aria-dropeffect`.
- Presence: `data-dragging`, `data-over`, `data-disabled`.
- `data-orientation=vertical|horizontal` on containers.
- Private measurement marker `data-tea-drag-scope`; don't alter it.
- Geometry-only inline styles: `--tea-drag-x`, `--tea-drag-y`.

## Full public API

```reason
type orientation = Vertical | Horizontal;
type direction = Ltr | Rtl;
type item = {key: string, label: string, disabled: bool};
type container = {key: string, label: string, disabled: bool, items: array(item)};
type target = {container: string, index: int};
type movement = {key: string, from_container: string, from_index: int,
                 to_container: string, to_index: int};
type out_msg = Drag_started(string) | Moved(movement) | Cancelled;
type labels = {
  instructions: string,
  picked_up: (string, int, int) => string,
  moved: (string, string, int, int) => string,
  dropped: string,
  cancelled: (string, int) => string,
};
let default_labels: labels;
type model;
type msg;
let init:
  (~id: string, ~orientation: orientation=?, ~dir: direction=?,
   ~threshold: float=?, ~touch_delay: int=?, ~drag_from_item: bool=?,
   ~labels: labels=?, ~assertive: bool=?, unit) => model;
let preview: model => option(target);
let dragging: model => option(string);
let announcement: model => string;
let update:
  (~containers: array(container), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
let cancel: model => (model, Tea_core.Cmd.t(msg));
type parts('parent) = {
  container: string => list(Tea_html.Html.attr('parent)),
  item: string => list(Tea_html.Html.attr('parent)),
  handle: string => list(Tea_html.Html.attr('parent)),
  instructions: list(Tea_html.Html.attr('parent)),
  live: list(Tea_html.Html.attr('parent)),
  instructions_text: string,
  live_text: string,
  preview: option(target),
  dragging: option(string),
  overlay: list(Tea_html.Html.attr('parent)),
};
let view:
  (~model: model, ~containers: array(container), ~to_parent: msg => 'parent,
   ~to_view: parts('parent) => Tea_html.Html.t('parent), unit) =>
  Tea_html.Html.t('parent);
```

Defaults: Vertical, Ltr, threshold=6., touch_delay=250, drag_from_item=true,
English labels, polite announcements. Negative threshold/delay clamp to zero.
All announcement callback positions are one-based; target indices are zero-based.
`cancel` emits no notification, unlike user cancellation.

## Boundaries

Single-item dragging only; no native HTML5 item dragging or selection model.
Collections must render all measured items in the current document, without
transformed preview layout. Virtualized/offscreen-unmounted items, nested sortable
containers, cross-document/iframe dragging and arbitrary two-dimensional grids
are outside this list-oriented API. No pointer capture across browser windows.
Changes to collections are reconciled on the next component intent; `view` does
not mutate state. No paste, file transfer or network upload (see File_drop).

Run `dune build @test/ui/drag_and_drop/runtest --force` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome npm run test:e2e -- drag_and_drop`.
The example includes a Kanban board, handle-only list, horizontal RTL list,
vertical RTL list and a scrollable list.
