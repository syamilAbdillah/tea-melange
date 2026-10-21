/** Headless keyed single- and multi-thumb slider.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/slider/
    and https://www.w3.org/WAI/ARIA/apg/patterns/slider-multithumb/.

    Keyboard table: Right/Up add one step; Left/Down subtract one step.
    RTL swaps Left/Right. PageUp/PageDown add/subtract [large_step] steps.
    Shift+arrow uses [large_step]. Home/End select the focused thumb's allowed
    minimum/maximum, including neighbours and minimum distance. Tab/Shift+Tab
    use native navigation in collection order; shortcuts and IME are untouched.
    Horizontal arrows also work in vertical orientation (APG slider model).

    Data attributes: data-orientation="horizontal|vertical", data-disabled,
    data-dragging. No data-focus: use :focus-visible. Inline styles are geometry
    custom properties and behavioural touch-action:none on the track.
    Track owns pointerdown; thumbs own keydown. Render each thumb inside track.
    Use neutral elements, not buttons or native range inputs. */

/** Axis; defaults to [Horizontal]. */
type orientation =
  | Horizontal
  | Vertical;
/** Writing direction; defaults to [Ltr]. */
type direction =
  | Ltr
  | Rtl;
/** Accessible thumb name. Multi-thumb names must distinguish the thumbs.
    Labelled_by references nonempty existing document ids. */
type label =
  | Label(string)
  | Labelled_by(list(string));
/** Parent-owned collection entry, in permanent tab/value order. [initial] is
    used at init and when a new key appears. Disabled thumbs cannot be moved by
    users and are skipped by track picking; they still constrain neighbours. */
type thumb = {
  /** Stable unique key, used for ids and values. */
  key: string,
  /** Accessible name. */
  label,
  /** Finite initial domain value. */
  initial: float,
  /** Whether user changes are forbidden. */
  disabled: bool,
};
/** Keyed domain values, in the supplied collection order. */
type values = list((string, float));
/** Opaque submodel; no collection or event listeners are stored here. */
type model;
/** Opaque intent messages; map with the parent's constructor. */
type msg;
/** Changed is emitted for each actual user value change. Keyboard changes also
    emit Committed immediately. Pointer up/cancel, lost capture or window blur commits once
    if that gesture changed any value. Bounds/no-op changes emit nothing. */
type out_msg =
  | Changed(values)
  | Committed(values);
/** Initialise and clamp/snap initial values. Defaults: min=0, max=100, step=1,
    min_distance=0 (in steps), large_step=10. The effective maximum is the last
    step-aligned value <= max. Rounding uses the decimal precision of step and
    min (maximum 100 places, the native toFixed limit), eliminating ordinary binary float drift.
    Raises Invalid_argument for nonfinite numbers, min>=max, step<=0, negative
    distance, large_step<1, duplicate keys, empty names, insufficient range, or
    an unrepresentable grid (>2^53-1 steps, zero steps, >100 decimal places).
    Collections may be empty. Values are repaired left-to-right, reserving room
    for remaining thumbs. No out messages are emitted by programmatic control. */
let init:
  (
    ~id: string,
    ~items: array(thumb),
    ~min: float=?,
    ~max: float=?,
    ~step: float=?,
    ~min_distance: int=?,
    ~large_step: int=?,
    ~orientation: orientation=?,
    ~dir: direction=?,
    ~disabled: bool=?,
    unit
  ) =>
  model;
/** Reconcile the latest collection and apply one intent. Removed/disabled
    dragging keys cancel the gesture. Pass the same items to update and view. */
let update:
  (~items: array(thumb), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Lifecycle-owned drag listeners; none when idle. Always map into the parent
    subscriptions. Shutdown/removing the submodel disposes listeners and frames. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Repaired snapshot for the current collection. */
let values: (~items: array(thumb), model) => values;
/** Replace keyed values, clamp/snap and repair neighbour spacing. Unknown keys
    are dropped; missing keys use initial values. Cancels dragging, emits no
    out messages. Nonfinite values raise Invalid_argument. */
let set_values:
  (~items: array(thumb), ~values: values, model) =>
  (model, Tea_core.Cmd.t(msg));
/** Enable/disable all thumbs; cancels dragging without emitting out messages. */
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));
/** One thumb's derived state and parent-message attribute bundles. */
type thumb_parts('parent) = {
  /** Stable key; use as the VDOM key. */
  key: string,
  /** Repaired snapped value. */
  value: float,
  /** Geometric percentage in the configured min/max range. */
  percent: float,
  /** Neutral focusable slider element; owns keydown and semantics.
      Geometry: --tea-slider-percent (percentage). */
  root: list(Tea_html.Html.attr('parent)),
  /** Hidden input; render only when name was supplied to view. Disabled inputs
      are unsuccessful. Single thumb uses name; multiple use name[key]. */
  input: list(Tea_html.Html.attr('parent)),
};
/** Attribute bundles. All thumb elements belong inside track. */
type parts('parent) = {
  /** Outer container; id, direction and state data attributes. */
  root: list(Tea_html.Html.attr('parent)),
  /** Measured track; owns pointerdown and touch-action:none. Do not override
      touch-action: it prevents touch panning during a slider gesture. */
  track: list(Tea_html.Html.attr('parent)),
  /** Fill; --tea-slider-start/end percentages (0 to thumb for single mode,
      first to last thumb for range). */
  range: list(Tea_html.Html.attr('parent)),
  /** Thumb bundles in collection order, never reordered by values. */
  thumbs: array(thumb_parts('parent)),
};
/** Render without wrappers. Formatter receives key and value for aria-valuetext
    (defaults to decimal number). Unhandled plain non-Tab keys go to the hook;
    hook messages do not prevent default. Required keys take precedence.
    Pointerdown chooses nearest enabled thumb, ties preferring a thumb able to
    move toward the pointer. Geometry is measured after render; document capture
    listeners retain early moves/releases. Trusted native pointers also capture
    on the track when supported; lost capture terminates the gesture. Synthetic
    events use document listeners without attempting native capture.
    Moves are coalesced per animation
    frame; release applies its final coordinate before committing. Scroll/resize
    refresh geometry. Escape does not cancel; pointercancel/blur commits the
    current values. Track clicks focus the selected thumb after render. */
let view:
  (
    ~model: model,
    ~items: array(thumb),
    ~to_parent: msg => 'parent,
    ~name: string=?,
    ~format_value: (string, float) => string=?,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
