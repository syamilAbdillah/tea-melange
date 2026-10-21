/** Editable list-autocomplete combobox with input-retained DOM focus.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/combobox/

    Keyboard table:
    - Down/Up: open and activate first/last, then next/previous enabled option.
    - Alt+Down: open without activation. Alt+Up: close, retain text.
    - Enter: select active; otherwise Submitted(text) in Free_form. Prevented
      only while open; closed Enter retains native form submission.
    - Escape: close when open; otherwise clear editable text/single selection.
      Claimed Escape stops propagation; empty/read-only closed Escape bubbles.
    - Tab/Shift+Tab: close without selecting; never prevent native traversal.
    - Home/End/Left/Right: native caret movement, no option navigation.
    - Backspace: discard inline completion first; on empty multiple input,
      remove last chip; otherwise native editing.
    - Typing: native editing, optionally open. Shortcuts and IME pass through.
      Query_changed is deferred until compositionend.
    Data: data-open, data-active, data-selected, data-disabled, data-invalid,
    data-readonly, Animation's data-closed/enter/leave/transition and Anchor's
    data-placement/reference-hidden. Flags are presence attributes.
    No stylesheet is required for correctness. */

/** Parent-owned suggestion; keys must be unique and stable, labels nonempty. */
type item = {
  key: string,
  label: string,
  disabled: bool,
};
/** ARIA autocomplete token; default List. Both adds selected inline completion. */
type autocomplete =
  | List
  | Both
  | None_;
/** Unmatched restricted input on blur/outside: restore selected label or clear. */
type unmatched =
  | Revert
  | Clear;
/** Default Restricted(Revert); Free_form emits Submitted on unselected Enter. */
type mode =
  | Restricted(unmatched)
  | Free_form;
/** Pure filtering strategy; default Contains. */
type filter_mode =
  | Contains
  | Starts_with;
/** Logical horizontal Anchor alignment; caret behavior remains native. */
type direction =
  | Ltr
  | Rtl;
/** Immediate user notifications; setters never emit these. */
type out_msg =
  | Query_changed(string)
  | Selected(string)
  | Removed(string)
  | Selection_changed(list(string))
  | Submitted(string)
  | Opened
  | Closed;
/** Immutable state; never retains the parent's collection or DOM nodes. */
type model;
/** Opaque keyboard/pointer intents and embedded lifecycle progress. */
type msg;
/** Folds query and labels like typeahead (lowercase, Unicode NFD, U+0300..036F
    marks removed, æ/œ/ø/ß/þ/ð/đ/ł/ı folded). Stable order; empty query
    matches all. Not locale-sensitive or a full Unicode casefold. */
let filter:
  (~query: string, ~label: 'a => string, ~mode: filter_mode=?, array('a)) =>
  array('a);
/** Initially closed. Defaults: empty text/selection, single, List, wrap=true,
    open_on_type=true, enabled/editable, Ltr, top_layer=true, match_width=true.
    id is a document-unique prefix. Selected labels are supplied separately
    through text; set_text/set_selected are deliberately independent. */
let init:
  (
    ~id: string,
    ~text: string=?,
    ~selected: list(string)=?,
    ~mode: mode=?,
    ~multiple: bool=?,
    ~autocomplete: autocomplete=?,
    ~wrap: bool=?,
    ~open_on_type: bool=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~dir: direction=?,
    ~top_layer: bool=?,
    ~match_width: bool=?,
    unit
  ) =>
  model;
/** Controlled displayed text, including any inline suffix. */
let text: model => string;
/** Parent filtering query, excluding inline suffix. */
let query: model => string;
/** Selected stable keys, in insertion order. */
let selected: model => list(string);
/** Logical expanded state, false during Leaving. */
let is_open: model => bool;
/** Retain popup through Leaving; may always mount it instead. */
let should_render: model => bool;
/** Embedded animation phase. */
let phase: model => Animation.phase;
/** Enabled active key if still present; absent does not select another item. */
let active_key: (~items: array(item), model) => option(string);
/** Set displayed text and query; clears activation/completion, no output. */
let set_text: (string, model) => model;
/** Set selection, deduplicating and taking at most one key in single mode. */
let set_selected: (list(string), model) => model;
/** Disable and close with normal leave lifecycle; no output. */
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));
/** Read-only permits opening/navigation, forbids domain edits. */
let set_readonly: (bool, model) => model;
/** Open/reverse Leaving without notifications or focus movement. */
let open_: model => (model, Tea_core.Cmd.t(msg));
/** Close without notifications/focus movement; Anchor stops only on Left. */
let close: model => (model, Tea_core.Cmd.t(msg));
/** Repair activation and recompute inline completion/loading/announcement.
    Call after parent filtering or async results and before subscriptions/view.
    Labels default to N results available / No results / Loading. Repeat custom
    labels on each refresh. No commands, output, collection retention or I/O. */
let refresh:
  (
    ~items: array(item),
    ~loading: bool=?,
    ~results_label: int => string=?,
    ~empty_label: string=?,
    ~loading_label: string=?,
    model
  ) =>
  model;
/** Forward intents using current results. label_for_key defaults to lookup in
    items; supply a full-catalog/cache lookup for filtered or remote selections
    so restricted Revert can recover labels absent from current results.
    Reconcile parent Query_changed/Submitted then refresh with final results. */
let update:
  (
    ~items: array(item),
    ~label_for_key: string => option(string)=?,
    msg,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Forward throughout lifetime, including Leaving. Owns outside pointer,
    positioning, cancellable animation waits, frame-debounced announcements,
    active-option scroll and inline selection range. Shutdown disposes all. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Apply bundles once to their matching elements; append nonconflicting facts. */
type parts('parent) = {
  /** Contains input and toggle; outside containment and Anchor reference. */
  wrapper: list(Tea_html.Html.attr('parent)),
  /** Native label, id and for. */
  label: list(Tea_html.Html.attr('parent)),
  /** Native text input. Owns input/blur/keydown/compositionstart/end,
      controlled value, validation, ARIA and id. No duplicate handlers. */
  control: list(Tea_html.Html.attr('parent)),
  /** Optional description, render iff has_description. */
  description: list(Tea_html.Html.attr('parent)),
  /** Optional error, render iff has_error. */
  error: list(Tea_html.Html.attr('parent)),
  /** Listbox element: single id/hidden, Anchor geometry and Animation flags.
      Never override hidden; true exactly when phase=Closed. */
  popup: list(Tea_html.Html.attr('parent)),
  /** Optional native button; tabindex=-1, pointerdown/click, no input blur. */
  toggle: list(Tea_html.Html.attr('parent)),
  /** Nonfocusable option; owns pointerdown/pointermove/click. */
  option_: string => list(Tea_html.Html.attr('parent)),
  /** Selected tag container; resolve labels using parent catalog/cache. */
  chip: string => list(Tea_html.Html.attr('parent)),
  /** Native chip button with Remove X name, tabindex=-1; owns pointerdown/click.
      Keyboard removal is Backspace in empty input, last first. */
  remove:
    (~key: string, ~label: string) => list(Tea_html.Html.attr('parent)),
  /** Loading message outside listbox; hidden unless open and loading. */
  loading: list(Tea_html.Html.attr('parent)),
  /** Empty message outside listbox; hidden unless open, empty and not loading. */
  empty: list(Tea_html.Html.attr('parent)),
  /** Always mount polite atomic status outside the popup. */
  live: list(Tea_html.Html.attr('parent)),
  /** Render as text inside live. Last results change in a frame wins. */
  announcement: string,
  /** Enabled active key for virtualizer pinning. */
  active_key: option(string),
  /** Selected keys for chips. */
  selected: list(string),
  /** Logical expanded state. */
  is_open: bool,
  /** Lifecycle visibility, including Leaving. */
  should_render: bool,
};
/** Wrapper-free parent-message-typed view. Input conventions match Input.
    rendered_keys, when supplied, lists actually mounted options: idrefs are
    omitted for others. A virtualizer must pin active_key and render its option
    before post-render scroll; add aria-posinset/setsize itself. Never portal
    options away from the popup. Keep popup mounted through Leaving.
    Hook receives other plain keys and cannot prevent native defaults. */
let view:
  (
    ~model: model,
    ~items: array(item),
    ~to_parent: msg => 'parent,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~name: string=?,
    ~rendered_keys: list(string)=?,
    ~on_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
