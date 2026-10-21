/** Reversible, subscription-owned enter/leave animation lifecycle.
    APG patterns: https://www.w3.org/WAI/ARIA/apg/patterns/
    Animation itself is not an APG widget and adds no role or ARIA state.

    Keyboard table: every key, including Tab/Shift+Tab, shortcuts and IME,
    passes through unchanged. The embedding overlay owns its keyboard model.

    Presence data attributes: data-closed, data-enter, data-leave,
    data-transition. hidden is true only in Closed. No CSS is necessary.
    Forward [subscriptions] throughout Entering and Leaving, and retain the
    same model across reversals. Apply [attrs] to the element named by [id]. */

/** Read-only lifecycle state. Leaving remains mounted and is not hidden. */
type phase =
  | Closed
  | Entering
  | Open
  | Leaving;
/** Abstract immutable submodel; ids address elements, not derived prefixes. */
type model;
/** Opaque, generation-tagged asynchronous progress. Forward to [update]. */
type msg;
/** Exactly one event for a completed, non-superseded transition. Initial open
    without appear emits nothing. Repeated show/hide emits nothing. */
type out_msg =
  | Entered
  | Left;
/** Initially Closed or Open. [appear=true] starts initially open models in
    Entering; the subscription starts automatically after the initial mount.
    [appear] defaults to false and does not affect initially closed models.
    [id] is the document-unique id of the element whose subtree is awaited. */
let init: (~id: string, ~open_: bool, ~appear: bool=?, unit) => model;
/** Current lifecycle phase, without exposing writable state. */
let phase: model => phase;
/** Enter or reverse a leave. Open/Entering are idempotent no-ops. Effects are
    subscription-owned; return/map the command normally (currently Cmd.none). */
let show: model => (model, Tea_core.Cmd.t(msg));
/** Leave or reverse an enter. Closed/Leaving are idempotent no-ops. Keep the
    element mounted until Left, or until [should_render] becomes false. */
let hide: model => (model, Tea_core.Cmd.t(msg));
/** Progress only the matching generation; stale/duplicate messages are no-ops.
    Enter waits across a paint, removes data-closed, then snapshots animations
    after that render. Leave snapshots after its first render. Missing elements
    count as completed. Finished/cancelled subtree animations are all awaited.
    Inherits Browser_dom's 5000 ms watchdog for paused/infinite animations. */
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Required while mounted, including during Leaving. Owns frame handoff and
    animation waits, including initial appear. Identity includes id, generation
    and stage. Stopping suppresses every pending callback; no DOM listeners are
    installed. Stopping also cancels an in-flight animation wait, releasing its
    observer and watchdog immediately. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Parent-message-polymorphic attributes; owns only the four data flags and
    hidden, no events, id, styles or ARIA. Set the element id to [init]'s id.
    hidden=false during Entering/Open/Leaving clears a previously kept mount's
    hidden property. Never override hidden or the owned flags. */
let attrs: model => list(Tea_html.Html.attr('msg));
/** True in Entering/Open/Leaving, even at the transparent enter/leave endpoint.
    Describes lifecycle visibility, not computed CSS or interaction policy. */
let is_visible: model => bool;
/** Same predicate as [is_visible]. False only in Closed; safe to unmount then.
    Keeping it mounted is also safe because [attrs] sets hidden in Closed. */
let should_render: model => bool;
