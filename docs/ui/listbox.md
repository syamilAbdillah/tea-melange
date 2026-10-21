# Listbox

A button-triggered custom select with a focus-owning popup. Use native
[Select](select.md) unless custom option presentation or multiple-selection tags
justify a custom control. Implements the
[APG listbox pattern](https://www.w3.org/WAI/ARIA/apg/patterns/listbox/), with a
button rather than a combobox trigger, like Headless UI/Foldkit's select pattern.

Unlike the [select-only combobox example](https://www.w3.org/WAI/ARIA/apg/patterns/combobox/examples/combobox-select-only/),
DOM focus moves **into the listbox**, whose `aria-activedescendant` identifies a
stable keyed option. The trigger remains a native button, not `role=combobox`.
Assistive technology therefore announces a popup button followed by a focused
listbox, rather than a continuously focused combobox; combobox-specific screen
reader shortcuts are not promised. Chromium accessibility snapshots and axe are
automated; manual screen-reader/device testing remains an application concern.

Selection never follows browsing. Tab closes **without** committing the active
option: this deliberately differs from APG's select-only combobox example and
avoids accidental changes when leaving a form field. Multiple selection uses
the APG no-modifier-required model. Optional Alt+Up commit and Ctrl+A/range
selection extensions are not implemented; browser/AT modifier shortcuts remain
untouched. Options cannot contain interactive descendants.

## Compose

```reason
open Tea;
module L = Ui.Listbox;
type msg = Box(L.msg);
type model = {box: L.model};
let items: array(L.item(string)) = [|
  {key: "apple", label: "Apple", disabled: false, value: "a"},
  {key: "pear", label: "Pear", disabled: false, value: "p"},
|];
let init = () => ({box: L.init(~id="fruit", ())}, Cmd.none);
let update = (Box(message), model) => {
  let (box, command, outs) = L.update(~items, message, model.box);
  /* Handle L.Changed({keys, values}), L.Opened and L.Closed in the parent. */
  ignore(outs);
  ({box}, Cmd.map(message => Box(message), command));
};
let subscriptions = model =>
  Sub.map(message => Box(message), L.subscriptions(model.box));
let view = model => L.view(
  ~model=model.box, ~items, ~to_parent=message => Box(message), ~name="fruit",
  ~form_value=item => item.value,
  ~to_view=p => Html.div(p.root, [
    Html.span(p.label, [Html.text("Fruit")]),
    Html.button(p.button, [Html.text(p.display_value)]),
    Html.div(p.listbox,
      Array.to_list(Array.map((item: L.item(string)) =>
        Html.div(p.option_(item.key), [Html.text(item.label)]), items))
      @ [Html.div(p.empty, [Html.text("No options available")])]),
  ] @ p.form_inputs), ());
```

The parent retains items, maps commands/subscriptions and forwards messages.
Use document-unique `id` prefixes and unique, stable string keys. Render the
popup even when closed: `Animation` owns its `hidden` property, and the button's
`aria-controls` always resolves. Do not put an HTML `label` around the button;
the noninteractive `label` part supplies the accessible name.

Group related options by nesting them in `Html.div(p.group("key"), ...)`, with
`Html.div(p.group_label("key"), [Html.text("Group name")])` first. Group keys
must be unique within this listbox; render only nonempty groups. The library
does not store a group collection or assume that all options are mounted.

Part event ownership: button owns click/keydown/keyup; listbox owns keydown/blur;
options own pointermove/pointerdown/click. Append styling or other nonconflicting
attributes only. Compose additional unhandled plain key behavior with
`~on_key_down`; it cannot replace required behavior or prevent default. Hooks
never receive IME or Ctrl/Alt/Meta chords. Button parts must be on a native
`button`; options should be neutral elements with no independently focusable
children. Pointerdown prevents listbox blur; pointermove, not mouseenter, changes
the active option, so scrolling under a stationary pointer does not activate it.

## Keyboard

| Location / key | Behavior |
| --- | --- |
| Trigger Enter / Space / Down | Open at first enabled selected item, otherwise first enabled |
| Trigger Up | Open at first enabled selected item, otherwise last enabled |
| Trigger printable character | Open, then search using that character |
| List Up / Down | Previous/next enabled option; clamp by default, `wrap=true` wraps |
| List Home / End | First/last enabled option |
| List PageUp / PageDown | Jump ten collection positions, skipping a disabled landing target |
| List printable character | Typeahead; repeated same letter cycles matching options |
| List Space during live search | Continue the nonempty search, including spaces inside labels |
| List Enter / Space outside live search | Single: commit, close, focus trigger. Multiple: toggle, remain open |
| Escape | Close without changing selection; focus trigger |
| Tab / Shift+Tab | Close without changing selection; never prevent native traversal |
| Ctrl/Alt/Meta chords, IME | Unchanged, including optional Alt+Up (not implemented) |
| Unhandled keys | Unchanged, or optional hook without preventing default |

Typeahead uses the event's `timeStamp`, with a **500ms** reset (including exactly
500ms), not timers or a wall clock. Case and Latin-1 accents/ligatures are folded
by `Ui_typeahead`; no full Unicode normalization or locale-sensitive casefolding.
A failed prefix retries the last character. Navigation resets the search. An
expired Space commits rather than extending an old search. No selected value
changes before an explicit selection intent.

Read-only controls open, navigate, typeahead and dismiss normally, but keyboard
and pointer cannot change their selection. Disabled controls have a native
disabled button and cannot open; disabling an open list closes it without
requesting focus on the now-disabled button. The parent should focus a logical
fallback when disabling the current control. RTL changes logical Anchor
alignment; vertical list navigation stays Up/Down and horizontal arrows remain
unhandled.

## Collection changes, forms and virtualization

Call `reconcile(~items=new_items, model)` in the same parent update that replaces
items. `update` and `view` also repair derived active state. A retained enabled
active key survives reorders. A removed key repairs around its last collection
position; a disabled key repairs around its current position; the nearest
enabled neighbor wins, forward on ties. Empty/all-disabled arrays have no active
key, omit `aria-activedescendant`, and Enter does nothing. Render `empty` inside
the listbox: it is an unselected, disabled informational option, ensuring valid
listbox ownership semantics even when there are no domain options. It is hidden
when items are nonempty and is never navigated to or committed.

**Missing selected keys are retained.** Returning items recover their selection.
`selected_keys` and `Changed.keys` include those keys, while `Changed.values`
omits missing values and preserves selection order. `parts.selected`, button
text and form inputs include only present selected items, in collection order.
A missing sole selection consequently displays the placeholder. To drop stale
keys, the parent filters them and calls `set_selected`; setters never emit
`Changed`. Disabled options can be selected programmatically, but users cannot
select/toggle them.

`~name` opts into hidden inputs: one per present selected key, even in multiple
mode, with `~form_value` defaulting to the key. Render `form_inputs` inside the
owning form. Disabled controls submit nothing; read-only controls submit values.
There is no input for a missing/empty selection. Form reset is parent-owned:
call `set_selected(initial_keys, model)` from the parent's reset handling.

`option_(key)` and active-descendant ids are keyed, not index-based. The component
only tries to scroll **the active option**, through
`Browser_dom.scroll_into_view(~block=\`Nearest, ...)` after render; it never
queries a DOM option collection. A future/current
[Virtual_list](virtual_list.md) composition must:

1. Keep the complete ordered items array in the parent for navigation/search.
2. Resolve `parts.active_key` to its current collection index and pin it in the
   **same render**, so the active descendant always exists inside this listbox.
3. Forward `Virtual_list.scroll_to_key`/`scroll_to_index` in the same update as an
   active change, before rendering a window that depends on that destination.
4. Compose owned facts without duplicate ids or ARIA: Listbox owns popup/option
   semantics and option ids; Virtual_list owns row geometry/observation and
   content/container scrolling. Filter its duplicate row id/setsize/posinset
   when applying both bundles; do not override Listbox's active-descendant id.

`aria-posinset` and `aria-setsize` describe the full supplied collection. This
example renders all 200 rows; it does not claim to be a virtualized example.

## Overlay lifetime and styling

Opening calls `Anchor.start` and `Animation.show`; closing calls only
`Animation.hide`. Anchor stays active and its native manual popover remains
promoted through Leaving; only `Animation.Left` stops Anchor. Both embedded
subscriptions are always forwarded. Top-layer promotion defaults **true**,
match-width true, offset 4px, bottom/start with Anchor's flip/shift defaults.
Focus waits for Anchor's first positioning render (not animation completion),
since an initially `visibility:hidden` panel cannot accept DOM focus. A retained
position allows immediate focus on reversal. No stylesheet is needed for
correct hiding or interaction.

Composition removes Anchor's `hidden` fact and retains its id, then applies
Animation's `hidden`. There is exactly one of each; `hidden` is true **only** in
Closed, including when a kept mount reopens. Leaving and Closed are also native
`inert` and `aria-hidden=true`: a visually exiting, logically closed panel must neither accept focus
nor remain exposed as an interactive scroll region to assistive technology.
`aria-expanded` reflects logical
open state, not Leaving. Outside capture-phase pointerdown uses both button and
popup ids and never requests focus. Escape/selection/trigger-close request
trigger focus; Tab and blur/outside dismissal do not. A blur into the trigger
is left for its click to toggle, avoiding close-then-reopen races.

The model is entirely immutable, including focus generations. A private
subscription-owned runtime lease registry (document-unique id, removed on stop)
cancels obsolete queued focus, because `Browser_dom.focus` has no cancellation
handle. Private `getElementById` and `focus({preventScroll:true})` bindings execute
only in guarded post-render callbacks. No capabilities were added to shared
browser modules. An open lease wraps
`Browser_events.restore_focus(~within=popup_id)` for shutdown/unmount restoration;
normal close decisions use explicit guarded trigger focus, and discard the
resource-free restoration callback. This prevents outside/Tab dismissal from
stealing focus, including a click on nonfocusable text. Shutdown removes leases,
Anchor listeners/popovers and Animation waits, including unresolved animations.

Presence attributes:

- Root/button/listbox: `data-open`, `data-disabled`, `data-readonly`, `data-invalid`.
- Options: `data-active`, `data-selected`, `data-disabled`.
- Animation on listbox: `data-closed`, `data-enter`, `data-leave`, `data-transition`.
- Anchor on listbox: `data-placement="side-alignment"`, `data-reference-hidden`.

The native trigger has `aria-haspopup="listbox"`, `aria-expanded`, `aria-controls`
and `aria-labelledby="<label-id> <button-id>"` (label plus displayed selection).
The popup has `role=listbox`, label idref, open tabindex=0 (closed/Leaving -1),
`inert`/`aria-hidden` when logically closed,
`aria-activedescendant` when active, `aria-multiselectable`, and readonly/disabled/
invalid ARIA states where applicable. Options have explicit `aria-selected` and
`aria-disabled`. Group labels resolve via `role=group`/`aria-labelledby`.

## Full API

```reason
type item('value) = {key: string, label: string, disabled: bool, value: 'value};
type selection('value) = {keys: list(string), values: list('value)};
type out_msg('value) = Changed(selection('value)) | Opened | Closed;
type model;
type msg;
let init: (~id: string, ~selected: list(string)=?, ~multiple: bool=?,
  ~disabled: bool=?, ~readonly: bool=?, ~invalid: bool=?, ~wrap: bool=?,
  ~dir: Anchor.direction=?, ~top_layer: bool=?, unit) => model;
let is_open: model => bool;
let should_render: model => bool;
let phase: model => Animation.phase;
let selected_keys: model => list(string);
let active_key: model => option(string);
let set_selected: (list(string), model) => model;
let configure: (~disabled: bool=?, ~readonly: bool=?, ~invalid: bool=?,
  ~wrap: bool=?, model) => (model, Tea_core.Cmd.t(msg));
let reconcile: (~items: array(item('value)), model) => (model, Tea_core.Cmd.t(msg));
let open_: (~items: array(item('value)), ~last: bool=?, model) =>
  (model, Tea_core.Cmd.t(msg));
let close: (~return_focus: bool=?, model) => (model, Tea_core.Cmd.t(msg));
let update: (~items: array(item('value)), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg('value)));
let subscriptions: model => Tea_core.Sub.t(msg);
type parts('value, 'parent) = {
  root: list(Tea_html.Html.attr('parent)),
  label: list(Tea_html.Html.attr('parent)),
  button: list(Tea_html.Html.attr('parent)),
  listbox: list(Tea_html.Html.attr('parent)),
  option_: string => list(Tea_html.Html.attr('parent)),
  group: string => list(Tea_html.Html.attr('parent)),
  group_label: string => list(Tea_html.Html.attr('parent)),
  empty: list(Tea_html.Html.attr('parent)),
  selected: array(item('value)), active_key: option(string),
  display_value: string, is_open: bool, should_render: bool,
  form_inputs: list(Tea_html.Html.t('parent)),
};
let view: (~model: model, ~items: array(item('value)), ~to_parent: msg => 'parent,
  ~placeholder: string=?, ~name: string=?, ~form_value: item('value) => string=?,
  ~on_key_down: string => option('parent)=?,
  ~to_view: parts('value, 'parent) => Tea_html.Html.t('parent), unit) =>
  Tea_html.Html.t('parent);
```

Defaults: closed, empty single selection, enabled/writable/valid, no wrap,
LTR, top layer true, placeholder `"Select an option"`, no form name. `last=false`
and `return_focus=false`. Programmatic controls emit no user out messages;
`Closed` reports completed Animation leave, while `Opened` reports user opening
intent and `Changed` only a genuinely different user selection.

## Run and boundaries

```sh
dune build @test/ui/listbox/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs listbox
node scripts/serve.js ui/listbox
```

The example includes groups/disabled options, tag removal, read-only, repeated
form values and submit, 200 scrolling items, RTL, empty/disabled/invalid/wrapping
states, and live collection replacement. Tests cover every keyboard row, real
typeahead timing, single/multiple selection, idrefs, focus, pointer behavior,
rapid intents/reversals, unresolved-animation cleanup, form serialization,
accessibility snapshots and unfiltered axe. No shared files are changed.

Anchor's documented top-layer fallback, viewport/layout visibility, shadow DOM
and iframe limitations apply. Without Popover API support, fixed-position panels
cannot escape transformed/clipping ancestors. Animation's 5000ms watchdog bounds
paused/infinite transitions. The parent supplies missing-opener focus fallbacks,
form reset and a synchronous active-row pinning contract when virtualizing.
