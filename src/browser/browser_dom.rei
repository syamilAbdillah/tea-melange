/** DOM tasks addressed by document-unique ids. Every task is lazy and runs
    after the next render, via [Tea_dom.Browser_platform.request_animation_frame].
    Synchronous tabbable selection is the explicit read-only exception.
    Use [Task.attempt] to handle missing elements. No DOM is read at construction. */

/** An id did not resolve at execution time (or [focus_first] had no candidate). */
type error =
  | Not_found(string);
/** A viewport-relative bounding rectangle, in CSS pixels. */
type rect = {
  /** Left edge. */
  x: float,
  /** Top edge. */
  y: float,
  /** Width. */
  width: float,
  /** Height. */
  height: float,
  /** Top edge. */
  top: float,
  /** Right edge. */
  right: float,
  /** Bottom edge. */
  bottom: float,
  /** Left edge. */
  left: float,
};
/** Geometry read in one post-render callback. */
type element_info = {
  /** Element rectangle relative to the viewport. */
  rect,
  /** Layout viewport width. */
  viewport_width: float,
  /** Layout viewport height. */
  viewport_height: float,
  /** Document horizontal scroll offset. */
  scroll_x: float,
  /** Document vertical scroll offset. */
  scroll_y: float,
};
/** Focus an element; [prevent_scroll] defaults to false. */
let focus: (~prevent_scroll: bool=?, string) => Tea_core.Task.t(unit, error);
/** Blur an element. */
let blur: string => Tea_core.Task.t(unit, error);
/** Focus, ignoring [Not_found]. */
let focus_cmd: string => Tea_core.Cmd.t('msg);
/** Synchronously select the first tabbable
    descendant of [root], excluding root itself. Returns [None] when empty.
    Reads the current DOM without scheduling work or moving focus; callers own
    render timing and cancellation. [exclude] defaults to accepting every node
    and can skip overlay boundary sentinels.

    Positive tabindex sorts ascending, then zero/implicit stops in DOM order;
    equal priorities retain DOM order. Skips negative tabindex, disabled controls
    (including disabled fieldsets), hidden inputs, hidden/inert ancestors,
    display:none, visibility:hidden/collapse and closed details content outside
    its first summary. Named radio groups use the checked enabled member, even
    outside root, matched by name and form owner; without one, entry follows the
    requested direction. Visibility checks conservatively include ancestors.
    Searches ordinary light DOM in the current document, without shadow roots
    or iframe traversal; no layout-box measurement is required (works in jsdom). */
let first_tabbable:
  (~exclude: Dom.node => bool=?, Dom.node) => option(Dom.node);
/** Synchronously select the last tabbable descendant in the same order and
    with the same exclusions as [first_tabbable]. */
let last_tabbable:
  (~exclude: Dom.node => bool=?, Dom.node) => option(Dom.node);
/** After render, focus the first descendant selected by [first_tabbable].
    Returns [Not_found(within)] if the root or a candidate is missing. */
let focus_first: (~within: string) => Tea_core.Task.t(unit, error);
/** Scroll into view; block defaults to [`Nearest]. Unsupported APIs are no-ops. */
let scroll_into_view:
  (
    ~block:
      [
        | `Nearest
        | `Center
        | `Start
        | `End
      ]
        =?,
    string
  ) =>
  Tea_core.Task.t(unit, error);
/** Read geometry, viewport dimensions and document scroll offsets. */
let get_element: string => Tea_core.Task.t(element_info, error);
/** Open a modal dialog. Already-open dialogs are left alone. Without native
    dialog support, sets [open]; the fallback does not provide modality. */
let show_modal: string => Tea_core.Task.t(unit, error);
/** Open a nonmodal dialog, falling back to [open]. */
let show: string => Tea_core.Task.t(unit, error);
/** Close a dialog, falling back to removing [open]. Already-closed is a no-op. */
let close: string => Tea_core.Task.t(unit, error);
/** Await a snapshot of subtree animations with all-settled semantics. Missing
    getAnimations, empty snapshots, cancellation and removal all resolve.
    A 5000 ms watchdog bounds paused, infinite or broken animation promises.
    Observers/timers are released on every completion path; completes once. */
let wait_for_animations: string => Tea_core.Task.t(unit, error);
/** [wait_for_animations] that can be abandoned: the returned function cancels
    it, releasing its observer and watchdog at once and suppressing completion.
    Subscriptions use this so stopping leaves no timers or observers behind. */
let watch_animations: (string, result(unit, error) => unit, unit) => unit;
/** Id of the active element after render; [None] for no element or an empty id. */
let active_element_id: Tea_core.Task.t(option(string), 'e);
