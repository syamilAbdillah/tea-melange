/** Windowed keyed collections with fixed or measured border-box row heights.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/listbox/ and
    https://www.w3.org/WAI/ARIA/apg/practices/keyboard-interface/#kbd_focus_activedescendant

    Keyboard table: every key, including Tab/Shift+Tab, arrows, Home/End,
    PageUp/PageDown, modifiers and IME, is untouched. Virtualization has no
    APG role or keyboard bindings; the consumer implements its chosen pattern.
    Call [scroll_to_index] before moving focus/active descendant and include
    that index in [pinned] in the SAME render. Pinned rows always exist.

    Data: data-virtual-list on container; data-pinned on pinned rows;
    data-tea-virtual-key and data-tea-virtual-generation are internal observer
    identities, not styling hooks.
    Container owns scroll (an element-level subscription handler). Content must
    contain only keyed row elements. Apply no margins to rows, padding to the
    container/content, or borders to content. Container borders are supported;
    put decoration inside rows. Geometry styles are owned.
    Consumers choose roles, labels, tabindex, selection and keyboard behavior.
    Use [Html.keyed] on content with [row.key], preserving DOM identity. */

/** Pure persistent sparse prefix-sum tree. No DOM, clocks or mutation. */
module Geometry: {
  /** Immutable geometry; unmeasured rows use the estimate. */
  type t;
  /** Half-open rendered interval. Empty viewport/collection gives [first=last=0]. */
  type range = {
    /** Inclusive first index. */
    first: int,
    /** Exclusive last index (may include a boundary row). */
    last: int,
    /** Actual tree nodes visited by the two inverse-prefix queries. */
    operations: int,
  };
  /** O(1). Negative count clamps to zero; invalid estimate becomes one pixel. */
  let create: (~count: int, ~estimate: float) => t;
  /** O(log n), persistent. Invalid index/nonpositive/nonfinite size is ignored. */
  let measure: (~index: int, ~size: float, t) => t;
  /** O(log n). Index clamps to [0,count]; count gives the total. */
  let offset: (~index: int, t) => float;
  /** O(1), CSS pixels. */
  let total: t => float;
  /** O(log n), independent of scroll distance and collection length.
      Overscan defaults to two rows on each side. Scroll is clamped. */
  let range: (~viewport: float, ~scroll: float, ~overscan: int=?, t) => range;
};
/** Parent-owned immutable key collection with a derived key-to-index dictionary.
    Not stored in the submodel. Retain/reuse this value until keys/order change. */
type items;
/** Copy keys and build an O(n) index. Keys must be unique; duplicates raise
    Invalid_argument. Recreate after insertions/removals/reorders, not per view. */
let items: array(string) => items;
/** Number of rows, including absent DOM rows. */
let count: items => int;
/** Key at index, None for an invalid index. */
let key_at: (~index: int, items) => option(string);
/** Fixed height, or estimated height progressively replaced by ResizeObserver
    border-box measurements. Sizes must be positive finite CSS pixels. */
type row_size =
  | Fixed(float)
  | Estimated(float);
/** Alignment; Auto scrolls only when not fully visible. */
type align =
  | Start
  | Center
  | End
  | Auto;
/** Abstract submodel. Measurements are keyed, bounded by current item count;
    old keys are pruned on collection changes. No historical cache is retained. */
type model;
/** Opaque frame-coalesced scroll/resize intent. */
type msg;
/** One notification per delivered frame, after measurements and anchoring. */
type out_msg =
  | Viewport_changed(Geometry.range);
/** Initialize. Viewport defaults to zero (observer reads actual height),
    overscan defaults to two. Id is a document-unique prefix. */
let init:
  (
    ~id: string,
    ~items: items,
    ~row_size: row_size,
    ~viewport: float=?,
    ~overscan: int=?,
    unit
  ) =>
  model;
/** Current scroll offset in CSS pixels. */
let scroll_offset: model => float;
/** Current viewport height in CSS pixels. */
let viewport_height: model => float;
/** Total estimated/measured content size. */
let total_size: model => float;
/** Number of measured keys, at most current item count. */
let measurement_count: model => int;
/** Current ordinary window, excluding pinned rows. */
let visible_range: model => Geometry.range;
/** Row offset; index clamps to [0,count]. */
let row_offset: (~index: int, model) => float;
/** Stable, collision-free DOM id derived from the key. */
let row_id: (~key: string, model) => string;
/** Repair derived geometry/prune measurements after collection replacement.
    Call in the parent's collection-changing update, even without a DOM event.
    Also performed automatically by update/view/scroll commands. No out message. */
let refresh: (~items: items, model) => (model, Tea_core.Cmd.t(msg));
/** Deliver current items, never a stale captured array. A measurement batch
    preserves the first visible row's pixel position when preceding rows grow.
    User scrolling cancels pending alignment; programmatic scrolling refines its
    estimated target as rows are measured. */
let update:
  (~items: items, msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Lifecycle owner: one scroll listener on container, one ResizeObserver for
    container and variable rows, one child-list MutationObserver to reconcile
    variable rows. Fixed lists observe only the container (no MutationObserver).
    All observations and pending deliveries stop synchronously on shutdown.
    If ResizeObserver is unavailable, estimates remain usable. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Scroll even to an unrendered index; Auto is default. Invalid indices no-op.
    Returns the new model immediately; DOM write runs after render. Does not
    focus or change consumer selection. Keep the destination pinned if active. */
let scroll_to_index:
  (~items: items, ~index: int, ~align: align=?, model) =>
  (model, Tea_core.Cmd.t(msg));
/** As scroll_to_index, resolving a stable key; missing keys no-op. */
let scroll_to_key:
  (~items: items, ~key: string, ~align: align=?, model) =>
  (model, Tea_core.Cmd.t(msg));
/** Set emits aria-setsize/posinset for list/listbox/feed semantics. Grid emits
    aria-rowcount on container and aria-rowindex on rows instead. Supply the
    corresponding roles and (for grids) cell semantics yourself. */
type semantics =
  | Set
  | Grid;
/** One row attribute bundle, in the parent's message type. */
type row('parent) = {
  /** Use as the Html.keyed identity. */
  key: string,
  /** Absolute collection index, zero-based. */
  index: int,
  /** Top offset in CSS pixels. */
  offset: float,
  /** Id, aria-setsize/posinset (or grid rowindex), observer identity and geometry.
      Append the chosen row role (option/listitem/article etc.). */
  attrs: list(Tea_html.Html.attr('parent)),
};
/** Attribute bundles and rendered rows in collection order. */
type parts('parent) = {
  /** Scroll container; supply height, semantics and a focus policy. */
  container: list(Tea_html.Html.attr('parent)),
  /** Apply to Html.keyed containing only row elements. */
  content: list(Tea_html.Html.attr('parent)),
  /** Window union valid pinned indices, deduplicated and sorted. */
  rows: list(row('parent)),
  /** Ordinary window. */
  range: Geometry.range,
  /** Total height. */
  total: float,
};
/** Render through parts without adding wrappers. Final unit erases optionals.
    Pinned indices default to []; invalid indices are ignored. Bound their count
    in the consumer. to_parent follows the common API; scroll events originate
    in subscriptions, so no event fact currently needs that mapping. */
let view:
  (
    ~model: model,
    ~items: items,
    ~pinned: list(int)=?,
    ~semantics: semantics=?,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
