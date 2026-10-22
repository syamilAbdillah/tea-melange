# Menu

A headless **actions menu**, available as `Tea_ui.Menu` / `Tea.Ui.Menu`.
Implements the [APG menu button](https://www.w3.org/WAI/ARIA/apg/patterns/menu-button/)
and [menu/submenu keyboard pattern](https://www.w3.org/WAI/ARIA/apg/patterns/menubar/).
Use ordinary links and a disclosure/Popover for site navigation, not menu roles.

The native trigger button has `aria-haspopup="menu"`, `aria-expanded`, and
`aria-controls`. Opening moves DOM focus into a `role="menu"` labelled by that
button. Its `aria-activedescendant` references the active stable-keyed item.
Submenus are separately focused menus labelled by their owning items.

## Compose and anatomy

```reason
open Tea;
module M = Ui.Menu;
type msg = Menu(M.msg);
let items: array(M.item) = [|
  {key: "save", label: "Save", disabled: false, kind: M.Action},
  {key: "grid", label: "Grid", disabled: false, kind: M.Checkbox},
  {key: "share", label: "Share", disabled: false, kind: M.Submenu([|
    {key: "link", label: "Copy link", disabled: false, kind: M.Action},
  |])},
|];
let init = () => (M.init(~id="file-actions", ()), Cmd.none);
let update = (Menu(message), model) => {
  let (model, command, outs) = M.update(~items, message, model);
  /* Handle Selected, Checked, Radio_selected, Opened and Closed in the parent. */
  ignore(outs);
  (model, Cmd.map(message => Menu(message), command));
};
let subscriptions = model =>
  Sub.map(message => Menu(message), M.subscriptions(model));
let rec entry = (p: M.item_parts(msg)) => Html.div(p.attrs,
  switch (p.item.kind) {
  | M.Group(_) => [
      Html.div(p.label, [Html.text(p.item.label)]),
      ...Array.to_list(Array.map(entry, p.children)),
    ]
  | M.Separator => []
  | _ => [Html.text(p.item.label)]
  });
let view = model => M.view(
  ~model, ~items, ~to_parent=message => Menu(message),
  ~to_view=p => Html.div([], [
    Html.button(p.trigger, [Html.text("File actions")]),
    ...Array.to_list(Array.map((panel: M.panel_parts(msg)) =>
      Html.div(panel.attrs, Array.to_list(Array.map(entry, panel.items))),
      p.panels)),
  ]), ());
```

The component adds no wrapper. Apply `trigger` to a **native button**, item and
panel bundles to neutral elements. Render every panel, including Closed panels,
as siblings after the trigger in the supplied depth-first order. Never put a
submenu panel inside its owning menuitem: keyboard events must not bubble into
a second menu handler. Groups remain inside their containing panel and render
`item_parts.children`; submenu children instead appear in `parts.panels`.

Use a document-unique id prefix and stable keys **unique throughout the tree**,
including groups/separators. Collections belong to the parent and are passed to
both `update` and `view`, never stored in the model. Keep the model for the
mounted lifetime. Map every returned command and forward subscriptions through
Leaving; removing the component also removes its subscriptions.

Anatomy and event ownership:

- Trigger: button semantics, click, keydown and keyup. Required keys take
  precedence over `on_trigger_key_down`.
- Panel: id, role, hidden, tabindex=-1, name/active-descendant idrefs, direction,
  Anchor/Animation facts, keydown, pointerenter and pointerleave.
- Item: stable id, role, tabindex=-1, ARIA/data state, pointermove, pointerdown
  and click. Pointerdown prevents focus loss; click prevents default and stops
  bubbling. Items have no individual keydown handlers.
- Group: `role="group"`, `aria-labelledby` pointing to its visible `label`
  bundle. `Radio(group_key)` members must name their containing Group's key.
- Separator: `role="separator"`, never active. Group labels are not navigable.

Append styling and nonconflicting facts only. Do not add a second keydown/click
fact, replace owned ids/ARIA/hidden, or introduce interactive descendants,
autofocus, links, buttons, inputs or additional tab stops inside items. Decorative
icons, checkmarks and shortcut hints should use `aria-hidden="true"`; render the
item's `label` as its accessible name and supply that same text for typeahead.
Shortcut hints do not register application shortcuts.

## Keyboard

| Location / key | Behavior |
| --- | --- |
| Trigger Enter / Space / Down | Open with first enabled item active; first navigable if all disabled |
| Trigger Up | Open with last navigable item active, including disabled |
| Menu Down / Up | Next/previous item, wrapping; disabled items remain navigable |
| Menu Home / End | First/last navigable item |
| Menu printable character | Timestamp-based typeahead; repeated character cycles matches |
| Menu Enter on action | Emit Selected, close whole tree, focus trigger |
| Menu Enter on checkbox / radio | Toggle checkbox / select radio, emit change, close tree, focus trigger |
| Menu Space on action | Activate and close tree, focus trigger |
| Menu Space on checkbox / radio | Toggle/select and emit change, **stay open** |
| Forward arrow on submenu item, Enter / Space | Open submenu with first enabled item active (first navigable if all disabled) |
| Back arrow in submenu | Close that submenu and its descendants; focus parent menu with owning item active |
| Escape in submenu | Close only that branch, focus parent menu with owning item active |
| Escape in root or expanded trigger | Close tree, focus trigger |
| Escape while trigger closed | Unclaimed: propagate without preventing default |
| Tab / Shift+Tab | Close whole tree, never prevent default; native traversal starts at trigger position |
| Ctrl/Alt/Meta chords, IME composition | Unchanged |
| Other keys | Unchanged, or optional non-preventing hook |

Forward is Right in LTR, Left in RTL; Back is the opposite. Root Back has no
navigation effect. Claimed Escape prevents default **and stops propagation** so
an enclosing Dialog/Popover does not also close. No other keyboard event stops
propagation: arrows, activation and Tab still reach ancestors. Unclaimed Escape,
modified/composing events and unknown keys also propagate. Parent capture-phase
listeners run before the menu and must respect nested-overlay ownership.

Typeahead uses `Ui_typeahead.character`, including astral keys represented by
UTF-16 surrogate pairs. It lowercases, normalizes NFD, removes U+0300–U+036F
combining marks and applies shared explicit folds (including ligatures). It is
not locale-sensitive collation or full grapheme segmentation. A lone combining
mark does not extend the search. Search resets at **500ms**, including exactly
500ms, or if timestamps go backwards; there is no search timer. Repeated
characters cycle, failed prefixes retry the last character, and navigation
resets the buffer. Space always activates/toggles, rather than extending a
search with a space. In Reason source use `{js|Éxport|js}` for real Unicode JS
strings; plain non-ASCII Reason string literals compile as UTF-8 byte strings.

Disabled items expose `aria-disabled="true"`, remain active-descendant targets
and typeahead candidates, but cannot activate or open submenus. Empty menus have
no active descendant and activation does nothing. Browsing never changes
selection. Checkbox Space staying open is the APG optional behavior; radio Space
selects without closing, even if already selected. Enter on an already selected
radio still emits its selection and closes.

### Why active descendant instead of roving focus?

Only the menu owns DOM focus; items are tabindex=-1 and assistive technology
tracks the keyed `aria-activedescendant`. This keeps a stable keyboard event
surface across immutable rerenders, permits disabled items to remain navigable,
and separates browsing from selection. Roving tabindex would move DOM focus on
every navigation step and require item-level focus coordination. Both are APG
strategies; this implementation deliberately uses the focused-menu approach.
Opening a submenu moves DOM focus to that submenu, and returning activates its
owner in the parent menu, not a separately focused item.

Tab synchronously focuses the trigger as the browser's sequential-navigation
origin, then lets the unprevented native default run. Consequently Tab reaches
the control after the trigger and Shift+Tab the control before it, regardless of
floating panels' DOM position. It is not a custom tabbable-element walker.

## Submenus, pointer and RTL

Each opened submenu owns its own Anchor and Animation. Root placement is
bottom/start, submenus right/start in LTR and left/start in RTL, with 4px offset
and Anchor's flip/shift defaults. `dir` is emitted on every menu. Vertical
navigation does not change in RTL; opening/closing arrows and submenu placement
mirror. Anchor can flip placement at viewport edges without changing key intent.
Depth is unrestricted; stable globally unique keys identify every branch.

Pointermove, not mouseenter, sets active and requests panel focus. Pointerleave
clears active unless that panel has an open child. Clicking an enabled item
activates it; checkbox/radio pointer activation closes the tree. Outside
capture-phase pointerdown uses the trigger, root and still-active submenu panel
ids, so moving into another menu in the tree is not an outside dismissal.
Outside dismissal never requests focus, even for a nonfocusable outside target.

Hover opening defaults to **220ms**, configurable by `hover_delay`. Moving to
another parent row gives the current submenu the same grace interval before
closing/replacing it. Entering the open child cancels the pending parent-row
change; leaving the parent cancels its pending hover. This permits diagonal
movement without geometric pointer-triangle calculations. Timers are cancellable
subscription resources, and repeated moves on the same pending row do not reset
the delay. No press-drag-release activation, touch-hover emulation or geometric
velocity prediction is implemented; click/tap activation works normally.

## Domain state, updates and notifications

`Selected(key)` stores the last action selection; toggles/radio choices do not
replace it. Checkbox and radio state live immutably in the submodel, with silent
setters for parent-owned synchronization. Missing selected/checked/radio keys are
retained and become visible again if descriptors return. These actions do not
provide form inputs or form-reset handling.

User out messages:

- `Opened`: root opening intent, not animation completion.
- `Selected(key)`, `Checked(key, bool)`, `Radio_selected(group_key, key)`:
  enabled item activation, before the corresponding Closed notification.
- `Closed(Escape | Tab | Outside | Trigger | Activation)`: whole-tree logical
  close, never Animation.Left. Submenu-only changes do not emit Opened/Closed.

`open_` and `close` emit no out messages. Open is idempotent; close requests
trigger focus when the root was open. Update repairs active keys against current
items before interpreting an intent; reordered keys survive, removed active
keys repair to a navigable candidate. Removed, reparented or newly disabled
submenu owners close their old branch on update. Keep all rendered idrefs valid
in the same render when changing collections; view does not run commands or
replace a missing active item with a separately mounted virtual row.

## Overlay lifetime and styling

Open calls `Anchor.start` and `Animation.show`; close calls only
`Animation.hide`. Anchor stays active/promoted throughout Leaving and stops
only on `Animation.Left`. Reopening during Leaving reverses the same submodels;
rapid same-frame trigger clicks are intents interpreted against the latest
model. Both embedded command and subscription streams are forwarded.

Panel composition removes Anchor's hidden fact and keeps its id, then adds
Animation's hidden fact: exactly one id and one hidden property. `hidden=true`
**only in Closed**, never Leaving. Logical `open_` and `aria-expanded` are false
during Leaving. No stylesheet is needed for hiding or interaction. All panels
stay mounted, so trigger/submenu `aria-controls` always resolve. Focus waits for
Anchor's visible measurement rather than the end of an entry animation; queued
focus is cancelled on close/unmount.

Top layer defaults true using native manual popovers. It escapes clipping and
transformed ancestors without a portal. Do not call showPopover/hidePopover or
mutate geometry yourself. Set `top_layer=false` only when the host deliberately
owns suitable placement/stacking constraints. Escape/activation restore trigger
focus; Back/submenu Escape restore parent-menu focus. A short
`Browser_events.restore_focus(~within=root_panel_id)` lease protects restoring
exits without acquiring an open-time lease that would steal outside-click
focus. Unmount/shutdown checks ownership of the currently focused menu in the
tree and restores its trigger if it still exists. No focus trap or scroll lock
is acquired.

Presence data attributes (absent when false):

- Trigger and panels: `data-open` while logically open.
- Items: `data-active`, `data-disabled`, `data-checked`, and `data-open` for an
  expanded submenu owner.
- Panels: Animation's `data-closed`, `data-enter`, `data-leave`, `data-transition`;
  Anchor's `data-placement="side-alignment"` and `data-reference-hidden`.
- Panels also carry `data-menu-tree="<id-prefix>"` for focus ownership/cleanup.

Use active/disabled/checked flags to style rows, and Animation flags to style
entry/exit endpoints. Honor reduced motion; do not override hidden or add CSS
required to make closed menus disappear. See the animated example for reversible
motion with the same markup.

## Full public API

```reason
type direction = Ltr | Rtl;
type item = {key: string, label: string, disabled: bool, kind}
and kind = Action | Checkbox | Radio(string) | Submenu(array(item))
  | Group(array(item)) | Separator;
type close_reason = Escape | Tab | Outside | Trigger | Activation;
type model;
type msg;
type out_msg = Selected(string) | Checked(string, bool)
  | Radio_selected(string, string) | Opened | Closed(close_reason);
let init: (~id: string, ~dir: direction=?, ~top_layer: bool=?,
  ~hover_delay: float=?, ~checked: list(string)=?,
  ~radio: list((string, string))=?, unit) => model;
let is_open: model => bool;
let phase: (~submenu: string=?, model) => Animation.phase;
let active_key: (~submenu: string=?, model) => option(string);
let selected: model => option(string);
let set_selected: (option(string), model) => model;
let checked: (~key: string, model) => bool;
let set_checked: (~key: string, bool, model) => model;
let radio_value: (~group: string, model) => option(string);
let set_radio: (~group: string, ~key: option(string), model) => model;
let open_: (~items: array(item), model) => (model, Tea_core.Cmd.t(msg));
let close: model => (model, Tea_core.Cmd.t(msg));
let update: (~items: array(item), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
type item_parts('parent) = {
  item, attrs: list(Tea_html.Html.attr('parent)),
  label: list(Tea_html.Html.attr('parent)),
  children: array(item_parts('parent)), active: bool, checked: bool,
};
type panel_parts('parent) = {
  owner: option(string), attrs: list(Tea_html.Html.attr('parent)),
  items: array(item_parts('parent)), open_: bool, phase: Animation.phase,
};
type parts('parent) = {
  trigger: list(Tea_html.Html.attr('parent)),
  panels: array(panel_parts('parent)),
};
let view: (~model: model, ~items: array(item), ~to_parent: msg => 'parent,
  ~on_trigger_key_down: string => option('parent)=?,
  ~on_menu_key_down: string => option('parent)=?,
  ~to_view: parts('parent) => Tea_html.Html.t('parent), unit) =>
  Tea_html.Html.t('parent);
```

Defaults: closed, LTR, top layer true, hover delay 220ms, no checked keys or radio
associations. `hover_delay` is clamped to a nonnegative timer-safe value;
nonfinite values use 220ms. Omit `submenu` for the root. A panel's `owner=None`
identifies root; `Some(key)` identifies its submenu owner. Hooks receive only
unhandled plain noncomposing key strings and never prevent default. Required
keys, including Tab and Escape intents, take precedence.

## Examples, verification and boundaries

```sh
dune build @examples/melange
dune build @test/ui/menu/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs menu
node scripts/serve.js ui/menu
```

`examples/ui/menu/main.re` includes file actions with decorative icons/shortcut
hints/separator/disabled item, checkbox/radio View settings, Share → Team nested
two levels deep, RTL and animated variants. jsdom checks real UTF-16 Unicode
search, exact fact ownership and hidden state, intent batching, disabled and
empty collections, domain outputs, Escape bubbling, unmount and unresolved
animation cleanup. Chromium checks every keyboard row, pointer/hover grace,
RTL/submenu placement, native Tab targets, focus return, outside focus,
accessibility snapshots and unfiltered axe in closed/open/nested/toggle/animated
states. A real modal dialog test verifies that the first Escape closes only the
menu and the second closes the enclosing dialog.

Private browser bindings are confined to Menu: getElementById/focus with
preventScroll for synchronous native-Tab origin and focus return, activeElement/
closest/getAttribute for unmount ownership, and cancellable hover timeouts.
Post-render focus uses a subscription-local live guard; no mutable state or DOM
handles live in the model. No shared browser capabilities were added, and no
Animation/Anchor changes are required.

Anchor's document-local id, shadow-root/iframe, viewport measurement and
fixed-position fallback limitations apply. Without native Popover API support,
fixed panels cannot escape transformed/clipping ancestors. Unrelated layout
animations are not continuously position-polled. Animation's 5000ms watchdog
bounds paused/infinite effects. There is no global overlay dismissal stack;
claimed Escape protects bubbling ancestors, not earlier capture listeners.
Menus do not automatically dismiss on arbitrary programmatic focus changes.
Leaving content remains mounted/visible for transitions; do not add interactive
descendants or bypass the model's logical-open checks. Virtualization, embedded
widgets, menubars/site navigation, form integration and locale-sensitive text
segmentation are outside this component. Applications supply focus fallbacks if
they remove the trigger, and should test their supported screen readers/devices
in addition to the automated Chromium accessibility checks.

Opening and pointer-driven panel focus is a cancellable post-render subscription.
It waits for a visible Anchor position, then proceeds only while focus remains
on the trigger or the relevant parent menu branch. Moving focus elsewhere before
positioning or before the focus frame leaves that destination alone. Each new
request has an immutable generation; close and shutdown cancel pending work.
Tab's synchronous navigation origin and synchronous Escape/activation return
remain separate so native traversal starts at the correct control.
