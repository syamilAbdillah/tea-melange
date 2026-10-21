# Tabs

[APG tabs](https://www.w3.org/WAI/ARIA/apg/patterns/tabs/). Focus and selected
panel remain separate in Manual mode. Automatic selects on focus and navigation;
use it only for panels ready to display without noticeable latency. Every actual
user selection change emits `Changed(key)`; repair/setters are quiet.

```reason
module T = Tea.Ui.Tabs;
let tabs = T.init(~id="settings", ~items, ~activation=T.Manual, ());
/* In the parent update's Tabs(msg) branch: */
let (tabs, cmd, out) = T.update(~items, msg, tabs);
let cmd = Tea.Cmd.map(msg => Tabs(msg), cmd);
/* View: */
T.view(~model=tabs, ~items, ~to_parent=msg => Tabs(msg),
  ~to_view=parts => Tea.Html.section([], [
    Tea.Html.h2(parts.label, [Tea.Html.text("Settings")]),
    Tea.Html.keyed("div", parts.root,
      Array.to_list(Array.map(part =>
        (part.item.key, Tea.Html.div(part.tab, [Tea.Html.text(part.item.label)])),
        parts.items))),
    Tea.Html.keyed("div", [],
      Array.to_list(Array.map(part =>
        (part.item.key, Tea.Html.div(part.panel,
          part.render_panel ? [Tea.Html.text("Content for " ++ part.item.label)] : [])),
        parts.items))),
  ]), ());
```

Defaults: first enabled selected, Ltr, Horizontal, Automatic, Keep_mounted,
focusable panels. Invalid initial or programmatic selections fall back to first
enabled; empty/all-disabled collections have no selected tab.

## Keyboard

| Key | Action |
| --- | --- |
| Tab / Shift+Tab | Enter selected tab; leave tablist normally |
| Horizontal Left / Right | Previous / next enabled, wrapping |
| Vertical Up / Down | Previous / next enabled, wrapping |
| Home / End | First / last enabled in either orientation |
| Enter / Space | Activate focused tab (idempotent if already selected) |
| Automatic navigation | Move focus and activate |
| Manual navigation | Move focus only |
| Orthogonal arrows / Delete | Unchanged |
| Ctrl/Alt/Meta chords; IME | Unchanged |

RTL swaps horizontal Left/Right, never Up/Down. Manual roving tabindex tracks
focus while inside; leaving resets the entry point to the selected tab.
Disabled tabs are skipped and never selectable, including pointer/AT clicks.

## ARIA, panels and styling

Root is the **tablist only**, with aria-orientation and either aria-labelledby
(default, render `label`) or `aria_label`. Panels must be outside the tablist.
Tab attributes include role=tab, aria-selected, aria-controls and roving tabindex.
Every panel shell gets role=tabpanel, aria-labelledby pointing to its tab and
the actual boolean hidden property. Hidden panels never depend on styles.

Panel tabindex defaults to 0. Set `panel_focusable=key => false` only when that
panel's first meaningful content is itself focusable. Merely having a link
somewhere later in a panel does not justify opting out.

Keep_mounted requests all content. Selected_only requests only selected
**content** via `render_panel`. Always render the lightweight panel shells with
`panel` attributes, even when `render_panel=false`; otherwise aria-controls
would dangle. This is a deliberate refinement of the spec's selected-only
rendering option: unselected content is not mounted, while ID relationships stay
valid in every state. Shells stay hidden and have no keyboard stops.

Tablist has data-orientation; tabs have data-selected, data-active (logical focus
inside tablist), data-disabled; panels have data-selected. Flags are presence
attributes. No inline styles or CSS are required for correctness.

## Full API

```reason
type direction =
  | Ltr
  | Rtl;
type orientation =
  | Horizontal
  | Vertical;
type activation =
  | Automatic
  | Manual;
type panel_rendering =
  | Selected_only
  | Keep_mounted;
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
  tab: list(Tea_html.Html.attr('parent)),
  panel: list(Tea_html.Html.attr('parent)),
  selected: bool,
  focused: bool,
  render_panel: bool,
};
type parts('parent) = {
  root: list(Tea_html.Html.attr('parent)),
  label: list(Tea_html.Html.attr('parent)),
  items: array(item_parts('parent)),
};
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
let selected: model => option(string);
let focused: model => option(string);
let configure:
  (
    ~items: array(item),
    ~dir: direction=?,
    ~orientation: orientation=?,
    ~activation: activation=?,
    model
  ) =>
  (model, Tea_core.Cmd.t(msg));
let reconcile: (~items: array(item), model) => (model, Tea_core.Cmd.t(msg));
let set_selected:
  (~items: array(item), string, model) => (model, Tea_core.Cmd.t(msg));
let update:
  (~items: array(item), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
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

### Private browser capability

Each module privately implements lifecycle-cancelable, latest-request-only focus,
using `Tea_dom.Browser_platform.request_animation_frame`, `getElementById` and
`element.focus()`. Missing nodes are harmless. This is the one exception to
using `Browser_dom.focus` directly: its tasks cannot currently be canceled once
queued. Direct use would steal focus back when an arrow is followed by Tab before
the next render, and would still focus after shutdown. The private capability
uses the same render-before-effect scheduling contract; it should be consolidated
into tea.browser by the integrator. No foundation or browser files are modified.
The small mutable owner holds effect-lifetime tickets only, never domain state
or a collection. All state/navigation remains in the immutable TEA submodel.

## Tests

Both jsdom and Chromium mount real programs. Coverage includes rapid intents
before one draw, disabled and empty/all-disabled/single collections, collection
repair/reordering, keys removed between messages, pointer use, modifiers/IME,
resolved ARIA idrefs, quiet programmatic changes, and shutdown cleanup. Chromium
also tests native Tab/Shift+Tab traversal, every keyboard-table row, ARIA snapshots
and unfiltered axe (zero violations) in each exercised state.

## Limits and example

Delete-to-close and tab popup menus are out of scope; Delete/Shift+F10 remain
available to the browser or composed application hooks. No typeahead is added.
Panel loading, focus within panel content and form/reset behavior belong to the
application. Selection repair chooses first enabled and does not emit Changed.
Configuration changes do not synthesize selection changes. They refocus only
when already inside: Automatic reasserts selected focus; Manual retains logical
focus. Map the returned command.

The [workspace settings example](../../examples/ui/tabs/main.re) demonstrates
both axes crossed with both activation modes, RTL, disabled tabs, accessible
names by visible label and aria-label, both content policies, a panel whose
first content is a link, setters and dynamic collections. The layout is a quiet
blue-and-white settings sheet with visible focus, selected fill and disabled
borders; presentation is never responsible for hiding panels.

Run `dune build @test/ui/tabs/runtest` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs tabs`.
