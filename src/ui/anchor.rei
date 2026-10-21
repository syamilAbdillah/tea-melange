/** Headless viewport positioning, not an interactive widget.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/ (consumers own their pattern;
    e.g. https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/).

    Keyboard table: every key, including Tab/Shift+Tab, Escape, shortcuts and
    IME: unchanged. No listeners, focus management, roles or ARIA are added by
    positioning; consumers implement their own keyboard/ARIA contract.

    Data attributes: data-placement="side-alignment"; data-reference-hidden
    (presence). Geometry styles are owned by [attrs]. No stylesheet is needed.
    Inactive elements use hidden; active elements use visibility:hidden until
    measured and while the reference is empty or outside the viewport.

    Opt-in [top_layer] uses popover=manual and showPopover/hidePopover to escape
    clipping and transformed ancestors without moving DOM nodes. Unsupported
    browsers fall back to plain fixed positioning, which cannot escape a
    transformed containing block. See [docs/ui/anchor.md] for lifecycle rules. */

/** Physical preferred side. */
type side =
  | Top
  | Bottom
  | Left
  | Right;
/** Cross-axis alignment; horizontal start/end are logical in RTL. */
type alignment =
  | Start
  | Center
  | End;
/** Side and alignment. Flipping preserves logical alignment. */
type placement = (side, alignment);
/** Horizontal writing direction. Vertical writing modes are not inferred. */
type direction =
  | Ltr
  | Rtl;
/** Viewport-relative rectangle in CSS pixels. */
type rect = {
  /** Left. */
  x: float,
  /** Top. */
  y: float,
  /** Nonnegative width. */
  width: float,
  /** Nonnegative height. */
  height: float,
};
/** Floating border-box dimensions in CSS pixels. */
type size = {
  /** Width. */
  width: float,
  /** Height. */
  height: float,
};
/** Geometric reasons for hiding a measured reference. */
type hidden_reason =
  | Reference_outside_viewport
  | Reference_empty;
/** Pure computed geometry. Nothing is rounded until rendering attributes. */
type position = {
  /** Viewport left. */
  x: float,
  /** Viewport top. */
  y: float,
  /** Effective floating width (reference width when matching). */
  width: float,
  /** Measured floating height. */
  height: float,
  /** Resolved side and original logical alignment. */
  placement,
  /** Reference border-box width. */
  anchor_width: float,
  /** Reference border-box height. */
  anchor_height: float,
  /** Padded usable width on the chosen side. */
  available_width: float,
  /** Padded usable height on the chosen side. */
  available_height: float,
  /** True when hidden_reason is present. Partial viewport intersection is visible. */
  reference_hidden: bool,
  /** Does not detect clipping by intermediate ancestors. */
  hidden_reason: option(hidden_reason),
  /** Optional arrow leading edge on the cross axis, relative to the floating
      border box, clamped inside it. The caller renders the arrow. */
  arrow_offset: option(float),
};
/** Compute without DOM access. Defaults: offset=0, flip=true,
    shift_padding=Some(8), match_width=false, dir=Ltr, no arrow.
    None disables shifting and uses zero viewport padding. Padding is capped at
    half each viewport dimension. Oversized floating elements pin their cross
    start to the padded viewport start; they are not resized. Main-axis overflow
    is left for consumers to constrain using available sizes. Flip only when the
    preferred side lacks space and its opposite offers strictly more space;
    ties retain the preferred side. Negative offsets permit overlap. Nonfinite
    inputs become zero and negative dimensions become zero. */
let compute:
  (
    ~anchor: rect,
    ~floating: size,
    ~viewport: rect,
    ~placement: placement,
    ~offset: float=?,
    ~flip: bool=?,
    ~shift_padding: option(float)=?,
    ~match_width: bool=?,
    ~dir: direction=?,
    ~arrow_size: float=?,
    unit
  ) =>
  position;
/** Canonical data-placement value, including center (e.g. bottom-center). */
let placement_name: placement => string;
/** Immutable configuration, lifecycle and latest measurement. */
type model;
/** Internal measurement completion; stale lifecycle generations are ignored. */
type msg;
/** Measurement notifications, not open/close or domain-selection changes. */
type out_msg =
  | Positioned(position)
  | Unavailable;
/** Create inactive. Both ids must be distinct and document-unique. Defaults
    match [compute], with placement=(Bottom, Start) and top_layer=false.
    Elements must be mounted when started; call refresh after replacing them.
    Do not share a floating id between Anchor instances. */
let init:
  (
    ~anchor_id: string,
    ~floating_id: string,
    ~placement: placement=?,
    ~offset: float=?,
    ~flip: bool=?,
    ~shift_padding: option(float)=?,
    ~match_width: bool=?,
    ~dir: direction=?,
    ~top_layer: bool=?,
    ~arrow_size: float=?,
    unit
  ) =>
  model;
/** Whether measurements/subscriptions are requested. */
let active: model => bool;
/** Latest measurement; None before first measurement or for missing elements. */
let position: model => option(position);
/** Activate idempotently. Mount the elements and include subscriptions in the
    same parent update. No focus is moved. Commands use the component msg type. */
let start: model => (model, Tea_core.Cmd.t(msg));
/** Deactivate idempotently; subscriptions synchronously dispose listeners and
    hide a promoted popover, including on shutdown. attrs hides the element. */
let stop: model => (model, Tea_core.Cmd.t(msg));
/** Restart measurement/observers after manual layout changes or node replacement.
    Does not activate an inactive model; clears the previous position. */
let refresh: model => (model, Tea_core.Cmd.t(msg));
/** Change only supplied options and restart measurement if active. For the
    optional arrow pass Some(size), or None to remove it. shift_padding=None
    disables shifting. No out messages are emitted programmatically. */
let configure:
  (
    ~placement: placement=?,
    ~offset: float=?,
    ~flip: bool=?,
    ~shift_padding: option(float)=?,
    ~match_width: bool=?,
    ~dir: direction=?,
    ~top_layer: bool=?,
    ~arrow_size: option(float)=?,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg));
/** Accept the newest live measurement, producing Positioned or Unavailable.
    Measurement callbacks dispatch only on actual geometry/DPR change (plus
    the first result per activation/refresh). Missing elements emit Unavailable
    once and retry on subsequent resize/scroll or explicit refresh. */
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Lifecycle-owned initial post-render measurement, window resize, any captured
    document scroll and anchor/floating ResizeObservers. Sources feed a shared
    pending-frame gate, not parent messages. Reads of both nodes are scheduled
    in the same frame via Browser_dom.get_element. Stopping suppresses pending
    callbacks. No continuous animation/layout polling. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Parent-message-typed floating attrs. Owns id, hidden, popover (when opted in),
    position, inset, margin, box-sizing, left/top, width, visibility and four CSS
    variables: --tea-anchor-width/height, --tea-available-width/height. left/top
    round to device pixels. Use intrinsic/CSS width unless match_width is set.
    Append only nonconflicting attrs; roles and ARIA belong to the consumer.
    A closed native popover is never displayed by a stylesheet workaround.
    Native popovers with autofocus descendants may invoke browser autofocus;
    omit autofocus when the consumer must retain trigger focus. */
let attrs: model => list(Tea_html.Html.attr('parent));
/** Wrapper-free rendering bundle. No owned DOM events. */
type parts('parent) = {
  /** Identical to attrs(model). */
  floating: list(Tea_html.Html.attr('parent)),
  /** Latest pure geometry for consumer state/arrow rendering. */
  position: option(position),
};
/** Conventional parts callback. to_parent is accepted for uniform composition;
    the bundle currently owns no events to map. */
let view:
  (
    ~model: model,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
