/** Button-triggered, focus-owning single/multiple listbox.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/listbox/
    Compare https://www.w3.org/WAI/ARIA/apg/patterns/combobox/examples/combobox-select-only/.

    Keyboard table:
    - Trigger Enter/Space/Down: open at selected or first enabled.
    - Trigger Up: open at selected or last enabled.
    - Printable characters: open/search; 500ms reset, repeated-letter cycling.
    - Popup Up/Down: previous/next enabled (no wrap unless configured).
    - Home/End: first/last enabled; PageUp/PageDown: ten positions.
    - Enter/Space: commit; single closes, multiple toggles and stays open.
      Space continues an unexpired, nonempty search instead of committing.
    - Escape: cancel browsing, close and focus trigger.
    - Tab/Shift+Tab: close without committing; never prevent native traversal.
    - Ctrl/Alt/Meta chords and IME: unchanged. No Alt+Up commit shortcut.
    Readonly permits browsing but never commits. Selection never follows focus.

    Presence data attributes: data-open, data-disabled, data-readonly,
    data-invalid, data-active, data-selected; Animation adds data-closed,
    data-enter, data-leave, data-transition; Anchor adds data-placement and
    data-reference-hidden. Apply bundles once; append nonconflicting attrs.
    Render the popup even when closed (hidden is owned by Animation).
    No CSS required. Options must not contain interactive descendants. */

/** Parent-owned ordered collection; keys must be unique and stable. */
type item('value) = {
  /** Identity, never an array index. */
  key: string,
  /** Plain accessible text for typeahead and the default display value. */
  label: string,
  /** Visible but skipped and not selectable. */
  disabled: bool,
  /** Arbitrary domain value; never compared for equality. */
  value: 'value,
};
/** Committed key set in selection order and corresponding currently present
    values. Missing selected keys are retained; their values are omitted. */
type selection('value) = {
  /** Unique selected keys; at most one in single mode. */
  keys: list(string),
  /** Values of keys still in the supplied items, in key selection order. */
  values: list('value),
};
/** User changes and lifecycle completion. Browsing does not emit Changed. */
type out_msg('value) =
  | Changed(selection('value))
  | Opened
  | Closed;
/** Immutable state; collections are not stored. */
type model;
/** Opaque event intents and forwarded Anchor/Animation messages. */
type msg;
/** Create closed; defaults: empty single selection, enabled, writable, valid,
    no wrapping, LTR, native top-layer promotion. */
let init:
  (
    ~id: string,
    ~selected: list(string)=?,
    ~multiple: bool=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~invalid: bool=?,
    ~wrap: bool=?,
    ~dir: Anchor.direction=?,
    ~top_layer: bool=?,
    unit
  ) =>
  model;
/** Logical open state; false while Leaving. */
let is_open: model => bool;
/** Lifecycle mounting state, including Leaving. Popup is normally kept mounted. */
let should_render: model => bool;
/** Embedded animation phase. */
let phase: model => Animation.phase;
/** Unique committed keys, including missing keys. */
let selected_keys: model => list(string);
/** Active identity; call reconcile when the parent changes items. */
let active_key: model => option(string);
/** Programmatic domain setter; no out messages. Deduplicates; single keeps first. */
let set_selected: (list(string), model) => model;
/** Configure supplied flags; disabling closes (a disabled native trigger cannot
    receive focus; the parent should choose a fallback). No out messages. */
let configure:
  (
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~invalid: bool=?,
    ~wrap: bool=?,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg));
/** Repair after collection changes in the same parent update. A removed/disabled
    active key moves to the nearest enabled position (forward on ties). Selected
    missing keys are retained. Schedules post-render nearest scrolling. */
let reconcile:
  (~items: array(item('value)), model) => (model, Tea_core.Cmd.t(msg));
/** Open programmatically without out messages. last chooses last fallback;
    the first enabled selected item in collection order always takes precedence. */
let open_:
  (~items: array(item('value)), ~last: bool=?, model) =>
  (model, Tea_core.Cmd.t(msg));
/** Begin leave, keeping Anchor active until Left. return_focus defaults false;
    true focuses the trigger. Outside/Tab dismissal never requests focus. */
let close: (~return_focus: bool=?, model) => (model, Tea_core.Cmd.t(msg));
/** Interpret against latest items/state; forward every returned command. */
let update:
  (~items: array(item('value)), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg('value)));
/** Required throughout the mounted lifetime, including Leaving. Shutdown stops
    Anchor promotion/listeners, suppresses pending focus/Animation work and
    restores an open popup's opener when it is still connected. All cancellation
    cells are subscription-owned, never part of model. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Parent-message-typed attributes and derived content. */
type parts('value, 'parent) = {
  /** Optional layout wrapper and state data flags. */
  root: list(Tea_html.Html.attr('parent)),
  /** Apply to visible noninteractive label text (span, not HTML label). */
  label: list(Tea_html.Html.attr('parent)),
  /** Native button only. Owns click, keydown, keyup and all button semantics. */
  button: list(Tea_html.Html.attr('parent)),
  /** Focus-owning popup, tabindex=0 while open, -1 otherwise. Owns keydown,
      blur, id, hidden, inert, positioning and ARIA. Leaving/Closed are inert,
      suppressing interaction/AT exposure without hiding the exit animation.
      Focus entering the trigger is
      ignored by blur so the trigger's subsequent click can toggle closed. */
  listbox: list(Tea_html.Html.attr('parent)),
  /** Option attrs by stable key. Owns pointermove, pointerdown and click.
      Includes full-collection aria-posinset/setsize for virtualization. */
  option_: string => list(Tea_html.Html.attr('parent)),
  /** Group wrapper attrs. Render only nonempty groups within the popup. */
  group: string => list(Tea_html.Html.attr('parent)),
  /** Noninteractive visible group label; key must match its group. */
  group_label: string => list(Tea_html.Html.attr('parent)),
  /** Nonselectable disabled option containing the empty-state message; hidden
      when items are nonempty. Never receives active-descendant or selection. */
  empty: list(Tea_html.Html.attr('parent)),
  /** Present selected items in collection order, for display/tags. */
  selected: array(item('value)),
  /** Render this option even in a virtualized window; it must be a descendant
      of the popup in the same render before effects scroll/focus it. */
  active_key: option(string),
  /** Comma-joined labels or placeholder. Render inside button for its name. */
  display_value: string,
  /** Logical interaction state. */
  is_open: bool,
  /** True during enter/open/leave; not a request to unmount the kept popup. */
  should_render: bool,
  /** Hidden inputs, one per present selected key; render inside the form.
      Disabled listboxes submit nothing. Readonly values are submitted. */
  form_inputs: list(Tea_html.Html.t('parent)),
};
/** Wrapper-free view. Render label, native button, popup (even Closed) and
    form_inputs. Default placeholder is "Select an option". name is opt-in;
    form_value defaults to item.key. on_key_down composes unhandled keys without
    preventing defaults (shortcuts/IME never reach the hook). Virtualized views
    must render active_key immediately, not in a later asynchronous window. */
let view:
  (
    ~model: model,
    ~items: array(item('value)),
    ~to_parent: msg => 'parent,
    ~placeholder: string=?,
    ~name: string=?,
    ~form_value: item('value) => string=?,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('value, 'parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
