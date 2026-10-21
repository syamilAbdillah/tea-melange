/** Controlled native or custom checkbox.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/checkbox/
    Keyboard: Tab/Shift+Tab navigate. Space down prevents scrolling on custom
    controls; Space up toggles. Native Space activation generates a click.
    Enter, shortcuts and IME are not intercepted. Disabled controls cannot
    activate and are removed from tab order.
    Data: data-checked, data-indeterminate, data-disabled (presence).
    Owns click, and custom keydown/keyup. */
/** Render the hidden_input alongside the control for custom form participation. */
type parts('msg) = {
  /** Apply to input (default) or neutral div/span (native=false).
     Native click defaults are cancelled so the browser cannot clear a
     controlled indeterminate property before an accepted parent update. */
  control: list(Tea_html.Html.attr('msg)),
  /** Native: label element, outside the input, linked with for.
     Custom: span INSIDE the role=checkbox control; its click bubbles to the
     control and native pointer focus reaches the focusable ancestor.
     Do not use an external HTML label for a non-labelable div/span. */
  label: list(Tea_html.Html.attr('msg)),
  /** Optional description; hidden when has_description=false. */
  description: list(Tea_html.Html.attr('msg)),
  /** Custom only: type=hidden when name is supplied; disabled when unchecked
     or disabled. Submit checked values even when indeterminate, like HTML. */
  hidden_input: Tea_html.Html.t('msg),
};
/** Compute the parent's next checked value from its LATEST state, in update.
    Indeterminate activation goes to checked; then clear indeterminate.
    Never call this from a view event closure. */
let next_checked: (~checked: bool, ~indeterminate: bool=?, unit) => bool;
/** Build bundles. on_change is a TOGGLE INTENT message, not a precomputed
    boolean, so rapid events compose correctly. Parent uses next_checked and
    clears indeterminate in update. Defaults: native, enabled, determinate,
    no description/name, value="on". Rejecting an intent retains native mixed
    state: click cancellation restores checked and indeterminate immediately.
    Apply all bundles as documented; do not nest interactive descendants.
    No document listeners, effects, or subscriptions. */
let parts:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: 'msg,
    ~indeterminate: bool=?,
    ~native: bool=?,
    ~disabled: bool=?,
    ~has_description: bool=?,
    ~name: string=?,
    ~value: string=?,
    unit
  ) =>
  parts('msg);
/** Render through a parts callback without adding a wrapper. Same contract
    and defaults as parts. Native input; custom label goes INSIDE control. */
let view:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: 'msg,
    ~indeterminate: bool=?,
    ~native: bool=?,
    ~disabled: bool=?,
    ~has_description: bool=?,
    ~name: string=?,
    ~value: string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
