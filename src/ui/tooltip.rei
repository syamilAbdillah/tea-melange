/** Accessible, hoverable, persistent tooltip submodel.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/
    WCAG: https://www.w3.org/WAI/WCAG22/Understanding/content-on-hover-or-focus.html

    Keyboard table:
    - Tab / Shift+Tab: native traversal, never prevented; keyboard focus opens
      immediately, blur closes. Pointer-originated focus does not open.
    - Escape while open: close, prevent default and stop propagation; retain
      focus. Hidden Escape propagates to enclosing overlays.
    - Enter / Space / arrows / other keys: native behavior or optional hook.
    - Ctrl/Alt/Meta chords and IME: unchanged.

    Presence data: data-open, data-closed, data-disabled, data-enter,
    data-leave, data-transition, data-reference-hidden. data-placement comes
    from Anchor. Geometry styles only; no CSS required for correctness.
    Always render the tooltip, even hidden, for Label mode accessible naming.
    Tooltip content must not contain focusable/interactive controls. */

/** Description adds aria-describedby (default); Label adds aria-labelledby
    for icon-only controls. Do not override the selected idref. */
type mode =
  | Description
  | Label;
/** Immutable opaque submodel. */
type model;
/** Opaque intent/progress messages; forward through update. */
type msg;
/** User-originated requested transitions, emitted once each and only on an
    actual change of requested state. Closed carries monotonic milliseconds in
    the PointerEvent.timeStamp origin. Both exist for the parent's warm-up
    group, keyed by the parent's own member key (see [track]). Programmatic
    [open_], [close], [close_immediately] and [set_disabled] emit nothing. */
type out_msg =
  | Opened
  | Closed(float);
/** Initially closed. id is a document-unique prefix; parts are id-trigger and
    id-tooltip. Defaults: Description, enabled, 600ms opening, 150ms closing,
    top-center, offset 8px, flip, padding Some(8px), Ltr, top_layer=true, no
    arrow. Delays clamp at zero. disabled disables the tooltip, not the control.
    Global pointerdown marks pointer modality before focus; an unmodified,
    non-composing key restores keyboard modality. Initial modality is keyboard
    (including programmatic initial focus). Touch never opens via hover/focus. */
let init:
  (
    ~id: string,
    ~mode: mode=?,
    ~disabled: bool=?,
    ~open_delay_ms: int=?,
    ~close_delay_ms: int=?,
    ~placement: Anchor.placement=?,
    ~offset: float=?,
    ~flip: bool=?,
    ~shift_padding: option(float)=?,
    ~dir: Anchor.direction=?,
    ~top_layer: bool=?,
    ~arrow_size: float=?,
    unit
  ) =>
  model;
/** Requested open state, excluding Leaving. */
let is_open: model => bool;
/** Lifecycle visibility including Leaving. */
let is_visible: model => bool;
/** Animation phase; hidden is true exactly in Closed. */
let phase: model => Animation.phase;
/** Latest Anchor measurement, including arrow offset. */
let position: model => option(Anchor.position);
/** Whether tooltip interactions are disabled. */
let disabled: model => bool;
/** Open immediately unless disabled; reverse Leaving. No focus movement.
    Emits nothing: if grouped, record_open with the parent's member key when
    actually open and immediately close/forget every other visible member. */
let open_: model => (model, Tea_core.Cmd.t(msg));
/** Clear hover timers/focus persistence and hide. Keep subscriptions and mounted
    tooltip through Leaving; Anchor stops only after Animation.Left. */
let close: model => (model, Tea_core.Cmd.t(msg));
/** Close with no leave: the tooltip is hidden synchronously (phase Closed, no
    Leaving frame), Anchor stops, hover timers and focus persistence clear. For
    a group, call it on every other visible member when one opens so only one
    tooltip is ever shown (an earlier member may still be Leaving, so test
    [is_visible], not [is_open]). Emits nothing; also call
    [Hover_intent.Group.forget]. Commit the model before unmounting. */
let close_immediately: model => (model, Tea_core.Cmd.t(msg));
/** Fold a notification into the parent's warm-up group for the member [key]:
    Opened is [Group.record_open ~key], Closed(t) is
    [Group.record_close ~key ~time_stamp:t] (which removes the member). Pass the
    result to the next member's [update]. The stable key is unique within the
    group; it need not equal the tooltip id. An open member warms the group
    during its close delay; after the last close, the skip window applies.
    This helper does not enforce exclusivity: on Opened, the parent must also
    [close_immediately] and forget every other visible member in the same
    update. Members closed programmatically, disabled or unmounted must be
    removed with [Hover_intent.Group.forget ~key], which records no timestamp. */
let track:
  (~key: string, out_msg, Hover_intent.Group.t) => Hover_intent.Group.t;
/** Disable atomically, cancelling intent and hiding; enabling does not open. */
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));
/** Resolve intents using the newest state. Optional immutable warm-up group is
    passed, never stored; fold the notifications into it with [track]. */
let update:
  (~group: Hover_intent.Group.t=?, msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Forward while mounted. An open tooltip also claims plain Escape outside
    its trigger, so hover-only users can dismiss it without moving focus
    (WCAG 1.4.13). */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Wrapper-free, parent-message-typed bundles. */
type parts('parent) = {
  /** Owns id, selected aria idref, pointerenter/leave/cancel, focus, blur,
      keydown and data flags. Native semantics/activation belong to the caller.
      Use a focusable span around a disabled button (pointer-events:none on
      the disabled child) to retain pointer and keyboard help. */
  trigger: list(Tea_html.Html.attr('parent)),
  /** Always render. Owns id, role=tooltip, hidden, Animation/Anchor attrs,
      pointerenter/leave/cancel and data flags. Never override hidden. */
  tooltip: list(Tea_html.Html.attr('parent)),
  /** Optional decorative part: aria-hidden, data-placement and geometry-only
      --tea-arrow-offset cross-axis leading edge. Set arrow_size at init; use
      CSS to attach the arrow to the resolved side. */
  arrow: list(Tea_html.Html.attr('parent)),
  /** Requested open state. */
  open_: bool,
  /** Lifecycle visibility; not permission to unmount Label-mode text. */
  visible: bool,
  /** Latest geometry. */
  position: option(Anchor.position),
};
/** Render without a wrapper. Hook receives unmodified, non-composing keys
    except Tab/Escape and never prevents defaults. Do not stack key handlers.
    Keep tooltip text present while hidden, particularly in Label mode. */
let view:
  (
    ~model: model,
    ~to_parent: msg => 'parent,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
