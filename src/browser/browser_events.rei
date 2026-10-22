/** Lifecycle-owned document/window sources. No manual disposer is needed:
    return [Sub.none] to stop, or shut down the program. Starts that need DOM
    nodes run after render; stopping cancels pending starts and removes listeners
    synchronously. Kept identities use the newest tagger/decoder.
    Source configuration is encoded in identities, except explicit user keys. */

/** Layout viewport size in CSS pixels. */
type viewport = {
  /** Viewport width. */
  width: float,
  /** Viewport height. */
  height: float,
};
/** An observed element's content-box size in CSS pixels. */
type element_size = {
  /** Content width. */
  width: float,
  /** Content height. */
  height: float,
};
/** Document keydown, filtered by a decoder over the raw event.
    [key] is a caller-chosen subscription identity, NOT a keyboard key.
    Identity: ("Browser_events.keydown.capture" or ".bubble", key).
    Changing the decoder does not restart the listener; failure emits nothing.
    Capture defaults to false. This listener never prevents browser defaults. */
let on_key_down:
  (~key: string, ~capture: bool=?, Tea_core.Decode.t('msg)) =>
  Tea_core.Sub.t('msg);
/** Capture-phase pointerdown outside every listed id; optional focusin as well.
    Missing ids never contain the target; nested ids are safe. Identity:
    ("Browser_events.outside", length-encoded ids plus focus flag).
    [focus] defaults to false. The newest message is used without restarting. */
let on_pointer_down_outside:
  (~ids: list(string), ~focus: bool=?, 'msg) => Tea_core.Sub.t('msg);
/** Window resize. Identity: ("Browser_events.resize", "window"). */
let on_resize: (viewport => 'msg) => Tea_core.Sub.t('msg);
/** Any scroll container's scroll, captured on document. Identity:
    ("Browser_events.scroll", "document"). Read geometry with [Browser_dom]. */
let on_scroll: 'msg => Tea_core.Sub.t('msg);
/** Document visibility, true when visible; changes only, not an initial event.
    Identity: ("Browser_events.visibility", "document"). */
let on_visibility_change: (bool => 'msg) => Tea_core.Sub.t('msg);
/** Observe the id after render. Missing elements and unsupported ResizeObserver
    produce no events. Identity: ("Browser_events.element_resize", id).
    Re-add the subscription to observe a replacement element with the same id. */
let on_element_resize:
  (~id: string, element_size => 'msg) => Tea_core.Sub.t('msg);
/** Deliver the current match after render and every change. Unsupported
    matchMedia delivers false once. Supports legacy addListener/removeListener.
    Identity: ("Browser_events.media_query", query). */
let on_media_query: (~query: string, bool => 'msg) => Tea_core.Sub.t('msg);
/** Reference-counted scroll lock. Identity: ("Browser_events.scroll_lock", key).
    Acquires/releases after render, compensates the scrollbar with body padding,
    and restores the exact prior html/body inline style attributes on final stop.
    On iOS/iPadOS only, a fixed body preserves/restores scroll position; other
    browsers use overflow:hidden without a position hack. Do not mutate html/body
    inline styles while a lock owns them. Independent keys and runtimes share
    the lock; stop/shutdown release only their own lease. */
let scroll_lock: (~key: string) => Tea_core.Sub.t('msg);
/** A post-render focus destination, resolved only when the frame runs. */
type focus_target =
  | /** Element with the given document id. */
    Element(string)
  | /** First sequential tab stop below the root id. */
    First_tabbable(string)
  | /** Last sequential tab stop below the root id. */
    Last_tabbable(string);
/** One cancellable focus attempt after the next render. Identity:
    [("Browser_events.focus", key)]. Include an immutable generation in [key]
    for each new request. [ready] defaults to true; while false, each render-frame
    checks the latest readiness without losing the opening focus snapshot. A
    false-to-true readiness transition crosses another frame so its render lands
    before focus. Target,
    readiness and completion tagger refresh for a kept key; [from] is fixed at
    start. A superseding focus movement completes false even before readiness.
    Captures activeElement synchronously on start and never overrides a focus
    movement before the frame, including moving away and back. Removal of the
    remembered node by render may leave focus on body and does not cancel.
    Optional [from] restricts focus to these id subtrees (body is also allowed);
    use an opening trigger to guard an attempt delayed by layout readiness.
    [prevent_scroll] defaults to true. Delivers whether the target took focus,
    including false for missing targets or superseded attempts. Remove the
    source on completion. Removal/shutdown cancels work and removes its focusin
    listeners immediately; no DOM state or mutable owner belongs in the model. */
let focus:
  (
    ~key: string,
    ~target: focus_target,
    ~ready: bool=?,
    ~from: list(string)=?,
    ~prevent_scroll: bool=?,
    bool => 'msg
  ) =>
  Tea_core.Sub.t('msg);
/** Remember activeElement on start, before the opening render. After the
    closing render, refocus it if connected and focus is on body, absent,
    disconnected, or inside the element with id [within] (an overlay that stays
    mounted while it animates out). Never steals focus from any other element.
    Identity: ("Browser_events.restore_focus", key). */
let restore_focus:
  (~key: string, ~within: string=?, unit) => Tea_core.Sub.t('msg);
