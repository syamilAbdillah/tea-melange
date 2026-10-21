/** Native fieldset and legend grouping.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/checkbox/
    (grouping checkboxes; native fieldset semantics are used).
    Keyboard: no group key bindings. Tab/Shift+Tab follow the browser;
    disabled descendants are skipped except the first legend's descendants.
    Data: data-disabled, data-invalid (presence). Owns no DOM events. */
/** Bundles plus inherited disabled state for custom descendants. */
type parts('msg) = {
  /** Apply to fieldset. Native disabled propagation and description idref. */
  root: list(Tea_html.Html.attr('msg)),
  /** Apply to the first legend child; supplies the group's name. */
  legend: list(Tea_html.Html.attr('msg)),
  /** Description; render iff has_description, or leave its hidden property. */
  description: list(Tea_html.Html.attr('msg)),
  /** Pass this to custom descendants' disabled option; HTML cannot disable roles. */
  disabled: bool,
};
/** Build bundles. Defaults false. has_description must match actual presence.
    HTML exempts controls in the first legend from disabled propagation.
    Put controls that must be disabled outside the legend. */
let parts:
  (
    ~id: string,
    ~disabled: bool=?,
    ~invalid: bool=?,
    ~has_description: bool=?,
    unit
  ) =>
  parts('msg);
/** Render through the conventional callback, with no added wrapper. */
let view:
  (
    ~id: string,
    ~disabled: bool=?,
    ~invalid: bool=?,
    ~has_description: bool=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
