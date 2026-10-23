/** Parent-owned disclosure.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/
    Accordion composition: https://www.w3.org/WAI/ARIA/apg/patterns/accordion/
    Keyboard: Tab/Shift+Tab use native navigation; Enter toggles on native
    click; Space toggles on native keyup click without scrolling.
    Other keys, shortcuts and IME retain native button behavior.
    Data attributes: data-open or data-closed on both button and panel.
    Owns click/keydown/keyup through Button; do not override these handlers.
    No focus effects, subscriptions, global listeners or CSS dependency. */

/** Parent-message-typed bundles. */
type parts('msg) = {
  /** Must be a native button. Includes type=button, expanded and controls. */
  button: list(Tea_html.Html.attr('msg)),
  /** Container id and hidden boolean property; render only when mounted. */
  panel: list(Tea_html.Html.attr('msg)),
  /** True when open or keep_mounted. Omit the panel when false. */
  mounted: bool,
};
/** Build bundles. open_ is parent state; on_toggle is an intent message
    (toggle against the latest model in update, not a precomputed boolean).
    keep_mounted defaults true: closed panel stays mounted with hidden=true.
    With false, omit the panel when mounted=false; aria-controls is then
    omitted to avoid a dangling idref. Reopening restores it.
    For accordion composition wrap each button, alone, in an appropriate
    native heading chosen by the application; parent enforces exclusivity.
    This supports zero-or-one-open accordions, not mandatory-one-open mode.
    Optional accordion arrow/Home/End navigation is not installed. */
let parts:
  (~id: string, ~open_: bool, ~on_toggle: 'msg, ~keep_mounted: bool=?, unit) =>
  parts('msg);
/** Render through caller's parts callback. Respect mounted as in [parts].
    Closing does not move focus; before programmatically closing a panel
    containing focus, the parent should focus its disclosure button. */
let view:
  (
    ~id: string,
    ~open_: bool,
    ~on_toggle: 'msg,
    ~keep_mounted: bool=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
