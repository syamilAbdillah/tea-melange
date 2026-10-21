# Combobox

[APG combobox pattern](https://www.w3.org/WAI/ARIA/apg/patterns/combobox/).
An editable, input-retained-focus listbox popup. The parent owns suggestions,
filtering, remote requests and labels; the immutable submodel owns text, selected
stable keys, activation and the Animation/Anchor lifecycle.

## Integration

```reason
module C = Tea_ui.Combobox;
let initial = C.init(~id="country", ());
let results = C.filter(~query=C.query(initial),
  ~label=(item: C.item) => item.label, ~mode=C.Starts_with, countries);

/* In parent update: */
let (combo, cmd, outs) = C.update(~items=results,
  ~label_for_key=key => lookup_country_label(key), message, model.combo);
/* Process Query_changed and any async response in the parent, then: */
let results = C.filter(~query=C.query(combo),
  ~label=(item: C.item) => item.label, countries);
let combo = C.refresh(~items=results, ~loading=false, combo);
/* Commit combo/results, map cmd, forward C.subscriptions(combo). */

/* In parent view: */
C.view(~model=combo, ~items=results, ~to_parent=m => Combobox(m),
  ~to_view=p => Html.div([], [
    Html.label(p.label, [Html.text("Country")]),
    Html.div(p.wrapper, [
      Html.input(p.control, []),
      Html.button(p.toggle, [Html.text("Show")]),
    ]),
    Html.div(p.popup, Array.to_list(Array.map((item: C.item) =>
      Html.div(p.option_(item.key), [Html.text(item.label)]), results))),
    Html.div(p.loading, [Html.text("Loading")]),
    Html.div(p.empty, [Html.text("No results")]),
    Html.div(p.live, [Html.text(p.announcement)]),
  ]), ());
```

The snippets illustrate parent bindings, not a standalone program. See
[the runnable example](../../examples/ui/combobox/main.re) for the complete loop:
250 countries/territories, multiple free-form tags, cancellable simulated remote
search, inline completion, none, RTL, clear/revert, read-only/disabled/invalid,
and a combobox inside a modal Dialog. Country data is tzdata's public-domain
ISO 3166 table plus Kosovo (XK), not a sovereignty assertion.

### State, filtering and async results

Pass **current** results to update and view. Do not store them in the submodel.
Each suggestion needs a unique stable string key and a nonempty label. Selection
survives filtering/removal; activation is removed when its item disappears or
becomes disabled. It never silently activates another result. Setters are silent;
set_text and set_selected are independent so parents control synchronization.

Process Query_changed, perform filtering, then call refresh with final results.
For remote requests, refresh with loading=true while pending, discard stale
responses in the parent, and refresh again with the received collection. Repeat
custom result/empty/loading labels on each refresh. Defaults are
“N results available”, “No results”, and “Loading”. The polite atomic status
announces only the final pending label per frame. Keep it mounted outside the
popup, including while closed. Loading and empty parts belong outside the
listbox (which may contain only options); they are hidden while closed.

Restricted(Revert) restores the selected label on unmatched blur/outside;
Restricted(Clear) clears single selection and text. An exact enabled label is
accepted on blur in single restricted mode. Supply label_for_key from the
full catalog/cache when results are filtered or remote; otherwise missing
selected labels necessarily revert to empty. In multiple mode blur clears only
the unfinished query, preserving chips. Free_form leaves text on blur and emits
Submitted(text) on Enter without an active option. The parent creates a stable
key for a new free-form tag and uses the silent setters; the example shows this.

Both completion is a literal case-insensitive prefix with its UTF-16 suffix
selected after render, and activates the candidate. Filtering additionally
supports NFD diacritic folding; an accent-folded match that is not a literal
prefix does **not** acquire an unsafe selection range. Deletion input does not
immediately reinsert completion. Composition updates displayed text but emits
no query until compositionend; composing keydowns never select, clear or close.

### Overlay lifecycle

Opening starts Anchor (wrapper reference, match_width=true, top_layer=true)
and shows Animation. Closing hides Animation only: the popup and Anchor remain
active through Leaving. Animation.Left stops Anchor. Reopen reverses Leaving.
One popup id and exactly one hidden property are composed; hidden is true
**exactly** in Animation.Closed. Keep mounted (as the example does), or unmount
only when should_render=false. Always forward subscriptions, including Leaving.

Outside pointerdown closes and applies the restricted rule without moving
focus. Option and toggle pointerdown prevent blur. Options never take DOM focus.
Toggle click returns focus to input. Claimed Escape stops propagation to an
enclosing Dialog/Popover; an unclaimed closed/empty Escape bubbles normally.

## Keyboard

| Input | Result |
| --- | --- |
| Down (closed/open) | Open + first enabled / next enabled |
| Up (closed/open) | Open + last enabled / previous enabled |
| Alt+Down | Open without changing activation |
| Alt+Up | Close without changing text |
| Enter, active and open | Select, set single text to label, close; prevent form submit |
| Enter, no active | Free_form: Submitted(text); close; prevent default only while open |
| Escape, open | Close, retain text, stop propagation |
| Escape, closed editable with text/single selection | Clear and notify, stop propagation |
| Escape, closed empty or read-only | Unclaimed; ancestor overlay may dismiss |
| Home / End / Left / Right | Native caret/selection; never navigate options |
| Backspace with inline suffix | Remove completion first, keep query |
| Backspace, empty multiple input | Remove last selected chip, keep input focus |
| Backspace otherwise | Native text editing |
| Tab / Shift+Tab | Close without selecting; native focus traversal |
| Typing | Native editing; Query_changed; open if open_on_type |
| Ctrl/Meta/other Alt chords | Unclaimed shortcuts |
| IME | Native editing; query emitted at compositionend |

Navigation skips disabled options and wraps by default; wrap=false retains the
boundary item. PageUp/PageDown remain native (optional APG behavior omitted).
Tab deliberately does **not** accept an active suggestion: this manual-selection
policy avoids committing a merely explored option, especially with async
results. Restricted blur validation still applies to actual input text.
Selection in multiple mode clears query and keeps the popup open.

## ARIA and owned facts

The native input has role=combobox, expanded, autocomplete=list/both/none,
controls only while the popup is rendered, and activedescendant only for an
enabled, active, rendered option while logically open. Popup is role=listbox,
labelled by the visible native label; multiple adds aria-multiselectable.
Each key-derived option id is collision-free, with explicit selected/disabled.
Disabled/read-only/required/invalid and label/description/error behavior mirrors
Input; declare has_description/has_error only for parts actually rendered.

Apply each bundle once. Input owns input/blur/keydown/composition events
(native change is not intercepted); the optional on_key_down hook receives other
plain keys in the **parent** message type, without preventing defaults. Options own
pointerdown/pointermove/click. Toggle/removal buttons own pointerdown/click and
use type=button, tabindex=-1. Chip removal names are “Remove X”; keyboard users
remove chips last-first with Backspace in the empty input. Render their
container using chip(key). Do not append competing event handlers, ids, hidden
or geometry. No CSS is necessary for hiding or keyboard correctness.

Styling: data-open on wrapper/control; data-active, data-selected,
data-disabled on options; data-selected on chips; Input's data-disabled,
data-readonly, data-invalid; Animation's data-closed, data-enter, data-leave,
data-transition; Anchor's data-placement and data-reference-hidden. Flags are
presence attrs. Only Anchor's positioning styles are inline.

### Large lists / future Virtual_list integration

Use active_key(~items, model) to pin the active option in the virtual window.
Pass rendered_keys listing exactly mounted options; no dangling active idref
is emitted otherwise. Mount the pinned option in the same draw before the
post-render scroll_into_view runs. Option ids always derive from keys, never
window indices. Supply aria-posinset/setsize for partial collections. Collections
still belong to the parent; no Virtual_list implementation is bundled here.

## Full public API

```reason
type item = {
  key: string,
  label: string,
  disabled: bool,
};
type autocomplete =
  | List
  | Both
  | None_;
type unmatched =
  | Revert
  | Clear;
type mode =
  | Restricted(unmatched)
  | Free_form;
type filter_mode =
  | Contains
  | Starts_with;
type direction =
  | Ltr
  | Rtl;
type out_msg =
  | Query_changed(string)
  | Selected(string)
  | Removed(string)
  | Selection_changed(list(string))
  | Submitted(string)
  | Opened
  | Closed;
type model;
type msg;
let filter:
  (~query: string, ~label: 'a => string, ~mode: filter_mode=?, array('a)) =>
  array('a);
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
let text: model => string;
let query: model => string;
let selected: model => list(string);
let is_open: model => bool;
let should_render: model => bool;
let phase: model => Animation.phase;
let active_key: (~items: array(item), model) => option(string);
let set_text: (string, model) => model;
let set_selected: (list(string), model) => model;
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));
let set_readonly: (bool, model) => model;
let open_: model => (model, Tea_core.Cmd.t(msg));
let close: model => (model, Tea_core.Cmd.t(msg));
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
let update:
  (
    ~items: array(item),
    ~label_for_key: string => option(string)=?,
    msg,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
type parts('parent) = {
  wrapper: list(Tea_html.Html.attr('parent)),
  label: list(Tea_html.Html.attr('parent)),
  control: list(Tea_html.Html.attr('parent)),
  description: list(Tea_html.Html.attr('parent)),
  error: list(Tea_html.Html.attr('parent)),
  popup: list(Tea_html.Html.attr('parent)),
  toggle: list(Tea_html.Html.attr('parent)),
  option_: string => list(Tea_html.Html.attr('parent)),
  chip: string => list(Tea_html.Html.attr('parent)),
  remove:
    (~key: string, ~label: string) => list(Tea_html.Html.attr('parent)),
  loading: list(Tea_html.Html.attr('parent)),
  empty: list(Tea_html.Html.attr('parent)),
  live: list(Tea_html.Html.attr('parent)),
  announcement: string,
  active_key: option(string),
  selected: list(string),
  is_open: bool,
  should_render: bool,
};
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
```

## Limits and browser capabilities

Multiple combobox-with-chips has no standardized APG chip keyboard pattern;
this API explicitly chooses input-retained focus and last-first Backspace
removal rather than a second roving chip focus system. Single selection uses
the APG editable list-autocomplete baseline. RTL affects Anchor alignment;
native caret keys are never swapped or stolen.

Private bindings add only input selection ranges (value/activeElement/id
lookup and setSelectionRange) plus JavaScript string NFD/lowercase/regex folding.
No mutable model fields, global listeners, timers or DOM node caches are added.
Range writes, scrolling and announcements are lifecycle-gated after render.
Anchor and Animation supply all overlay browser lifecycle capabilities without
workarounds. Anchor's native popover fallback and document-local id limitations
apply; see [Anchor](anchor.md) and [Animation](animation.md). Filtering folds like
typeahead (lowercase, NFD, U+0300..036F marks removed, æ/œ/ø/ß/þ/ð/đ/ł/ı
folded); it is not a locale-specific collation or full Unicode casefold, and
marks outside U+0300..036F are retained. Unicode source literals in Reason/Melange should use
`{js|…|js}`; normal DOM/remote JavaScript strings already use UTF-16.

## Verification

```sh
dune build @test/ui/combobox/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs combobox
```

jsdom mounts real programs and checks intent batching, every keyboard policy,
modifiers/composition, selection, filtering, inline ranges, pointer/blur,
idrefs/hidden composition, live announcements, lifecycle reversal, shutdown
during an unfinished animation and listener/timer/observer cleanup. Chromium
drives the actual example, verifies native caret/Tab behavior, full ARIA
snapshots, large-list scrolling, chips, remote loading, RTL and modal Escape
layering, with unfiltered axe checks in every meaningful state.
