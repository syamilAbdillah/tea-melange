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
/** User-originated requested transitions. Closed carries monotonic milliseconds
    suitable for Hover_intent.Group.record_close. Setters emit nothing. */
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
/** Open immediately unless disabled; reverse Leaving. No focus movement. */
let open_: model => (model, Tea_core.Cmd.t(msg));
/** Clear hover timers/focus persistence and hide. Keep subscriptions and mounted
    tooltip through Leaving; Anchor stops only after Animation.Left. */
let close: model => (model, Tea_core.Cmd.t(msg));
/** Disable atomically, cancelling intent and hiding; enabling does not open. */
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));
/** Resolve intents using the newest state. Optional immutable warm-up group is
    passed, never stored; record Closed timestamps in the parent's group. */
let update:
  (~group: Hover_intent.Group.t=?, msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Forward for the entire mounted lifetime. Owns pointer/keyboard modality,
    hover timers, Animation and Anchor. Open hover-only tooltips also claim
    unmodified Escape outside the trigger so dismissal never requires moving
    focus. Shutdown/removal synchronously disposes listeners/timers/observers
    and top-layer promotion (shared scheduler callbacks become inert). */
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
  /** Lifecycle visibility; NOT permission to unmount Label-mode text. */
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
