# Anchor

Anchor is a viewport-positioning runtime, not an interactive widget. Consumers
supply the button, listbox, tooltip, menu, or dialog semantics. It adds no roles,
ARIA relationships, key handlers, dismissal behavior, focus movement or locks.
[APG patterns](https://www.w3.org/WAI/ARIA/apg/patterns/) remain the consumer's
contract; for example, a [tooltip](https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/)
must leave focus on its trigger and dismiss on Escape.

## Compose

```reason
module A = Tea_ui.Anchor;
/* Parent init: inactive until its overlay is mounted. */
let anchor = A.init(
  ~anchor_id="trigger", ~floating_id="panel",
  ~placement=(A.Bottom, A.Start), ~offset=8., ~top_layer=true, (),
);
/* On opening, commit the returned model and map its command with Cmd.map. */
let (anchor, command) = A.start(anchor);
/* Parent subscriptions: Sub.map(msg => Anchor(msg), A.subscriptions(anchor)).
   Parent update delegates Anchor(msg) to A.update; handle its out messages. */
let panel = A.view(
  ~model=anchor, ~to_parent=msg => Anchor(msg),
  ~to_view=parts => Html.div(parts.floating, [Html.text("Panel")]), (),
);
```

Apply `A.attrs(anchor)` directly when a parts callback is unnecessary. Attribute
bundles are polymorphic in the parent's message type. The callback accepts
`to_parent` for the stateful convention, but Anchor has no DOM handlers to map.

`start`, `stop`, `refresh` and `configure` return `(model, Cmd.t(msg))` and do
not emit domain changes. Commit the model, map the command, and include its
subscriptions. `start`/`stop` are idempotent. `update` returns the conventional
`(model, command, out_messages)` triple. `Positioned(position)` reports a new
measurement; `Unavailable` means an id was missing. Neither opens/closes the
consumer. `reference_hidden` lets consumers decide whether to close or retain
the hidden overlay. Stale generations cannot resurrect stopped overlays.

Ids are supplied explicitly because Anchor composes existing component parts;
it does not derive a second trigger/panel id prefix. Both must be distinct,
document-unique, and refer to elements in this document. Only one Anchor may
own a floating element. Keep its element mounted while active. Call `refresh`
after replacing either element (even with the same id), or after a manual
layout change that does not scroll/resize. Stop before a consumer removes the
elements. Shutdown disposes the same sources without needing a stop command.

## Geometry

The pure function uses CSS-pixel viewport rectangles and floating border-box
size. Top/Bottom alignment is logical horizontally: Start means left in LTR and
right in RTL. Left/Right alignment is vertically top/center/bottom and does not
swap in RTL. Sides themselves are physical. Flipping preserves the requested
logical alignment. It flips only if the preferred side does not fit and the
opposite has more usable room; if neither fits, the larger side wins. Ties keep
the preferred side. No perpendicular-side fallback or main-axis shift occurs.

`shift_padding=Some(p)` shifts only the cross axis within the padded viewport.
Padding is capped at half each viewport dimension; oversized panels pin to the
padded cross-axis start without resizing. `None` disables shifting and uses
zero padding for available-space calculations. Negative offsets permit overlap;
negative dimensions/padding become zero and nonfinite inputs become zero.

`available_width` and `available_height` are nonnegative usable extents on the
resolved side, bounded by the padded viewport. Constrain a scrolling panel with
`max-height: var(--tea-available-height)` and/or its width counterpart. Anchor
does not impose scrolling, max sizes or styles beyond positioning. Main-axis
overflow is possible if consumers ignore these available extents.

`match_width` sets floating **border-box** width to anchor width, even if that
width exceeds the viewport. The next ResizeObserver delivery accommodates a
resulting text reflow. `arrow_size` optionally computes the arrow's cross-axis
leading edge, clamped inside the panel; the consumer owns arrow markup/styles.
The core does not round. Runtime left/top round to device pixels.

A reference is hidden when it has zero area or no positive-area intersection
with the viewport. Partial intersection remains visible. Hidden reason does
not include intermediate ancestor clipping or CSS visibility.

## Lifecycle and top layer

Both `Browser_dom.get_element` tasks are queued after render in the same frame.
Window resize, captured scroll from any document container, and
`Browser_events.on_element_resize` for both elements feed one pending-frame
measurement gate. Events do not dispatch parent messages individually. Only
changed computed geometry/DPR (or the first result per generation) dispatches,
with at most one completion per animation frame. Stopping immediately removes
listeners/observers and suppresses queued callbacks. The shared frame scheduler
has no cancellation handle; already queued guarded reads finish harmlessly.

`visibility:hidden` applies until the first measurement. This prevents a flash
at `(0,0)` without preventing measurement. It also hides a measured panel while
its reference is offscreen/empty. Inactive panels use the native `hidden`
property. A missing element clears the measurement and emits `Unavailable`
once; subsequent resize/scroll can recover, or call `refresh` after mounting.

`top_layer=true` adds `popover="manual"` and feature-detects
`showPopover`/`hidePopover`. Promotion happens after render and before reads.
The panel remains in its original DOM location while escaping overflow clips
and transformed containing blocks. Anchor resets UA popover margin/inset, and
hides its promoted element on stop/shutdown. Manual popovers do **not** acquire
light dismissal or Escape behavior. No dialog modality is introduced. Omit
autofocus descendants if the consumer must retain trigger focus.

Default `top_layer=false` leaves a plain fixed element. Unsupported Popover API
browsers use that same fallback: it **cannot** escape clipping/transformed fixed
containing blocks. Place such a panel under an untransformed root yourself.
This is not a portal or a top-layer polyfill. Do not override the owned `popover`
attribute or native popover state while Anchor is active.

## Keyboard, ARIA and attributes

| Input | Anchor behavior |
| --- | --- |
| Tab / Shift+Tab | Native traversal, never prevented |
| Escape | Unchanged; consuming overlay owns dismissal |
| Enter / Space / arrows | Unchanged; consuming control owns activation/navigation |
| Ctrl / Alt / Meta chords, IME | Unchanged |
| Pointer | No interception or focus movement |

`data-placement="top-start|top-center|top-end|bottom-start|bottom-center|bottom-end|left-start|left-center|left-end|right-start|right-center|right-end"`
uses the resolved side and logical alignment. `data-reference-hidden=""` is a
presence flag. No `aria-hidden`, role, or idref is invented by Anchor; CSS
visibility/native hidden already removes unmeasured/inactive panels from the
accessibility tree. The consumer adds all pattern-specific ARIA.

Owned attrs/styles: `id`, `hidden`, optional `popover`, `position:fixed`,
`inset:auto`, `margin:0`, `box-sizing:border-box`, `left`, `top`, `width`,
`visibility`, `--tea-anchor-width`, `--tea-anchor-height`,
`--tea-available-width`, `--tea-available-height`. Append only nonconflicting
facts. Use intrinsic/CSS width when not matching; do not append an overriding
inline width. Geometry, not a stylesheet, determines visibility and placement.

## Full API

All types and signatures below are exported by `Tea_ui.Anchor` / `Tea.Ui.Anchor`.
The `.rei` contains field-level documentation.

```reason
type side = Top | Bottom | Left | Right;
type alignment = Start | Center | End;
type placement = (side, alignment);
type direction = Ltr | Rtl;
type rect = {x: float, y: float, width: float, height: float};
type size = {width: float, height: float};
type hidden_reason = Reference_outside_viewport | Reference_empty;
type position = {
  x: float, y: float, width: float, height: float, placement: placement,
  anchor_width: float, anchor_height: float,
  available_width: float, available_height: float,
  reference_hidden: bool, hidden_reason: option(hidden_reason),
  arrow_offset: option(float),
};
let compute: (
  ~anchor: rect, ~floating: size, ~viewport: rect, ~placement: placement,
  ~offset: float=?, ~flip: bool=?, ~shift_padding: option(float)=?,
  ~match_width: bool=?, ~dir: direction=?, ~arrow_size: float=?, unit
) => position;
let placement_name: placement => string;
type model;
type msg;
type out_msg = Positioned(position) | Unavailable;
let init: (
  ~anchor_id: string, ~floating_id: string, ~placement: placement=?,
  ~offset: float=?, ~flip: bool=?, ~shift_padding: option(float)=?,
  ~match_width: bool=?, ~dir: direction=?, ~top_layer: bool=?,
  ~arrow_size: float=?, unit
) => model;
let active: model => bool;
let position: model => option(position);
let start: model => (model, Tea_core.Cmd.t(msg));
let stop: model => (model, Tea_core.Cmd.t(msg));
let refresh: model => (model, Tea_core.Cmd.t(msg));
let configure: (
  ~placement: placement=?, ~offset: float=?, ~flip: bool=?,
  ~shift_padding: option(float)=?, ~match_width: bool=?, ~dir: direction=?,
  ~top_layer: bool=?, ~arrow_size: option(float)=?, model
) => (model, Tea_core.Cmd.t(msg));
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
let attrs: model => list(Tea_html.Html.attr('parent));
type parts('parent) = {
  floating: list(Tea_html.Html.attr('parent)), position: option(position),
};
let view: (
  ~model: model, ~to_parent: msg => 'parent,
  ~to_view: parts('parent) => Tea_html.Html.t('parent), unit
) => Tea_html.Html.t('parent);
```

Defaults: inactive, `(Bottom, Start)`, offset `0`, flip `true`, shift padding
`Some(8)`, match width `false`, LTR, top layer `false`, no arrow. All geometry
options can change with `configure`. `configure(~arrow_size=None, model)`
removes an existing arrow calculation; `~shift_padding=None` disables shifting.

## Verification and boundaries

Run `dune build @test/ui/anchor/runtest --force` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs anchor`.
The pure matrix covers every placement, flip, shift, direction and match-width
combination over normal, displaced, tiny and zero viewports, including partial,
empty and offscreen anchors. jsdom verifies real runtime subscription cleanup,
rapid lifecycle changes, source bursts, missing/replaced nodes, custom
properties, DPR rounding and explicit popover ownership. Chromium verifies
all placements/RTL, flip/shift, offsets, nested/window scrolling, viewport and
anchor resize, native keyboard/pointer controls, ARIA snapshots, unfiltered axe
in each tested state, and hit-testing beyond an overflow-hidden transformed
ancestor. The example exposes every runtime geometry option except optional
arrow rendering (available through the pure/public API).

No continuous polling for animation-only transforms or unrelated layout shifts;
call `refresh`. ResizeObserver absence leaves initial/scroll/window measurements
working, but element-only resize needs `refresh`. No visual-viewport/pinch-zoom,
vertical writing-mode, iframe/shadow-root id lookup, clipping-ancestor hiding,
portal fallback or arbitrary transformed fallback coordinate conversion is
claimed. These boundaries are explicit rather than hidden CSS dependencies.
