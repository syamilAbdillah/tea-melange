# Virtual list

A stateful, headless window over a parent-owned keyed collection. Fixed heights
and estimated/measured variable heights share a pure sparse prefix-sum index.
There is no imposed role, selection model, disabled policy, or keyboard handler.

[APG listbox](https://www.w3.org/WAI/ARIA/apg/patterns/listbox/) and
[active-descendant focus management](https://www.w3.org/WAI/ARIA/apg/practices/keyboard-interface/#kbd_focus_activedescendant)
define the consumer's interaction, not virtualization itself.

## Usage

```reason
open Tea;
module Virtual = Ui.Virtual_list;
/* Build once, and rebuild only when keys or their order change. Domain rows
   remain in the parent; this collection contains only a snapshot of their keys. */
let keys = Virtual.items(Array.init(100000, i => "row-" ++ string_of_int(i)));
let virtual_ = Virtual.init(
  ~id="results", ~items=keys, ~row_size=Virtual.Fixed(32.),
  ~viewport=240., (),
);
/* Parent update: */
let update_virtual = (msg, model) => {
  let (virtual_, cmd, out) = Virtual.update(~items=keys, msg, model);
  (virtual_, cmd, out);
};
/* Parent maps commands/subscriptions with Cmd.map/Sub.map as usual. */
let subscriptions = model => Sub.map(msg => Virtual_msg(msg),
  Virtual.subscriptions(model));
let view = model => Virtual.view(
  ~model, ~items=keys, ~to_parent=msg => Virtual_msg(msg),
  ~to_view=parts =>
    Html.div(parts.container @ [
      Html.role("list"), Html.aria("label", "Results"),
      Html.tabindex(0), Html.style("height", "240px"),
    ], [
      Html.keyed("div", parts.content @ [Html.role("presentation")],
        List.map(row => (row.key,
          Html.div(row.attrs @ [Html.role("listitem")],
            [Html.text(row.key)])), parts.rows)),
    ]),
  (),
);
```

The parent message constructor `Virtual_msg` is application-defined. Apply all
parts to the corresponding elements. Give the container a constrained height
with an inline geometry attribute, a layout constraint, or a stylesheet.
No stylesheet is otherwise required. Content must have **only keyed row
elements** as direct children; no spacer or sentinel elements. Do not reuse
array indices as keys. Arbitrary keys, including empty/prototype-looking keys
and punctuation, are supported; duplicates are rejected at collection creation.
Keep the `items` value in the parent, not in the virtual submodel. It copies
keys and creates a null-prototype key-to-index dictionary in O(n). Reuse it
until insertion, removal or reorder. Calling `items` inside every view would
unnecessarily rebuild the index and measurement tree.

For a collection change, call `refresh(~items=new_items, model)` in the same
parent update, mapping its command. Every update/view/scroll command also
synchronizes automatically. Removed measurements and removed alignment targets
are discarded; retained measurements follow their keys across reorders.
`view` repairs locally but cannot commit a model: use `refresh` to commit
changes even when no scroll/resize event occurs.

## Layout and performance

Rows use **absolute top positioning**, not transforms. Content is relatively
positioned with the estimated/measured total height; each row is full-width,
border-box, zero-margin, with a top equal to its prefix sum. Fixed rows also
have an exact inline height. Estimated rows obtain their natural height and
are measured as border boxes, including padding/borders. Geometry resets
content margins/padding and container padding; decorate inside rows, and do
not override the owned geometry. Container borders are supported. Avoid row
transforms, vertical margins, and CSS height transitions: they invalidate the
height contract or repeatedly move the viewport.

The pure tree stores deviations from an estimated base. Construction is O(1);
measurement and prefix/inverse-prefix queries are O(log n), total size is O(1).
Range queries walk at most two root-to-leaf paths, independent of scroll
distance. The public `operations` counts actual visited nodes so the million-row
test asserts a bound (42), not machine-dependent timing. Range endpoints are
half-open and conservatively include a row exactly at the bottom boundary.
Overscan is in rows and defaults to two. View cost is O(w log n), plus sorting
pinned indices; w is the mounted window plus valid distinct pins, not n.

**Memory policy:** keep at most one size per currently present key, not a
historical cache. A collection change immediately prunes absent keys and
rebuilds the positional index from retained measurements, O(m log n).
Measurement memory is O(n) at worst; the sparse tree is O(n) at worst and
allocates only paths containing measured deviations. There is deliberately no
LRU eviction: scrolling back to a visited region retains accurate offsets.
The parent key snapshot/index is O(n); the model never retains the collection.
Old immutable versions become garbage unless the application retains history.

Variable lists use **one ResizeObserver** for the container and every mounted
row, requested with `box: "border-box"`. A single child-list MutationObserver
reconciles observed row nodes after keyed rendering. Reconciliation compares
mounted identities in O(w²), never scans the full collection. Fixed lists
observe only the container and need no MutationObserver. A local element-level
scroll handler decodes `target.scrollTop` and `target.clientHeight`; no document
or window scroll listener is installed. Scroll and resize payloads coalesce
to at most one delivery/update per animation frame, latest sample/size wins.
Observers and pending deliveries are cancelled by subscription disposal,
including shutdown before deferred installation.

Measurements above the viewport preserve its first visible row plus intra-row
pixel offset. Browser scroll anchoring is disabled to avoid double adjustment.
Programmatic commands establish an alignment target (key plus alignment),
render its estimated window, and refine that target when measurements arrive.
A real scroll away cancels the target. Internal render generations prevent
queued old scroll samples and superseded DOM commands from undoing newer
programmatic intent; keyed size samples remain usable.

## Commands and focus contract

`scroll_to_index(~items, ~index, ~align, model)` and `scroll_to_key` return a
new model and a post-render DOM command. Start/Center/End request their named
alignment, clamped at collection boundaries. Auto (default) scrolls only when
the target is not fully visible; oversized rows align their start when below
the window. Invalid indices and missing keys do nothing. Commands do not focus,
select, or emit domain changes. `Viewport_changed` is an observational frame
notification and may follow either programmatic or user scrolling.

A Listbox/Combobox integration should:
1. Resolve the next active **key** in its own update, using current items.
2. Call `scroll_to_index` before moving focus or changing active descendant.
3. Commit both models in the same parent update and map the returned command.
4. Pass `~pinned=[active_index]` on every view; include any separately focused
   row too. Pins are unioned with the window, deduplicated, and rendered in
   collection order. Keep pin count bounded.
5. Use `row_id(~key, model)` for aria-activedescendant and ensure the required
   DOM ownership/controls relationship. On focus entry, scroll the active row
   into view again. For roving focus, chain Browser_dom.focus after rendering.

A pin alone guarantees DOM existence, **not visibility**. It prevents dangling
idrefs during rapid frame-batched navigation, pointer scrolling, and distant
jumps. Do not hide pinned off-window rows: they must remain in the accessibility
tree. The example demonstrates selection following focus and explicit focus
restoration after pointer selection, but is not the future full Listbox API.

## Keyboard and ARIA

| Key | Virtual list |
| --- | --- |
| Tab / Shift+Tab | Native focus traversal; never prevented |
| Arrows, Home, End, PageUp, PageDown, Space, Enter | Untouched; native scroll or consumer behavior |
| Ctrl/Alt/Meta chords and IME | Untouched |

The listbox-like example adds ArrowUp/Down, Home/End and PageUp/Down (seven rows)
with selection following focus, no wrapping. It leaves Tab, IME and modified
shortcuts alone; browser shortcut defaults can still scroll. This is consumer
code, not an imposed virtualization keyboard model.

With default `semantics=Set`, each row has `aria-setsize` equal to the full
collection count and one-based `aria-posinset`. Use list/listitem,
listbox/option, or feed/article roles as appropriate. With `semantics=Grid`,
the container instead has `aria-rowcount` and rows have `aria-rowindex`;
supply grid/row/gridcell (or table/row/cell), column metadata, accessible names
and the appropriate consumer keyboard/focus model yourself. The example can
switch its fixed collection between list and static table semantics.

Styling hooks: `data-virtual-list=""` on container and `data-pinned=""` on
each explicitly pinned row. Internal `data-tea-virtual-key` and
`data-tea-virtual-generation` attributes belong to observation/race control.
Ids derive from the unique init prefix: `-viewport`, `-content`, and safe
key-derived row ids. Never override owned geometry, ids or internal attributes.

## Full public API

```reason
module Geometry: {
  type t;
  type range = {
    first: int,
    last: int,
    operations: int,
  };
  let create: (~count: int, ~estimate: float) => t;
  let measure: (~index: int, ~size: float, t) => t;
  let offset: (~index: int, t) => float;
  let total: t => float;
  let range: (~viewport: float, ~scroll: float, ~overscan: int=?, t) => range;
};
type items;
let items: array(string) => items;
let count: items => int;
let key_at: (~index: int, items) => option(string);
type row_size =
  | Fixed(float)
  | Estimated(float);
type align =
  | Start
  | Center
  | End
  | Auto;
type model;
type msg;
type out_msg =
  | Viewport_changed(Geometry.range);
let init:
  (
    ~id: string,
    ~items: items,
    ~row_size: row_size,
    ~viewport: float=?,
    ~overscan: int=?,
    unit
  ) =>
  model;
let scroll_offset: model => float;
let viewport_height: model => float;
let total_size: model => float;
let measurement_count: model => int;
let visible_range: model => Geometry.range;
let row_offset: (~index: int, model) => float;
let row_id: (~key: string, model) => string;
let refresh: (~items: items, model) => (model, Tea_core.Cmd.t(msg));
let update:
  (~items: items, msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
let scroll_to_index:
  (~items: items, ~index: int, ~align: align=?, model) =>
  (model, Tea_core.Cmd.t(msg));
let scroll_to_key:
  (~items: items, ~key: string, ~align: align=?, model) =>
  (model, Tea_core.Cmd.t(msg));
type semantics =
  | Set
  | Grid;
type row('parent) = {
  key: string,
  index: int,
  offset: float,
  attrs: list(Tea_html.Html.attr('parent)),
};
type parts('parent) = {
  container: list(Tea_html.Html.attr('parent)),
  content: list(Tea_html.Html.attr('parent)),
  rows: list(row('parent)),
  range: Geometry.range,
  total: float,
};
let view:
  (
    ~model: model,
    ~items: items,
    ~pinned: list(int)=?,
    ~semantics: semantics=?,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
```

## Limits

- Vertical lists, finite positive CSS-pixel sizes only. Invalid estimates become
  one pixel; invalid measurements are ignored. No horizontal/RTL math is needed.
- Collections may be 1M rows in pure geometry, but browsers cap scrollable
  element dimensions (Chromium roughly 33 million CSS pixels). This module does
  not implement segmented/rebased scrolling beyond the browser's pixel limit.
- Without ResizeObserver, estimates remain usable, viewport height is read on
  mount and scroll, but size changes without scrolling cannot be detected.
- Subscribe only while the parts are mounted. Replacing a container node with
  the same id requires stopping/restarting subscriptions, as with browser
  element-resize subscriptions. Nested lists need their own unique prefixes.
- This does not expose all unmounted text to browser Find, printing, or a
  screen reader's noninteractive browse mode. Use pagination/full rendering
  when those workflows are required. Selection and disabled/read-only policies
  are entirely the consumer's domain; there is no implicit skipping of rows.

## Verification

`dune build @test/ui/virtual_list/runtest --force` runs the Reason/jsdom suite,
including an exhaustive linear geometry oracle, a million-row operation bound,
coalescing, DOM identity, arbitrary keys, pins, corrected jumps, anchoring,
reordering/pruning, empty/single collections, unsupported observers, rapid
commands and shutdown. All-disabled collections have no special geometry case;
the consumer owns disabled semantics and navigation.

`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs virtual_list`
checks bounded DOM during wheel/distant scrolling, unseen fixed/variable jumps,
actual border-box growth and anchor compensation, list/table/option ARIA
snapshots, pointer focus, native focus traversal, shortcut/IME handling,
every active index through 1,000 rows, and unfiltered zero-violation axe checks
in each demonstrated state.
