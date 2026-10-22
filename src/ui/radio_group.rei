/** Stateful, non-toolbar radio group with roving tabindex.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/radio/

    Keyboard table:
    - Tab / Shift+Tab: enter at checked radio (first enabled if unchecked), leave normally.
    - Down / Right: next enabled, wrap, focus and check; Left / Up: previous.
    - RTL swaps Left / Right. Both axes work regardless of orientation hint.
    - Space: check focused radio. Enter, Home and End are not radio shortcuts.
    - Read-only: arrows only move focus; Space does nothing.
    - Disabled: no navigation or changes; one enabled item remains tabbable.
    - Ctrl/Alt/Meta chords and IME are untouched. Tab is never prevented.

    Data: data-checked, data-active, data-disabled, data-readonly, data-invalid
    (presence flags), data-orientation=horizontal|vertical on root.
    Apply item bundles to neutral elements; no interactive descendants.
    Items own click, pointerdown, focus and keydown; root owns focusout. */

/** Horizontal writing direction. */
type direction =
  | Ltr
  | Rtl;
/** Styling hint only: radiogroup does not support aria-orientation. */
type orientation =
  | Horizontal
  | Vertical;
/** Parent-owned collection entry. Keys must be unique, stable strings. */
type item = {
  /** Identity; safely encoded in DOM ids. */
  key: string,
  /** Accessible name (also render visibly). */
  label: string,
  /** Skipped by navigation and never checked. */
  disabled: bool,
};
/** Opaque selection, focus and configuration; never stores the collection. */
type model;
/** Opaque event intent, evaluated against the latest model and collection. */
type msg;
/** User selection changed; repair and programmatic setters emit nothing. */
type out_msg =
  | Changed(string);
/** One radio's parent-message-typed attributes and derived state. */
type item_parts('parent) = {
  /** Original entry. */
  item,
  /** Role, id, aria-checked, tabindex and owned event handlers. */
  root: list(Tea_html.Html.attr('parent)),
  /** Whether selected after collection repair. */
  checked: bool,
  /** Logical focus within the group. */
  focused: bool,
};
/** Renderable parts; no wrapper is inserted. */
type parts('parent) = {
  /** Radiogroup container, named by label and optionally described. */
  root: list(Tea_html.Html.attr('parent)),
  /** Apply to the visible group label (e.g. a heading or span). */
  label: list(Tea_html.Html.attr('parent)),
  /** Apply to optional description; enable its idref with description=true. */
  description: list(Tea_html.Html.attr('parent)),
  /** One entry per supplied item, preserving order. Use keyed rendering. */
  items: array(item_parts('parent)),
  /** Render inside the form. Present only with name and a valid selection.
      Disabled groups produce a disabled hidden input. */
  hidden_input: option(Tea_html.Html.t('parent)),
};
/** Initialize and validate against items. Defaults: unchecked, enabled,
    writable, Ltr, Vertical. No effects and no collection retained. */
let init:
  (
    ~id: string,
    ~items: array(item),
    ~checked: string=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~dir: direction=?,
    ~orientation: orientation=?,
    unit
  ) =>
  model;
/** Current checked key, or None. Call reconcile when replacing items. */
let checked: model => option(string);
/** Current logical focus key (not a synchronous DOM read). */
let focused: model => option(string);
/** Change configuration without Changed output. Reassert the checked/first
    entry when becoming writable or disabled; readonly retains logical focus.
    Refocus only if already inside. Disabled groups keep one tab entry. */
let configure:
  (
    ~items: array(item),
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~dir: direction=?,
    ~orientation: orientation=?,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg));
/** Repair removed/disabled keys. Selection becomes None; focus falls back to
    checked or first enabled. Only refocus if focus was already inside. Reasserts focus after keyed
    DOM reordering; if no enabled item remains, park focus on root.
    Call in the parent's collection-changing update and map its command. */
let reconcile: (~items: array(item), model) => (model, Tea_core.Cmd.t(msg));
/** Set or clear selection; invalid/disabled keys become None. Bypasses
    disabled/read-only user restrictions. No Changed output. */
let set_checked:
  (~items: array(item), option(string), model) =>
  (model, Tea_core.Cmd.t(msg));
/** Reconcile then interpret intent. Focus subscriptions run after render.
    Changed is emitted exactly once per actual user selection change. */
let update:
  (~items: array(item), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Execute immutable generation-keyed focus requests after render. Always map
    this subscription into the parent: stopping cancels queued work. Temporary
    document focus listeners protect intervening user navigation and are removed
    on completion, removal or shutdown. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Render the repaired state through parts. Apply root and visible label.
    description defaults false; required/invalid default false. Required is
    aria-required only: hidden inputs cannot perform native constraint
    validation. Parent validates and controls invalid. name submits checked key.
    Read-only navigation roves tabindex with focus while inside, then restores
    checked/first entry on exit. on_key_down composes unhandled plain non-Tab
    keys without preventing default. Do not duplicate owned event attributes. */
let view:
  (
    ~model: model,
    ~items: array(item),
    ~to_parent: msg => 'parent,
    ~description: bool=?,
    ~required: bool=?,
    ~invalid: bool=?,
    ~name: string=?,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
