# Slider

A stateful, headless [APG slider](https://www.w3.org/WAI/ARIA/apg/patterns/slider/)
and [multi-thumb slider](https://www.w3.org/WAI/ARIA/apg/patterns/slider-multithumb/).
One keyed thumb creates a single slider; two or more create a constrained range.
Each thumb has its own name, value and tab stop. Thumbs never cross.

## Integrating

```reason
open Tea;
module S = Ui.Slider;
let items = [|
  {S.key: "minimum", label: S.Label("Minimum price"), initial: 20., disabled: false},
  {S.key: "maximum", label: S.Label("Maximum price"), initial: 80., disabled: false},
|];
let slider = S.init(~id="prices", ~items, ~min_distance=10, ());
/* Parent update: */
let update_slider = (message, slider) => {
  let (slider, command, outputs) = S.update(~items, message, slider);
  /* Store slider; map command into the parent's message. Handle outputs. */
  (slider, command, outputs);
};
/* Parent subscriptions: Sub.map(Slider, S.subscriptions(model.slider)). */
let render = slider =>
  S.view(
    ~model=slider, ~items, ~to_parent=message => message,
    ~name="price", ~format_value=(_, value) => Js.Float.toString(value) ++ " dollars",
    ~to_view=parts =>
      Html.div(parts.root, [
        Html.div(parts.track, [
          Html.div(parts.range, []),
          Html.Keyed.div([], Array.to_list(Array.map(
            (thumb: S.thumb_parts(S.msg)) =>
              (thumb.key, Html.div(thumb.root, [])), parts.thumbs))),
        ]),
        Html.div([], Array.to_list(Array.map(
          (thumb: S.thumb_parts(S.msg)) => Html.input(thumb.input, []), parts.thumbs))),
      ]),
    (),
  );
```

The example at `examples/ui/slider/` demonstrates volume with 0.1 steps, a price
range with minimum spacing, vertical orientation, RTL, custom large steps,
per-thumb labels/idrefs, value text, hidden fields, and disabled state.

## Complete public API

```reason
type orientation = Horizontal | Vertical;
type direction = Ltr | Rtl;
type label = Label(string) | Labelled_by(list(string));
type thumb = {key: string, label: label, initial: float, disabled: bool};
type values = list((string, float));
type model;
type msg;
type out_msg = Changed(values) | Committed(values);

let init:
  (~id: string, ~items: array(thumb), ~min: float=?, ~max: float=?,
   ~step: float=?, ~min_distance: int=?, ~large_step: int=?,
   ~orientation: orientation=?, ~dir: direction=?, ~disabled: bool=?, unit) => model;
let update:
  (~items: array(thumb), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
let values: (~items: array(thumb), model) => values;
let set_values:
  (~items: array(thumb), ~values: values, model) => (model, Tea_core.Cmd.t(msg));
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));

type thumb_parts('parent) = {
  key: string, value: float, percent: float,
  root: list(Tea_html.Html.attr('parent)),
  input: list(Tea_html.Html.attr('parent)),
};
type parts('parent) = {
  root: list(Tea_html.Html.attr('parent)),
  track: list(Tea_html.Html.attr('parent)),
  range: list(Tea_html.Html.attr('parent)),
  thumbs: array(thumb_parts('parent)),
};
let view:
  (~model: model, ~items: array(thumb), ~to_parent: msg => 'parent,
   ~name: string=?, ~format_value: (string, float) => string=?,
   ~on_key_down: string => option('parent)=?,
   ~to_view: parts('parent) => Tea_html.Html.t('parent), unit) =>
  Tea_html.Html.t('parent);
```

Defaults: `min=0`, `max=100`, `step=1`, `min_distance=0`, `large_step=10`,
`orientation=Horizontal`, `dir=Ltr`, `disabled=false`. The formatter defaults
to decimal numeric text. The optional keyboard hook receives unhandled plain,
non-Tab keys; its messages do not prevent default. Owned slider keys take priority.

`items` is parent-owned and is never retained in the submodel. Keys are stable,
unique strings, not indices. IDs use foundation's collision-free key encoding.
Pass the same collection to `update`, `values`, and `view`. On a parent-only
collection replacement, synchronise the submodel using
`set_values(~items, ~values=S.values(~items, model), model)` before storing it.
This also disposes a drag immediately. Otherwise the next slider message repairs
the collection; view/accessors already expose repaired values in the meantime.
Removed keys are dropped, new keys use `initial`, and values are repaired
left-to-right, reserving room for all later thumbs. Collection order determines
value order and permanent tab order. Keep this order stable throughout a gesture.

The setter replaces the keyed snapshot, drops unknown keys and defaults missing
keys to `initial`. It and `set_disabled` cancel dragging silently, with no out
messages. Per-thumb disabling forbids user movement, including nearest-thumb
picking, but that thumb's value still constrains its neighbours. Globally or
individually disabled thumbs use `tabindex=-1` and unsuccessful hidden inputs.

## Numeric contract

All numbers must be finite. `step<=0`, `min>=max`, negative minimum distance,
`large_step<1`, duplicate keys, blank names/idrefs, nonfinite initial/setter
values, or insufficient room for `(thumb_count-1)*min_distance` steps raise
`Invalid_argument`. Empty collections are valid. The grid must contain at
least one step, no more than `2^53-1` steps, and have representable increments.
At most 100 decimal places (the native `toFixed` limit) are supported; unsupported grids also raise
`Invalid_argument`, rather than silently degrading precision.

Snapping is relative to `min`, with nearest-step rounding and half steps toward
the larger value (a 1e-9 tick tolerance compensates ordinary division drift).
The result is rounded to the maximum decimal precision of `min` and `step`,
including exponent notation. Keeping `min`'s precision preserves offset grids:
`min=0.05, step=0.1` yields `0.05, 0.15, 0.25`, not `0.1, 0.2, 0.3`.
`0.1 + 0.2` with `step=0.1` serializes as `0.3`.
The effective maximum is the last aligned grid point not beyond `max`;
non-aligned `max` is not an extra off-grid step. Minimum distance is an integer
number of steps. Programmatic values are clamped, snapped and spacing-repaired.

User changes emit `Changed(values)` only when the value actually changes.
Each keyboard change then emits `Committed(values)` immediately. Pointer
movement emits `Changed` at most once per frame; pointer up applies its final
coordinate and emits one `Committed` if the gesture changed anything, even if
it returned to its starting value. Pointer cancellation/lost capture/window blur commits the
last applied values without reverting. Bound/no-op interactions emit nothing.
Output snapshots are immutable, keyed and in collection order.

## Keyboard

| Key | Action |
| --- | --- |
| Right / Up | Increase one step |
| Left / Down | Decrease one step |
| RTL Right / Left | Decrease / increase one step |
| Shift + arrow | Same direction, `large_step` steps |
| PageUp / PageDown | Increase / decrease `large_step` steps |
| Home / End | Focused thumb's allowed minimum / maximum |
| Tab / Shift+Tab | Native focus traversal; never prevented |
| Ctrl / Alt / Meta chords, IME | Never intercepted |

Up/Down have the same meaning in both orientations. Horizontal arrows remain
available in vertical orientation, matching the APG keyboard table. All value
changes respect neighbour spacing, including Home/End and large steps.
Focus is retained on the thumb. Disabled owned keys cannot change values.
Escape has no special drag action; it is not intercepted unless the caller hook
chooses to handle it (without preventing default).

## Pointer and touch

Render thumbs inside the track and apply all part attributes exactly once.
The track owns `pointerdown`; each thumb owns `keydown`. Use neutral elements,
not native buttons/range inputs. Extra attributes must not override owned
semantics, events, IDs or behavioural styles. Do not put interactive children
inside thumbs. Use real, distinct thumb names, e.g. “Minimum price” and
“Maximum price”, and reference existing visible labels where available.

Primary-button pointerdown chooses the nearest enabled thumb, with ties favouring
a thumb able to move toward the pointer. The selected thumb moves to the pointer
and receives post-render focus. Coordinates are viewport-relative. Horizontal
RTL reverses the axis; vertical increases bottom-to-top. Coordinates outside the
track clamp to the grid and neighbour limits.

Track geometry comes from `Browser_dom.get_element` after render. Capture-phase
document listeners are installed synchronously when the drag intent is processed,
so movement/release cannot be missed while measurement is pending. The final
release coordinate supersedes queued movement. Multiple complete clicks before
one frame retain separate measurements/commits. Scroll and resize refresh the
rectangle. A zero-length track deterministically maps to its start (end for RTL),
never NaN. Missing tracks abort measurement cleanly.

This uses **document-level capture-phase listeners plus native pointer capture**
on the track when supported. Capture is acquired synchronously for trusted native
pointerdown only: synthetic events cannot establish an active native pointer and
must not call `setPointerCapture`. Unsupported environments still use document
listeners. Capture retains native movement/releases outside the document; the
listeners also handle implicit touch-capture retargeting. Lost track capture,
pointer cancellation, and window blur terminate dragging; descendant element blur
does not. Pointer identity rejects other fingers; a second pointerdown does not
steal an active gesture. Subscription disposal removes all document/window
listeners, releases retained native capture and cancels pending move frames;
post-render Browser_dom tasks cannot restart a disposed drag.

The track sets inline `touch-action: none`. This is a behavioural exception to
the no-presentation-inline-style rule: it disables browser panning/zoom gestures
starting on the track, allowing reliable touch drags. Do not remove it or replace
it with CSS that re-enables panning. Chromium tests use native CDP touch input
and verify that a horizontal drag with vertical finger movement does not scroll.

## ARIA, parts and geometry

Each thumb has `role=slider`, a tab stop when enabled, per-thumb `aria-label` or
`aria-labelledby`, `aria-valuenow`, neighbour-constrained `aria-valuemin/max`,
`aria-valuetext`, explicit `aria-orientation` and `aria-disabled=true|false`.
The reported neighbour limits include minimum-distance steps and update whenever
another thumb changes. Every enabled thumb remains a separate tab stop.

| Part | Styling hooks |
| --- | --- |
| root | `data-orientation`, presence `data-disabled`, `data-dragging`; document direction |
| track | Same data flags; inline `touch-action:none` |
| range | Same data flags; `--tea-slider-start`, `--tea-slider-end` |
| thumb.root | `data-orientation`; per-thumb `data-disabled`, active-thumb `data-dragging`; `--tea-slider-percent` |
| thumb.input | Native hidden input attributes; no styling hooks |

All geometry variables contain percentages. Single fill starts at 0; range fill
runs from the first to last thumb. Percentages are logical min-to-max fractions;
use `right` for horizontal RTL and `bottom` for vertical presentation. No
`data-focus` is added: use `:focus-visible`. No CSS is required for semantics,
value constraints or focus; the example's CSS only draws the tracks and handles.

With `~name`, render one `input` bundle per thumb. A single thumb uses `name`
unchanged; multiple thumbs use `name[key]` in collection order. Keys are literal
in form field names (only DOM IDs are escaped); use form-safe keys if your server
parses bracket notation. Without `name`, omit the input elements. Hidden fields
are parent-controlled mirrors, not native form-reset or constraint-validation
controls. Handle form reset in the parent with `set_values` if desired.

## Tests and limits

- `dune build @test/ui/slider/runtest --force`: real runtime/events in jsdom,
  numeric/configuration invariants, every key and modifier, rapid same-frame
  messages, collection repair, empty/all-disabled states, pointer races,
  geometry refresh, form fields, focus and listener/frame disposal.
- `TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs slider`:
  actual example, keyboard table, tab order, constrained and out-of-track drags,
  RTL/vertical interaction, native capture/lost-capture cleanup, hidden FormData,
  touch/no-scroll, ARIA snapshots and
  unfiltered axe with zero violations in each exercised state, including drag.

As the APG warns, touch-based assistive technologies may not synthesize slider
key events consistently; test actual target devices and screen readers before
shipping. Chromium automation does not establish Safari/iOS/AT compatibility.
IDs and document sources address the current document, not shadow roots/iframes.
Transforms/layout changes without scroll/resize are not automatically observed:
keep track geometry stable during a drag. Escape does not revert a gesture.
The numeric precision/grid bounds above are deliberate validation limits.
