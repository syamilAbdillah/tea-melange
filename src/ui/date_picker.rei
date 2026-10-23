/** Editable civil date with an anchored, focus-modal Calendar dialog.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/examples/datepicker-dialog/

    Keyboard table:
    - Enter/Space on Choose date: native button activation opens the grid.
    - Enter in the text input: commit without submitting; blur also commits.
    - Tab/Shift+Tab in the dialog: cycle rendered controls; never prevented.
    - Escape: close, prevent default, stop propagation, return to Choose date.
    - Calendar arrows, Home/End, PageUp/Down and Shift+PageUp/Down: Calendar.
    - Enter/Space on a date: select; close unless confirm mode is enabled.
    - Enter/Space on OK/Cancel: commit/cancel and return to Choose date.
    - Modified shortcuts, IME and unhandled keys: unchanged.

    Data: data-open, data-disabled, data-readonly, data-invalid; inherited
    Calendar, Animation and Anchor presence flags and data-placement.
    No CSS is required for correctness. Render boundaries as documented.
    Popup pointer dismissal does not return focus or steal the destination.
    Read-only permits browsing but never changes the committed date.
    Background pointer interaction dismisses; keyboard focus is contained.
    See docs/ui/date_picker.md for the distinction from native inert modality. */

/** Calendar's immutable civil date (years 1 through 9999). */
type date = Calendar.date;
/** Parse failure versus inclusive-bound or unavailable-date failure. */
type invalid_reason =
  | Syntax
  | Before_min
  | After_max
  | Unavailable;
/** Logical dismissal source. Cancel includes toggling an expanded trigger. */
type reason =
  | Escape
  | Outside
  | Selection
  | Confirm
  | Cancel
  | Programmatic;
/** User-intent notifications, immediate rather than animation completions.
    Invalid retains both the raw text and the previous committed value. */
type out_msg =
  | Changed(option(date))
  | Invalid(string, invalid_reason)
  | Opened
  | Closed(reason);
/** Localizable picker labels; Calendar has its own separate labels record. */
type labels = {
  /** Accessible name when no date is committed; also use as visible text. */
  choose: string,
  /** Accessible name including the chosen civil date and visible button text. */
  choose_selected: date => string,
  /** Visible OK label. */
  confirm: string,
  /** Visible Cancel label. */
  cancel: string,
  /** Accessible name for the optional generic close button. */
  close: string,
  /** Polite keyboard-help announcement after initial grid focus. */
  instructions: string,
  /** Default visible error text for each validation reason. */
  error: invalid_reason => string,
};
/** English defaults, including Choose date, OK, Cancel and keyboard help.
    Selected name reads e.g. Choose date, selected date is March 5, 2026. */
let english: labels;
/** Immutable state embedding Calendar and Popover (and hence Anchor/Animation). */
type model;
/** Opaque user-intent and asynchronous progress messages. */
type msg;
/** Initially closed. focus is the parent-supplied today/fallback (no clock).
    Selected defaults absent; format/parse default to strict ISO YYYY-MM-DD.
    confirm=false commits date selection immediately; true stages until OK.
    Calendar defaults: Sunday-first, Ltr, six weeks, blank adjacent-month cells.
    Placement defaults Bottom/Start, top_layer=true. top_layer=false exists for
    environments without native popovers; fixed-position fallback limitations
    apply. Retain the same instance across openings and reversals. */
let init:
  (
    ~id: string,
    ~focus: date,
    ~selected: date=?,
    ~confirm: bool=?,
    ~format: date => string=?,
    ~parse: string => option(date)=?,
    ~first_day_of_week: int=?,
    ~dir: Calendar.direction=?,
    ~fixed_weeks: bool=?,
    ~show_outside_month: bool=?,
    ~placement: Anchor.placement=?,
    ~top_layer: bool=?,
    unit
  ) =>
  model;
/** Committed form/domain value, unaffected by invalid text or staged selection. */
let value: model => option(date);
/** Controlled editable text, including uncommitted and invalid text. */
let text: model => string;
/** Most recent commit validation failure; cleared by a successful commit/setter. */
let invalid_reason: model => option(invalid_reason);
/** Logical expanded state (false during Leaving). */
let is_open: model => bool;
/** Keep the popup mounted throughout Entering, Open and Leaving. */
let should_render: model => bool;
/** Embedded Animation lifecycle. */
let phase: model => Animation.phase;
/** Embedded Calendar roving date, including staged browsing. */
let focus_date: model => date;
/** Silent domain setter. Also formats text, clears validation, and replaces
    staged selection. Does not move focus, close the popup or validate bounds. */
let set_value: (option(date), model) => model;
/** Silent idempotent opening; ignores disabled=true. Valid typed text supplies
    the focus date; otherwise committed value, then init's focus. Pass the same
    current constraints as update/view. Does not commit unblurred text.
    Captures the opening focus before waiting for positioning. Intervening
    focus movement cancels initial focus and emits Closed(Outside); the modal
    focus boundary and instructions activate only after successful initial focus. */
let open_:
  (~disabled: bool=?, ~constraints: Calendar.constraints=?, model) =>
  (model, Tea_core.Cmd.t(msg));
/** Silent close without focus movement; discards staged selection. Idempotent
    logically (no notification); cancels pending picker focus effects. */
let close: model => (model, Tea_core.Cmd.t(msg));
/** Evaluate intents with current constraints and modes, identical to view's.
    Disabled rejects editing/opening/grid interaction; read-only permits browsing.
    Empty text clears; invalid text keeps previous value. Enter/blur deduplicate
    the same edit. Calendar selection in confirm mode only changes staging.
    Constraints are never stored; they also revalidate OK at activation time. */
let update:
  (
    ~constraints: Calendar.constraints=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    msg,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Forward while mounted, including Leaving. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Attribute bundles in the parent's message type, plus derived presentation. */
type parts('parent) = {
  /** Input-group container around input, trigger and dialog. Owns its anchor id. */
  group: list(Tea_html.Html.attr('parent)),
  /** Native label, for=input id. */
  label: list(Tea_html.Html.attr('parent)),
  /** Native text input; owns input/change/blur/keydown. No name: use hidden_input. */
  input: list(Tea_html.Html.attr('parent)),
  /** Native button; owns click/keydown/keyup. Render immediately before dialog. */
  trigger: list(Tea_html.Html.attr('parent)),
  /** Optional description, hidden unless has_description=true. */
  description: list(Tea_html.Html.attr('parent)),
  /** Error part, hidden unless invalid; render always to resolve error idrefs. */
  error: list(Tea_html.Html.attr('parent)),
  /** Localized error message, empty when valid. */
  error_text: string,
  /** Native hidden input with optional name and committed ISO value. */
  hidden_input: list(Tea_html.Html.attr('parent)),
  /** Floating div, role=dialog, aria-modal, labelled by Calendar heading.
      Owns keydown and inherited positioning/animation attributes. */
  dialog: list(Tea_html.Html.attr('parent)),
  /** Empty span as FIRST child of dialog, before all controls. Focus sentinel;
      do not hide, omit, style display:none, or override its tabindex/id. */
  start_boundary: list(Tea_html.Html.attr('parent)),
  /** Empty span as LAST child of dialog, after all controls; same contract. */
  end_boundary: list(Tea_html.Html.attr('parent)),
  /** Optional live keyboard-help part. Keep mounted; may be visually hidden,
      never hidden/aria-hidden. Has no required idref. */
  instructions: list(Tea_html.Html.attr('parent)),
  /** Help after initial focus, empty otherwise; render as instructions content. */
  instructions_text: string,
  /** Render heading, navigation and grid using Calendar's anatomy unchanged. */
  calendar: Calendar.parts('parent),
  /** Optional native OK button, owns click/keydown/keyup; render iff confirm. */
  confirm_button: list(Tea_html.Html.attr('parent)),
  /** Native Cancel button (recommended also in immediate selection mode). */
  cancel_button: list(Tea_html.Html.attr('parent)),
  /** Optional generic native close button, owns click/keydown/keyup, labelled
      with labels.close. Emits Closed(Programmatic), discarding staging. Silent
      close(model) instead deliberately emits nothing and does not move focus. */
  close_button: list(Tea_html.Html.attr('parent)),
  /** Whether selection requires explicit OK. */
  confirm: bool,
  /** Logical expanded state. */
  open_: bool,
  /** Required popup mount lifetime. */
  rendered: bool,
  /** Current committed value. */
  value: option(date),
  /** Current editable text. */
  text: string,
};
/** Wrapper-free rendering. Default modes/description/required are false;
    labels default english; constraints default Calendar.unconstrained.
    Apply bundles once, appending only nonconflicting attributes. Render the
    label, input, trigger, dialog and Calendar heading/grid. Render both empty
    span boundaries inside the dialog and preserve DOM order (no portals or
    positive tabindex). Optional year buttons may be omitted. Render the error
    part always, description iff declared. Keep stable Calendar heading/table
    positions. Apply group to the container around input, trigger and dialog;
    the group anchors the popup in the top layer, not the trigger.
    The hidden input (when named) submits only the committed ISO value, even
    with custom text formatting. Disabled omits it from successful controls.
    Use labels.choose/confirm/cancel for visible button text (matching ARIA). */
let view:
  (
    ~model: model,
    ~constraints: Calendar.constraints=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~required: bool=?,
    ~has_description: bool=?,
    ~name: string=?,
    ~today: date=?,
    ~labels: labels=?,
    ~calendar_labels: Calendar.labels=?,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
