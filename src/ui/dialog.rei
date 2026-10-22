/** Native modal dialog and alertdialog, with reversible enter/leave lifecycle.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/
    and https://www.w3.org/WAI/ARIA/apg/patterns/alertdialog/

    Keyboard table:
    - Tab / Shift+Tab: native modal focus containment; never prevented.
    - Escape while visible (including Leaving): request dismissal of the
      focused/top dialog; prevent native close and stop propagation.
      Closed, mounted dialogs leave Escape to enclosing handlers.
    - Enter / Space: native buttons and form submission; never synthesized.
    - Other keys: unchanged, or the optional unmodified-key hook.
    - Ctrl/Alt/Meta chords, IME: unchanged.

    Presence data attributes: data-closed, data-enter, data-leave,
    data-transition, inherited from Animation. The dialog stays open in the
    top layer through Leaving. No CSS is required. Native dialog support is
    required for modality; there is deliberately no nonmodal variant.
    Always forward subscriptions, and keep root mounted through Leaving. */

/** Modal uses optional initial_focus, first tabbable, then root. Alertdialog
    REQUIRES the id of a rendered, enabled least-destructive action, normally
    Cancel. The alert target takes precedence over initial_focus. */
type kind =
  | Modal
  | Alertdialog(string);
/** Title references the title part (render it). Label omits aria-labelledby
    and uses a nonempty accessible label instead; the title part is optional. */
type name =
  | Title
  | Label(string);
/** Localizable accessible labels. */
type labels = {
  /** Accessible name of the close button. */
  close: string,
};
/** English labels: close="Close". */
let default_labels: labels;
/** User intent source. Native includes intercepted method=dialog submission
    and external imperative/native close events; inspect return_value. */
type reason =
  | Escape
  | Backdrop
  | Close_button
  | Native;
/** Lifecycle and intent notifications. Opened follows native showModal;
    Closed follows completed leave. Repeated dismissals during Leaving emit
    nothing. Close_requested precedes automatic closing, or asks the parent
    when dismissible=false. An ignored guarded request leaves the dialog open. */
type out_msg =
  | Opened
  | Close_requested(reason)
  | Closed;
/** Immutable opaque submodel. */
type model;
/** Opaque intent/progress messages. */
type msg;
/** Initially closed. id is a document-unique prefix; root is id-dialog,
    with id-title, id-description and id-close for the other parts.
    dismissible defaults true. Set false for unsaved-changes guards: every
    user dismissal only emits Close_requested; the parent must call close to
    accept. Programmatic close always bypasses the guard.
    close_on_backdrop defaults true for Modal, false for Alertdialog. */
let init:
  (
    ~id: string,
    ~kind: kind=?,
    ~initial_focus: string=?,
    ~dismissible: bool=?,
    ~close_on_backdrop: bool=?,
    unit
  ) =>
  model;
/** Current animation phase. */
let phase: model => Animation.phase;
/** True in Entering/Open (the requested open state), false during Leaving. */
let is_open: model => bool;
/** True in Entering/Open/Leaving. Keep mounted while true. */
let is_visible: model => bool;
/** Most recent native/form return value, reset on a fresh open. Method=dialog
    submits the submitter's value, or empty string without a submitter. */
let return_value: model => string;
/** Show or reverse Leaving. Idempotent in Entering/Open. Map the command. */
let open_: model => (model, Tea_core.Cmd.t(msg));
/** Leave; native close happens only after Animation.Left. Ignores the guard.
    Idempotent in Leaving/Closed. Map the command. */
let close: model => (model, Tea_core.Cmd.t(msg));
/** Process intent against the latest model. Returns notifications in order. */
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Required throughout the mounted lifetime, including Leaving. Composes
    Animation, native show/close, reference-counted scroll_lock and conditional
    restore_focus within root. Shutdown closes the native dialog and releases
    locks after render. Removed openers are not focused; intentional focus on
    another connected element is respected. Choose any fallback in the parent. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Attribute bundles in the parent message type and derived state. */
type parts('parent) = {
  /** Apply to a native dialog. Owns id, tabindex=-1, role (alertdialog only),
      aria-modal, name/description, returnValue, Animation flags/hidden, keydown,
      cancel, close, pointerdown/up/cancel, click, submit. Do not override open,
      hidden or those handlers. Root intercepts method=dialog forms for guards
      and leave animation; normal forms retain native behavior. */
  root: list(Tea_html.Html.attr('parent)),
  /** Title id; render when name=Title. */
  title: list(Tea_html.Html.attr('parent)),
  /** Description id; render iff has_description=true. Omit the description
      reference for long/structured content following APG. */
  description: list(Tea_html.Html.attr('parent)),
  /** Apply to a native button. Owns id, type, aria-label, click. */
  close_button: list(Tea_html.Html.attr('parent)),
  /** Current animation phase. */
  phase: Animation.phase,
  /** Requested open state. */
  is_open: bool,
  /** Lifecycle visibility including leave. */
  is_visible: bool,
  /** Current form/native return value. */
  return_value: string,
};
/** Render without a wrapper. name defaults Title; has_description defaults
    false; labels defaults default_labels. Use Label when no title is rendered.
    Backdrop dismissal requires down, up AND click on root, outside its bounding
    box; a drag originating in content never dismisses. Use padding:0 on root
    and put padded content in a child. Style ::backdrop using root data flags.
    While visible, Escape keydown and cancel are both handled: this prevents
    Chromium's repeated-Escape close watcher from bypassing a guard or an exit animation.
    on_key_down receives unhandled unmodified, non-composing keys (including
    Escape while closed); it returns an optional parent message without preventing default or stopping propagation.
    Tab is owned but never prevented. Only Escape stops propagation. */
let view:
  (
    ~model: model,
    ~name: name=?,
    ~has_description: bool=?,
    ~labels: labels=?,
    ~on_key_down: string => option('parent)=?,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
