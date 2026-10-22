/** Pointer-hover intent with cancellable opening and shared closing delays.
    Related APG: https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/
    This primitive is not a tooltip: consumers own semantics, visibility,
    positioning, keyboard and focus behavior.

    Keyboard table: all keys, including Tab/Shift+Tab, Escape, Enter, Space,
    modifier chords and IME, are untouched. Consumers call [open_now] on focus
    and [close_now] on blur/Escape as appropriate to their own APG pattern.
    Focus is never moved. Touch pointers are ignored; mouse and pen hover.

    Data attributes on both parts: data-open, data-closed, data-disabled
    (presence flags). Owns pointerenter, pointerleave and pointercancel only.
    No ids, roles, ARIA, hidden property, focus/blur handlers or styles. */

/** Immutable, parent-owned warm-up window; no global or shared mutable state. */
module Group: {
  /** A group value may be passed to any number of intent models. */
  type t;
  /** Start cold. Negative durations clamp to zero; zero disables warm-up. */
  let init: (~skip_delay_ms: int, unit) => t;
  /** Remember a close, using monotonic milliseconds in the same time origin as
      PointerEvent.timeStamp (performance.now in modern browsers). Call after
      Intent_close with [last_close_time], then pass the returned group to all
      members' [update]. Programmatic closes can supply the consumer's timestamp.
      With [~key], also removes that member from the open members.
      Warm-up expires at exactly skip_delay_ms after the last close, provided no
      member is still open; backwards timestamps stay cold. */
  let record_close: (~key: string=?, ~time_stamp: float, t) => t;
  /** Remember that the member [key] is open. While any member is open the
      group is warm (unless warm-up is disabled), so moving the pointer to a
      sibling opens it without waiting for the first member's close delay.
      Pair with [record_close(~key, ...)] when that member closes. */
  let record_open: (~key: string, t) => t;
  /** Drop [key] from the open members without recording a close time, e.g.
      when a member unmounts or is closed programmatically. */
  let forget: (~key: string, t) => t;
  /** Keys currently recorded as open, most recent first. Consumers that show
      one popup at a time close the others when a member opens. */
  let open_keys: t => list(string);
};
/** Abstract immutable state, one per trigger/content pair. */
type model;
/** Opaque event intents; safe across multiple events before the next render. */
type msg;
/** Actual pointer-originated transitions, emitted once, never for setters. */
type out_msg =
  | Intent_open
  | Intent_close;
/** Initially closed. Delays are integer milliseconds, clamped to zero when
    negative. Zero delay transitions synchronously without creating a timer. */
let init: (~open_delay_ms: int, ~close_delay_ms: int, unit) => model;
/** Current intent state, not necessarily the consumer's visibility state. */
let is_open: model => bool;
/** Timestamp of the most recent pointer/environment close, for Group.record_close.
    None before any such close. Programmatic setters do not modify it. */
let last_close_time: model => option(float);
/** Process intent against current state. Pass the current disabled flag on
    every call, even for timer messages: disabled clears pending/open state
    without emitting intents. When changing disabled in the parent, also call
    [close_now] immediately. Optional group is read, never stored or mutated.
    Moving between trigger/content cancels the shared leave timer. Duplicate
    events do not restart delays; stale timer generations cannot transition.
    Commands are currently none; lifecycle-owned tasks run in subscriptions. */
let update:
  (~disabled: bool=?, ~group: Group.t=?, msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Forward while mounted. [key] must be unique per instance (for example
    the consumer's id). Leaving the window starts the close delay; hiding the
    document closes immediately. */
let subscriptions: (~key: string, model) => Tea_core.Sub.t(msg);
/** Open immediately, invalidate pending timers, emit no out messages.
    The consumer decides whether keyboard focus should keep visibility open. */
let open_now: model => (model, Tea_core.Cmd.t(msg));
/** Close immediately and clear both hover regions/pending timers, emitting no
    out messages. Commit the model and reconcile subscriptions before unmount.
    A fresh pointerenter may start a new intent; obsolete callbacks cannot. */
let close_now: model => (model, Tea_core.Cmd.t(msg));
/** Trigger attributes mapped to the parent message type. Apply exactly once;
    add your own id, native disabled/ARIA, semantics and keyboard handlers.
    disabled is a styling flag here; update is the authoritative guard. */
let trigger_attrs:
  (~model: model, ~disabled: bool=?, ~to_parent: msg => 'parent, unit) =>
  list(Tea_html.Html.attr('parent));
/** Content attributes. May be applied to portalled content; no containment or
    DOM adjacency requirement. Consumer must hide/unmount closed content. */
let content_attrs:
  (~model: model, ~disabled: bool=?, ~to_parent: msg => 'parent, unit) =>
  list(Tea_html.Html.attr('parent));
/** Message-typed parts for conventional callback rendering. */
type parts('parent) = {
  /** Apply to the hover target. */
  trigger_attrs: list(Tea_html.Html.attr('parent)),
  /** Apply to its floating content. */
  content_attrs: list(Tea_html.Html.attr('parent)),
  /** Current hover intent; consumer chooses how to combine it with focus. */
  is_open: bool,
};
/** Render through the parts callback without adding a wrapper. */
let view:
  (
    ~model: model,
    ~disabled: bool=?,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
