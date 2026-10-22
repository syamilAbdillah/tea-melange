/** Stateful tabs, with separate focus and selection.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/tabs/

    Keyboard table:
    - Tab / Shift+Tab: enter selected tab, leave tablist normally.
    - Horizontal Left / Right: previous / next enabled tab, wrapping; swapped in RTL.
    - Vertical Up / Down: previous / next enabled tab, wrapping.
    - Home / End: first / last enabled tab in either orientation.
    - Automatic: navigation activates; Manual: Enter / Space activate focused tab.
    - Orthogonal arrows, Delete, browser chords and IME are untouched.
    - Tab is never prevented. No popup-menu or Delete-to-close behavior.

    Data: data-selected, data-active, data-disabled (presence flags);
    data-orientation=horizontal|vertical on tablist. Panels carry data-selected.
    Apply tabs to neutral elements, without interactive descendants. Tab parts
    own click, pointerdown, focus and keydown; tablist owns focusout. */

/** Horizontal writing direction. */
type direction =
  | Ltr
  | Rtl;
/** Which arrow axis the tablist claims. */
type orientation =
  | Horizontal
  | Vertical;
/** Automatic selects on focus/navigation; Manual requires Enter/Space or click. */
type activation =
  | Automatic
  | Manual;
/** Content policy: Selected_only mounts selected panel content only;
    Keep_mounted keeps every panel's content. Always render all panel shells
    with panel attributes so aria-controls references resolve. */
type panel_rendering =
  | Selected_only
  | Keep_mounted;
/** Parent-owned entry. Keys must be unique, stable strings. */
type item = {
  /** Identity, safely encoded in tab and panel ids. */
  key: string,
  /** Accessible tab name (also render visibly). */
  label: string,
  /** Skipped by navigation; cannot be selected. */
  disabled: bool,
};
/** Opaque state; collection is not retained. */
type model;
/** Opaque event intent, evaluated against the latest state. */
type msg;
/** Actual user selection change; never emitted by setters or repairs. */
type out_msg =
  | Changed(string);
/** One tab and its corresponding panel shell. */
type item_parts('parent) = {
  /** Original entry. */
  item,
  /** Tab role, name, id, aria-selected, aria-controls, tabindex and handlers. */
  tab: list(Tea_html.Html.attr('parent)),
  /** Panel role, id, aria-labelledby, hidden property and optional tabindex. */
  panel: list(Tea_html.Html.attr('parent)),
  /** Whether this panel is selected and visible. */
  selected: bool,
  /** Logical focus within the tablist. */
  focused: bool,
  /** Whether to render panel content under the chosen content policy. */
  render_panel: bool,
};
/** Parts callback payload, without an added wrapper. */
type parts('parent) = {
  /** Apply to tablist container only, not a wrapper around panels. */
  root: list(Tea_html.Html.attr('parent)),
  /** Visible label, referenced unless aria_label is provided. */
  label: list(Tea_html.Html.attr('parent)),
  /** Entries preserving order; render tabs and panels with stable keys. */
  items: array(item_parts('parent)),
};
/** Initialize, selecting requested enabled key or first enabled. Empty and
    all-disabled collections have no selection. Defaults: Ltr, Horizontal,
    Automatic. Choose Manual for panels with loading latency. */
let init:
  (
    ~id: string,
    ~items: array(item),
    ~selected: string=?,
    ~dir: direction=?,
    ~orientation: orientation=?,
    ~activation: activation=?,
    unit
  ) =>
  model;
/** Current selected key; reconcile when replacing items. */
let selected: model => option(string);
/** Current logical focus key, distinct from selection in Manual mode. */
let focused: model => option(string);
/** Change configuration without Changed output. Automatic mode reasserts
    selected tab focus; Manual retains logical focus. Refocus only if inside. */
let configure:
  (
    ~items: array(item),
    ~dir: direction=?,
    ~orientation: orientation=?,
    ~activation: activation=?,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg));
/** Repair removed/disabled selection to first enabled; repair missing focus to
    selection. Refocus only if focus was inside, including after keyed DOM reordering.
    If no enabled tab remains, park focus on the tablist. Call from parent updates that
    replace the collection and map the resulting command. No Changed output. */
let reconcile: (~items: array(item), model) => (model, Tea_core.Cmd.t(msg));
/** Select programmatically; invalid key falls back to first enabled. Moves
    focus only when already inside. No Changed output. */
let set_selected:
  (~items: array(item), string, model) => (model, Tea_core.Cmd.t(msg));
/** Repair and interpret intent; subscriptions move focus after rendering.
    Emits Changed only for actual user selection changes. */
let update:
  (~items: array(item), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Execute immutable generation-keyed focus requests after render. Always map
    this subscription into the parent: stopping cancels queued work. Temporary
    document focus listeners protect intervening user navigation and are removed
    on completion, removal or shutdown. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Render through parent-typed parts. By default root references visible label;
    aria_label instead supplies a direct accessible name. Keep_mounted is the
    default; unselected shells use hidden, never CSS. panel_focusable defaults
    to true (tabindex=0); return false only when the panel's first meaningful
    content is focusable. Manual navigation roves focus independently, restoring
    selected tab's entry tabindex when focus leaves the tablist. on_key_down
    receives unhandled plain non-Tab keys without preventing default. */
let view:
  (
    ~model: model,
    ~items: array(item),
    ~to_parent: msg => 'parent,
    ~aria_label: string=?,
    ~panel_rendering: panel_rendering=?,
    ~panel_focusable: string => bool=?,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
