# Radio group

[APG radio group](https://www.w3.org/WAI/ARIA/apg/patterns/radio/), roving-tabindex
variant **outside a toolbar**. Selection and logical focus are distinct; user
selection changes emit `Changed(key)`. Programmatic setters and collection
repairs emit no output.

```reason
module R = Tea.Ui.Radio_group;
/* Parent init: */
let group = R.init(~id="schedule", ~items, ());
/* Parent update, when receiving Group(msg): */
let (group, cmd, out) = R.update(~items, msg, group);
let cmd = Tea.Cmd.map(msg => Group(msg), cmd);
/* Parent view: */
R.view(~model=group, ~items, ~to_parent=msg => Group(msg),
  ~name="schedule", ~required=true, ~description=true,
  ~to_view=parts => Tea.Html.div([], [
    Tea.Html.h2(parts.label, [Tea.Html.text("Delivery schedule")]),
    Tea.Html.p(parts.description, [Tea.Html.text("Choose a delivery schedule.")]),
    Tea.Html.keyed("div", parts.root,
      Array.to_list(Array.map(part =>
        (part.item.key, Tea.Html.div(part.root, [Tea.Html.text(part.item.label)])),
        parts.items))),
    Option.value(~default=Tea.Html.text(""), parts.hidden_input),
  ]), ());
```

Defaults: unchecked, enabled, writable, Ltr; Vertical is a visual hint.
No collection is retained by `init`.

## Keyboard

| Key | Action |
| --- | --- |
| Tab / Shift+Tab | Enter at checked radio, or first enabled; leave normally |
| Right / Down | Next enabled, wrapping; focus and check |
| Left / Up | Previous enabled, wrapping; focus and check |
| Space | Check focused radio if unchecked |
| Enter / Home / End | No built-in action |
| Ctrl/Alt/Meta chords; IME | Unchanged |
| Read-only arrows | Move focus without checking |
| Read-only Space | Prevent scroll; no change |
| Disabled group | No navigation or checking; keep one tab entry |

RTL swaps Left/Right only; both axes always work, independent of orientation.
Repeated Space on a checked radio emits no duplicate output.

## ARIA, forms and styling

Render root with role=radiogroup and the visible label with `label` attributes.
`description=true` opts into the description idref; then render `description`.
Each item gets role=radio, aria-checked=true|false and its accessible name.
Only checked (or first enabled when unchecked) is tabbable during normal use.
Disabled individual radios are skipped, cannot be selected, and prevent pointer
focus. A disabled **group** uses aria-disabled and keeps its normal entry point
discoverable; arrows, Space and clicks are inert. All-disabled/empty groups have
no item tab entry.

Read-only uses aria-readonly on the group. Navigation must not change selection,
but must still allow a single Tab to leave: while inside, readonly tabindex
roves with focus; on exit it resets to checked/first. This is an intentional
clarification of the normal checked-only tabindex rule, which cannot satisfy
both requirements when readonly focus differs from checked.

`required` emits aria-required, not native constraint validation. Parent controls
`invalid` and validation/error messaging. A supplied `name` produces
`parts.hidden_input` with the checked key; **render it inside the form**. No
selection means no hidden input, rather than submitting a misleading empty key.
Disabled groups disable the input and do not submit; readonly values do submit.
Hidden inputs cannot implement native required validation. Form reset is
parent-owned: call `set_checked` with the initial value in your reset handler.

The orientation option emits data-orientation only: **aria-orientation is not
valid on radiogroup**. Root also carries data-disabled, data-readonly,
data-invalid. Items carry data-checked, data-active (logical focus within),
data-disabled, data-readonly. All flags are presence attributes; no CSS is
needed for behavior.

## Full API

```reason
type direction =
  | Ltr
  | Rtl;
type orientation =
  | Horizontal
  | Vertical;
type item = {
  key: string,
  label: string,
  disabled: bool,
};
type model;
type msg;
type out_msg =
  | Changed(string);
type item_parts('parent) = {
  item,
  root: list(Tea_html.Html.attr('parent)),
  checked: bool,
  focused: bool,
};
type parts('parent) = {
  root: list(Tea_html.Html.attr('parent)),
  label: list(Tea_html.Html.attr('parent)),
  description: list(Tea_html.Html.attr('parent)),
  items: array(item_parts('parent)),
  hidden_input: option(Tea_html.Html.t('parent)),
};
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
let checked: model => option(string);
let focused: model => option(string);
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
let reconcile: (~items: array(item), model) => (model, Tea_core.Cmd.t(msg));
let set_checked:
  (~items: array(item), option(string), model) =>
  (model, Tea_core.Cmd.t(msg));
let update:
  (~items: array(item), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
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
```

## Integration contract

Keep the collection in the parent, with unique stable string keys. Pass the
same collection to `update` and `view`. When replacing or reordering it, call
`reconcile(~items, model)` in that parent update and map its command. This also
reasserts focus after browsers blur a DOM node moved by keyed rendering.
`update` independently repairs stale keys before interpreting every event.
Use keyed rendering and document-unique id prefixes.

Map both returned commands and `subscriptions` through the parent constructor.
Subscriptions install **no global listeners**: they own the lifetime of pending
focus work. Their cleanup makes a queued focus inert on shutdown/reinitialization.
Focus requests are coalesced after render; a subsequent outside focus cancels
them, so rapid arrow presses cannot rewind state or steal focus back after Tab.
If the last enabled item disappears while focused, focus parks on the root
(`tabindex=-1`) rather than being abandoned on body. Outside collection changes
never steal focus.

Item parts are for neutral elements, not native inputs/buttons or anchors.
Do not place interactive descendants in radios/tabs or override owned event
facts. The keydown hook composes unhandled plain keys without preventing their
default; required shortcuts take precedence. Tab never reaches hooks. Item
names come from `item.label`; render the same text visibly. All derived IDs
encode keys with the foundation's collision-free UTF-16 safe-key encoding.

### Browser focus capability

Focus requests are immutable generation-keyed model data, executed by
`Browser_events.focus` after render. A newer request cancels its predecessor;
leaving the group or shutdown cancels pending work. An intervening focus movement
wins, including moving away and back before the frame. Missing nodes are harmless.
Only subscription-local closures hold cancellation cells; models contain no
mutable effect owner or DOM state.

## Tests

Both jsdom and Chromium mount real programs. Coverage includes rapid intents
before one draw, disabled and empty/all-disabled/single collections, collection
repair/reordering, keys removed between messages, pointer use, modifiers/IME,
resolved ARIA idrefs, quiet programmatic changes, and shutdown cleanup. Chromium
also tests native Tab/Shift+Tab traversal, every keyboard-table row, ARIA snapshots
and unfiltered axe (zero violations) in each exercised state.

## Limits and example

Not the toolbar-specific radio keyboard pattern. No typeahead, Enter or Home/End
shortcuts are added. Disabled keys are treated like removed keys: selection
clears, rather than silently selecting another value. Configuration changes
reassert the checked/first focus entry on writable/disabled groups when inside;
readonly retains logical focus. Map their command just like a domain setter.
Domain setters bypass disabled/readonly restrictions intentionally. Invalid selections passed to setters clear selection.

The [settings form example](../../examples/ui/radio_group/main.re) demonstrates
disabled items, disabled/read-only groups, RTL, required/invalid and hidden form
values, clearing selection, and changing collections. Its deliberately spare
blue-and-white settings layout uses visible focus and state borders, not CSS
for correctness.

Run `dune build @test/ui/radio_group/runtest` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs radio_group`.
