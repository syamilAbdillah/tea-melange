/** Immutable notification queue with one {!Animation} per entry.
    There is no APG toast pattern; live-region guidance:
    https://www.w3.org/WAI/ARIA/apg/patterns/alert/ and action buttons:
    https://www.w3.org/WAI/ARIA/apg/patterns/button/ .

    Keyboard table:
    - F8 (configurable exact chord): focus the most recently pushed visible
      toast, or the empty region. Consumes only that chord; excludes IME/repeat.
    - Tab / Shift+Tab: native button navigation, never prevented. Tab-out after
      hotkey entry returns to the remembered element, after native navigation.
    - Plain Escape: dismiss the focused toast; after hotkey entry also return
      focus. On the empty hotkey-focused region, return without dismissing.
      Handled Escape stops propagation so an enclosing overlay stays open.
    - Enter / Space: native button activation. Other keys/chords/IME propagate.

    Data: data-index, data-urgency="normal|urgent", data-paused (presence), and
    Animation's data-enter, data-leave, data-transition, data-closed. Geometry:
    --tea-toast-index. Swipe is deliberately not implemented: no swipe handlers,
    states, variables or dismissal reasons are advertised.

    Always mount region, polite and assertive parts, even when empty, before
    inserting toasts. Render each toast as a keyed child of the live region
    matching its urgency. The initial subscription defers promotion across a
    paint so pushes in init cannot precede live-region creation. Parent owns
    content; store it until Dismissed, not until dismiss is requested.
    Urgent interrupts speech: reserve it for time-critical failures, not success.
    Actions must have an equivalent path outside the toast (described by
    alt_text), and must be persistent per WCAG 2.2.1. Supply has_actions=true:
    this forces duration=None. Finite durations are clamped to >=5000ms; long
    content may still require more time or persistence. */

/** Announcement priority. */
type urgency =
  | Normal
  | Urgent;
/** DOM order within each priority lane; admission is always FIFO. */
type order =
  | Oldest_first
  | Newest_first;
/** Why a toast finished leaving. Every reason has an implemented source. */
type reason =
  | Timeout
  | Close_button
  | Action
  | Escape
  | Programmatic;
/** Action is immediate; Dismissed is emitted exactly once, only after Left.
    Programmatic dismissals also emit Dismissed (this is lifecycle, not change). */
type out_msg =
  | Dismissed(string, reason)
  | Action(string);
/** Exact hotkey chord; modifier matching is intentional, unlike plain Escape. */
type hotkey = {
  /** DOM KeyboardEvent.key, e.g. F8. */
  key: string,
  /** Require Control. */
  ctrl: bool,
  /** Require Alt. */
  alt: bool,
  /** Require Meta. */
  meta: bool,
  /** Require Shift. */
  shift: bool,
};
/** Immutable keys, timing and lifecycle only; never content or mutable cells. */
type model;
/** Opaque intent/progress message. */
type msg;
/** Empty queue; max_visible defaults to 3 (clamped to at least 1), order to
    Oldest_first, label to Notifications and hotkey to unmodified F8. */
let init:
  (
    ~id: string,
    ~label: string=?,
    ~max_visible: int=?,
    ~order: order=?,
    ~hotkey: hotkey=?,
    unit
  ) =>
  model;
/** Visible keys in configured order, including leaving entries. */
let keys: model => list(string);
/** Waiting keys in admission order, excluding cancelled entries. */
let queued_keys: model => list(string);
/** Whether hover, focus or document visibility pauses all auto-dismiss timers. */
let paused: model => bool;
/** Add a content reference. Omitted key generates a collision-free key, visible
    through keys/queued_keys. Duration defaults to Some(5000); None persists.
    has_actions defaults false; true forces persistence. Duplicate key updates
    content and resets duration in place, reversing leave if necessary, without
    emitting Dismissed. Visible urgency/position stay unchanged to avoid moving
    a focused DOM subtree; queued urgency may change. Text changes in the same
    live region may be re-announced, but identical text is not forcibly replayed.
    Timers start only after Entered and resume remaining time after every pause.
    Map the returned command and reconcile subscriptions after calling. */
let push:
  (
    model,
    ~key: string=?,
    ~duration: option(int)=?,
    ~urgency: urgency,
    ~has_actions: bool=?,
    string
  ) =>
  (model, Tea_core.Cmd.t(msg));
/** Start a programmatic leave. Unknown/already leaving keys are no-ops.
    Queued cancellations finish through Left without rendering or taking a slot. */
let dismiss: (model, string) => (model, Tea_core.Cmd.t(msg));
/** Start leaving every visible and queued entry; does not promote cancelled ones. */
let dismiss_all: model => (model, Tea_core.Cmd.t(msg));
/** Process intents against current state; stale timer/animation tokens are ignored. */
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Required while the region is mounted, including empty and leaving states.
    Owns cancellable timers, visibility, hotkey, boundary focus listeners and
    Animation subscriptions. Stop/unmount cancels all live work. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Per-entry parent-typed parts. */
type toast_parts('parent) = {
  /** Stable toast key. */
  key: string,
  /** Parent-owned content lookup key supplied to push. */
  content_key: string,
  /** Priority lane; immutable while visible. */
  urgency,
  /** Current per-toast lifecycle phase. */
  phase: Animation.phase,
  /** Zero-based visible stacking order across both lanes. */
  index: int,
  /** No timeout; action parts are only enabled for persistent entries. */
  persistent: bool,
  /** Group with title/description idrefs, tabindex=-1, animation/stacking data.
      Owns focusin and keydown (plain Escape only). */
  root: list(Tea_html.Html.attr('parent)),
  /** Required title element id. */
  title: list(Tea_html.Html.attr('parent)),
  /** Required description element id (may have empty text). */
  description: list(Tea_html.Html.attr('parent)),
  /** Native button, labelled Dismiss notification, disabled during leave.
      Owns click, keydown and keyup via Button. */
  close_button: list(Tea_html.Html.attr('parent)),
  /** Native action button attrs. key uniquely names this action within the toast.
      alt_text describes the equivalent route outside the toast, not its label;
      blank alt_text or a finite-duration toast disables the action. Activation
      emits Action(toast_key) then leaves with reason Action. Owns Button events. */
  action_attrs:
    (~key: string, ~alt_text: string, unit) =>
    list(Tea_html.Html.attr('parent)),
};
/** Persistent region plus priority lanes and derived toast parts. */
type parts('parent) = {
  /** role=region, labelled, tabindex=-1; owns pointerenter/leave/cancel and keydown
      (Tab bookkeeping, Escape only on the hotkey-focused region itself).
      Unhandled plain keys bubble here to view's on_key_down hook, including
      keys from toast/button parts. */
  region: list(Tea_html.Html.attr('parent)),
  /** Persistent role=status live=polite lane; atomic=false, additions/text only. */
  polite: list(Tea_html.Html.attr('parent)),
  /** Persistent role=alert live=assertive lane for Urgent only. */
  assertive: list(Tea_html.Html.attr('parent)),
  /** Visible entries, never queued entries. Key children by key inside each lane. */
  toasts: list(toast_parts('parent)),
  /** Current aggregate pause state. */
  paused: bool,
  /** Waiting count excluding cancellations. */
  queued: int,
};
/** Render no wrapper; parent applies every required part and renders its own
    title/description/actions. Do not override owned handlers or hide a leaving
    toast. Append styling/non-conflicting attributes freely. on_key_down receives
    otherwise unhandled plain keys bubbling through the region, not Tab/IME or
    modified shortcuts; hooks neither prevent default nor stop propagation. */
let view:
  (
    ~model: model,
    ~to_parent: msg => 'parent,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
