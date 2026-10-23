/** Controlled native select, not a custom Listbox.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/combobox/ and
    https://www.w3.org/WAI/ARIA/apg/patterns/listbox/ are custom patterns;
    this helper deliberately retains platform-native select behavior.
    Keyboard: Tab/Shift+Tab navigate; arrows, Home/End, typeahead, popup
    activation/dismissal and multi-selection use the browser/OS conventions.
    No key handler is installed, including for Tab, modifiers or IME.
    Data: data-invalid, data-disabled (presence). Owns change. */
/** Option key is both logical identity and the native form value. */
type option_item = {
  /** Unique among all options, including grouped options; any string. */
  key: string,
  /** Visible option label. */
  label: string,
  /** Native disabled option. */
  disabled: bool,
};
/** Flat option or native optgroup. Groups cannot nest. */
type item =
  | /** An ungrouped native option. */
    Option(option_item)
  | /** A native optgroup with stable identity and shared disabled state. */
    Group({
      /** Stable group identity, unique among groups. */
      key: string,
      /** Native optgroup label. */
      label: string,
      /** Disables all group options. */
      disabled: bool,
      /** Options, whose keys are unique across the entire select. */
      items: array(option_item),
    });
/** Mode and controlled selection. Missing keys render unselected. */
type selection =
  | /** None selects a disabled placeholder, even when no visible placeholder. */
    Single(
      option(string),
    )
  | /** Native multiple selection; duplicate/missing keys are normalized away. */
    Multiple(
      array(string),
    );
/** Native parts. Use Html.keyed("select", control, options). */
type parts('msg) = {
  /** Apply to label; for points to control. */
  label: list(Tea_html.Html.attr('msg)),
  /** Apply to native select using Html.keyed, not Html.select. */
  control: list(Tea_html.Html.attr('msg)),
  /** Optional description, hidden unless declared present. */
  description: list(Tea_html.Html.attr('msg)),
  /** Optional error, hidden unless declared present. */
  error: list(Tea_html.Html.attr('msg)),
  /** Keyed option/optgroup children. Do not add/reorder by positional identity. */
  options: list((string, Tea_html.Html.t('msg))),
};
/** Keep existing keys, including disabled selections, and remove duplicate
    multiple keys. Use in parent update with the current items when collections
    can change between messages; view also normalizes for rendering.
    Programmatic normalization emits no change message. */
let normalize: (~items: array(item), selection) => selection;
/** Build bundles. on_change receives actual selected option keys (not indices).
    Parent owns selection and items. Presence flags default false and must
    match rendered parts. invalid describes the error when present and also
    sets aria-errormessage. Placeholder is single-mode only, disabled, with
    native empty form value. A real option with key="" is distinguished by
    its id but, like any empty-valued native option, fails native required.
    No selection never silently selects the first option.
    Disabled controls do not dispatch change. */
let parts:
  (
    ~id: string,
    ~items: array(item),
    ~value: selection,
    ~on_change: selection => 'msg,
    ~placeholder: string=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~name: string=?,
    unit
  ) =>
  parts('msg);
/** Render through the conventional callback without adding a wrapper. */
let view:
  (
    ~id: string,
    ~items: array(item),
    ~value: selection,
    ~on_change: selection => 'msg,
    ~placeholder: string=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~name: string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
