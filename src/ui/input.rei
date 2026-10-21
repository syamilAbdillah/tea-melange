/** Controlled native input.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/ (native text entry has no
    separate APG pattern). Native HTML semantics are retained.
    Keyboard: Tab/Shift+Tab navigate; text editing, arrows, Home/End,
    selection, shortcuts and IME remain browser-owned.
    Enter retains native implicit form submission.
    Data: data-invalid, data-disabled, data-readonly (presence).
    Owns input, change and blur. No key events are intercepted. */
/** Apply each bundle to the matching native element. */
type parts('msg) = {
  /** Visible label; includes for=control id. */
  label: list(Tea_html.Html.attr('msg)),
  /** Native input; controlled value, validation and events. */
  control: list(Tea_html.Html.attr('msg)),
  /** Description; hidden unless has_description. */
  description: list(Tea_html.Html.attr('msg)),
  /** Error; hidden unless has_error. */
  error: list(Tea_html.Html.attr('msg)),
};
/** Build bundles. Flags default false; declare has_description/has_error only
    when rendering those parts, so idrefs never dangle. Error is described
    whenever present and is aria-errormessage only when invalid. Disabled and
    readonly controls reject input/change messages; blur is still reported.
    Parent owns value and validation; events deliver actual DOM values.
    Type defaults to text; autocomplete/inputmode/name pass through verbatim. */
let parts:
  (
    ~id: string,
    ~value: string,
    ~on_input: string => 'msg,
    ~on_change: string => 'msg=?,
    ~on_blur: 'msg=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~name: string=?,
    ~type_: string=?,
    ~autocomplete: string=?,
    ~inputmode: string=?,
    unit
  ) =>
  parts('msg);
/** Render through a parts callback without adding a wrapper. Same options,
    defaults, ownership and presence contract as parts. */
let view:
  (
    ~id: string,
    ~value: string,
    ~on_input: string => 'msg,
    ~on_change: string => 'msg=?,
    ~on_blur: 'msg=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~name: string=?,
    ~type_: string=?,
    ~autocomplete: string=?,
    ~inputmode: string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
