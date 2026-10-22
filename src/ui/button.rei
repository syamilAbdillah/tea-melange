/** Stateless accessible button and toggle-button helper.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/button/

    Keyboard: Tab/Shift+Tab use native focus navigation. Enter activates on
    keydown. Space prevents scrolling on keydown and activates on keyup.
    Native buttons retain the browser's keyboard behavior. Disabled buttons
    never activate. Tab, browser shortcuts and IME events are not intercepted.

    Data attributes: data-disabled, data-pressed (presence flags).
    Owns click, keydown and keyup; use hooks instead of duplicate event facts.
    Do not place interactive descendants inside a button. */

/** Native form behavior; defaults to [Button]. */
type button_type =
  | Button
  | Submit
  | Reset;
/** A message-typed attribute bundle for custom rendering. */
type parts('msg) = {
  /** Apply to a native button by default; use [native=false] on a neutral
      non-button element. Includes id, semantics, state and event handlers. */
  root: list(Tea_html.Html.attr('msg)),
};
/** Build attributes without adding a DOM wrapper. Parent owns pressed/disabled.
    [focusable_when_disabled] defaults to false. When true, a disabled native
    button uses aria-disabled instead of disabled so it remains focusable.
    [pressed] absent means an ordinary button; present emits aria-pressed.
    Hooks receive unhandled, unmodified keys only and do not prevent default.
    Required Enter/Space behavior takes precedence over hooks. */
let parts:
  (
    ~id: string,
    ~on_press: 'msg,
    ~native: bool=?,
    ~button_type: button_type=?,
    ~disabled: bool=?,
    ~focusable_when_disabled: bool=?,
    ~pressed: bool=?,
    ~on_key_down: string => option('msg)=?,
    ~on_key_up: string => option('msg)=?,
    unit
  ) =>
  parts('msg);
/** Render through the conventional parts callback. Apply [root] to a native
    button by default; set [native=false] for a neutral element. Owns the same
    events as [parts]. The final unit makes optional arguments erasable. */
let view:
  (
    ~id: string,
    ~on_press: 'msg,
    ~native: bool=?,
    ~button_type: button_type=?,
    ~disabled: bool=?,
    ~focusable_when_disabled: bool=?,
    ~pressed: bool=?,
    ~on_key_down: string => option('msg)=?,
    ~on_key_up: string => option('msg)=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
/** Render a native button by default ([as_="button"]). Any other tag receives
    role=button, tabindex and the APG keyboard model. Use only neutral elements
    (e.g. div/span, not inputs or anchors with href). [attrs] appends caller
    styling/nonconflicting attributes; do not override owned events/semantics.
    [button_type] affects native buttons only. */
let button:
  (
    ~id: string,
    ~on_press: 'msg,
    ~as_: string=?,
    ~button_type: button_type=?,
    ~disabled: bool=?,
    ~focusable_when_disabled: bool=?,
    ~pressed: bool=?,
    ~attrs: list(Tea_html.Html.attr('msg))=?,
    ~on_key_down: string => option('msg)=?,
    ~on_key_up: string => option('msg)=?,
    list(Tea_html.Html.t('msg))
  ) =>
  Tea_html.Html.t('msg);
