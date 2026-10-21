/** Native button switch; module Tea.Ui.Switch_ (Reason keyword suffix).
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/switch/
    Keyboard: Tab/Shift+Tab navigate; Enter activates on keydown; Space
    activates on keyup, without scrolling. These are NATIVE button behaviors:
    no synthetic key handler is installed, so Space never double-fires.
    Data: data-checked, data-disabled (presence). Owns click only. */
/** Apply to native elements; render hidden_input for form participation. */
type parts('msg) = {
  /** Apply ONLY to button. type=button, role=switch, aria-checked and name. */
  root: list(Tea_html.Html.attr('msg)),
  /** Apply to label. Native for=button activation toggles and focuses.
     Keep the label text constant when checked changes. */
  label: list(Tea_html.Html.attr('msg)),
  /** Optional description, hidden when has_description=false. */
  description: list(Tea_html.Html.attr('msg)),
  /** Hidden named input, successful only when checked and enabled. */
  hidden_input: Tea_html.Html.t('msg),
};
/** Build bundles. on_change is a parent TOGGLE INTENT, not a boolean computed
    from the last frame. Toggle the latest parent state in update.
    Defaults: unchecked is supplied explicitly, enabled, no description/name,
    value="on". No document listeners or subscriptions.
    Native button semantics retain browser shortcuts and IME handling. */
let parts:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: 'msg,
    ~disabled: bool=?,
    ~has_description: bool=?,
    ~name: string=?,
    ~value: string=?,
    unit
  ) =>
  parts('msg);
/** Render through a parts callback with no wrapper; same options as parts. */
let view:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: 'msg,
    ~disabled: bool=?,
    ~has_description: bool=?,
    ~name: string=?,
    ~value: string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
