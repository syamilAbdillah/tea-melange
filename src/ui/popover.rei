/** Non-modal anchored disclosure, optionally named as a dialog.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/
    Non-modal dialog semantics: https://www.w3.org/TR/wai-aria-1.2/#dialog
    There is no dedicated APG popover pattern. Dialog means a labelled,
    non-modal dialog, not the APG modal-dialog focus trap.

    Keyboard table:
    - Enter / Space on the native trigger: toggle (native button timing).
    - Tab / Shift+Tab: natural DOM navigation, never prevented or trapped.
      Shift+Tab from the first panel control returns to the trigger, staying open.
    - Escape in trigger/panel: close and focus trigger.
    - Other keys, modified shortcuts and IME: unchanged.

    Data attributes: data-open; Animation's data-closed, data-enter,
    data-leave, data-transition; Anchor's data-placement and
    data-reference-hidden. All flags are presence attributes.
    Render the panel immediately after its trigger in DOM order, never portal
    it. Top-layer promotion does not change DOM order. Omit autofocus. */

/** Panel semantics: Dialog(label) requires a nonempty accessible name and adds
    aria-haspopup=dialog to the trigger; Region is named by the trigger; None
    adds no role. None is the default. Never adds aria-modal. */
type role =
  | Dialog(string)
  | Region
  | None;
/** Opening focus policy; defaults to None (retain current focus). */
type focus_on_open =
  | None
  | First_tabbable
  | Element(string);
/** Logical dismissal cause; a close-button activation is Programmatic. */
type reason =
  | Escape
  | Outside
  | Focus_out
  | Trigger
  | Programmatic;
/** User-intent notifications, emitted immediately, not at animation completion. */
type out_msg =
  | Opened
  | Closed(reason);
/** Immutable state containing the Animation and Anchor submodels. */
type model;
/** Opaque intent and asynchronous submodel messages. */
type msg;
/** Initially closed. id is a unique prefix. Defaults: no role/focus movement,
    close_on_focus_out=true, top_layer=true. Geometry defaults match Anchor
    except offset=8. group_triggers lists peer trigger ids that may receive focus
    without closing this member; parents enforce exclusivity on Opened. */
let init:
  (
    ~id: string,
    ~role: role=?,
    ~focus_on_open: focus_on_open=?,
    ~close_on_focus_out: bool=?,
    ~group_triggers: list(string)=?,
    ~placement: Anchor.placement=?,
    ~offset: float=?,
    ~flip: bool=?,
    ~shift_padding: option(float)=?,
    ~match_width: bool=?,
    ~dir: Anchor.direction=?,
    ~top_layer: bool=?,
    ~arrow_size: float=?,
    unit
  ) =>
  model;
/** Stable native trigger id: prefix-trigger. */
let trigger_id: model => string;
/** Stable floating id: prefix-panel. */
let panel_id: model => string;
/** Logical expanded state, false during Leaving. */
let is_open: model => bool;
/** Whether the floating element must remain mounted, including Leaving. */
let should_render: model => bool;
/** Current embedded animation phase. */
let phase: model => Animation.phase;
/** Latest Anchor geometry, including optional arrow offset. */
let position: model => option(Anchor.position);
/** Forward every intent/submodel message; emits user notifications only. */
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Required throughout the mounted model lifetime, including Leaving. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Open/reverse Leaving without an out message; idempotent. */
let open_: model => (model, Tea_core.Cmd.t(msg));
/** Close without an out message or moving focus; idempotent. Parent should
    choose a logical focus destination when programmatically removing controls. */
let close: model => (model, Tea_core.Cmd.t(msg));
/** Parent-message-typed bundles, no wrapper or styling dependency. */
type parts('parent) = {
  /** Native button attrs: id/type/click/keydown/keyup, expanded and controls.
      controls is present only while rendered. Do not override owned facts. */
  trigger: list(Tea_html.Html.attr('parent)),
  /** Floating div attrs: id/hidden/keydown, geometry, animation and semantics.
      hidden is true exactly in Animation.Closed; never append a second hidden. */
  panel: list(Tea_html.Html.attr('parent)),
  /** Optional native close button: id/type/click/keydown/keyup. Use at most once. */
  close_button: list(Tea_html.Html.attr('parent)),
  /** Logical expanded state. */
  open_: bool,
  /** Mount while true; alternatively keep mounted and let hidden hide Closed. */
  rendered: bool,
  /** Current geometry for arrow presentation. */
  position: option(Anchor.position),
};
/** Apply bundles once, rendering trigger immediately before panel. Owns the
    documented events; append only non-conflicting attrs. Keydown hooks receive
    unhandled plain non-Tab keys and do not prevent default. Native activation
    on the trigger and Escape dismissal take precedence. Panel hooks may see
    bubbled events from descendants; do not override input editing behavior. */
let view:
  (
    ~model: model,
    ~to_parent: msg => 'parent,
    ~on_trigger_key_down: string => option('parent)=?,
    ~on_panel_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
