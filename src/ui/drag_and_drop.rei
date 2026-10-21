/** Pointer sorting and accessible button-based moving. APG button pattern:
    https://www.w3.org/WAI/ARIA/apg/patterns/button/
    There is no APG drag-and-drop pattern; this is NOT a listbox.

    Keyboard table: Tab/Shift+Tab navigate normally (and cancel a lift);
    Space/Enter lift or drop; Escape cancels; Up/Down move in vertical lists;
    Left/Right move in horizontal lists (RTL reversed); Shift+Left/Right
    move across vertical containers (RTL reversed), Shift+Up/Down across
    horizontal containers. Repeated activation, IME and Ctrl/Alt/Meta chords
    are ignored. Arrows are owned only while lifted.

    Data attributes: data-dragging on the item/handle, data-over on the target
    container, data-disabled, data-orientation. Geometry only: --tea-drag-x/y.
    Render instructions and live text from the parts; do not hide the live
    region with hidden/display:none. No deprecated grabbed/dropeffect ARIA. */

/** Main axis of insertion and keyboard movement. */
type orientation =
  | Vertical
  | Horizontal;
/** Horizontal writing direction. */
type direction =
  | Ltr
  | Rtl;
/** Stable globally unique item key and accessible announcement label. */
type item = {
  /** Stable globally unique identity. */
  key: string,
  /** Name used in announcements. */
  label: string,
  /** Cannot lift; remains an insertion boundary. */
  disabled: bool,
};
/** Parent-owned list. Keys must be unique across this instance. */
type container = {
  /** Stable list identity. */
  key: string,
  /** Accessible list name. */
  label: string,
  /** Forbids lifting from or dropping into this list. */
  disabled: bool,
  /** Current parent-owned order. */
  items: array(item),
};
/** Drop insertion index, in the target list after removing the dragged item. */
type target = {
  /** Destination container key. */
  container: string,
  /** Zero-based slot after removing the dragged key. */
  index: int,
};
/** Committed movement; parent must apply this to its collections. */
type movement = {
  /** Moved item identity. */
  key: string,
  /** Current source container. */
  from_container: string,
  /** Current source index before removal. */
  from_index: int,
  /** Destination container. */
  to_container: string,
  /** Insertion index after removal. */
  to_index: int,
};
/** Semantic notifications. Moved is emitted only on drop, even for a no-op. */
type out_msg =
  | Drag_started(string)
  | Moved(movement)
  | Cancelled;
/** Announcement localization callbacks. Positions are one-based. */
type labels = {
  /** Operating instructions referenced by handles. */
  instructions: string,
  /** Item label, one-based position, total count. */
  picked_up: (string, int, int) => string,
  /** Item label, destination label, one-based position, total count. */
  moved: (string, string, int, int) => string,
  /** Successful drop confirmation. */
  dropped: string,
  /** Item label and original one-based position. */
  cancelled: (string, int) => string,
};
/** English instructions and announcements. */
let default_labels: labels;
/** Opaque transient interaction state; never stores collections. */
type model;
/** Opaque intent messages, mapped by the parent. */
type msg;
/** Initialize. Threshold is CSS pixels (default 6); touch delay defaults to
    250 milliseconds. Negative configuration values are clamped to zero.
    [drag_from_item=false] makes only handles initiate pointer dragging. */
let init:
  (
    ~id: string,
    ~orientation: orientation=?,
    ~dir: direction=?,
    ~threshold: float=?,
    ~touch_delay: int=?,
    ~drag_from_item: bool=?,
    ~labels: labels=?,
    ~assertive: bool=?,
    unit
  ) =>
  model;
/** Current valid in-flight target, absent when idle or pointer outside all lists. */
let preview: model => option(target);
/** Dragged key, absent during the pre-threshold press. */
let dragging: model => option(string);
/** Current live announcement. */
let announcement: model => string;
/** Apply an intent using the latest collections. Removing/disabling the source
    cancels. Target indices are repaired before every operation. */
let update:
  (~containers: array(container), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Lifecycle-owned pointer listeners, touch timer, scroll/resize measurement,
    visibility/blur cancellation and auto-scroll frames. A cancelled pointer
    gesture retains a capture click guard until its release/click; a post-release
    frame expires it if no click arrives. Shutdown always disposes everything. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Programmatically cancel without emitting out messages. Restores handle focus. */
let cancel: model => (model, Tea_core.Cmd.t(msg));
/** Attribute bundles in the parent's message type. */
type parts('parent) = {
  /** Apply to each list container. Owns id and state; label uses aria-label. */
  container: string => list(Tea_html.Html.attr('parent)),
  /** Apply to each item. Owns pointerdown/dragstart and geometry. */
  item: string => list(Tea_html.Html.attr('parent)),
  /** Apply to a native button with a visible accessible name. Owns pointerdown,
      click, keydown/keyup, type, disabled, id and describedby. */
  handle: string => list(Tea_html.Html.attr('parent)),
  /** Apply to a p/div containing instructions_text. */
  instructions: list(Tea_html.Html.attr('parent)),
  /** Apply to an always-mounted div containing live_text. */
  live: list(Tea_html.Html.attr('parent)),
  /** Localized keyboard instructions. */
  instructions_text: string,
  /** Localized current announcement. */
  live_text: string,
  /** In-flight insertion target for an optional aria-hidden placeholder. */
  preview: option(target),
  /** Active key; keep the real item mounted until drop. */
  dragging: option(string),
  /** Geometry-only CSS variables for an optional aria-hidden overlay. */
  overlay: list(Tea_html.Html.attr('parent)),
};
/** Render headlessly. All supplied parts use to_parent, never internal messages.
    Keep item geometry unchanged during preview; apply Moved on drop. */
let view:
  (
    ~model: model,
    ~containers: array(container),
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
