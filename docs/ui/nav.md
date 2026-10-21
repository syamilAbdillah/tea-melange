# Nav

[APG navigation guidance](https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/examples/disclosure-navigation/).
A named native navigation landmark containing ordinary native links. This is
**not** a menu or menubar: no roving tabindex, arrow-key navigation or typeahead.

```reason
Ui.Nav.view(
  ~id="sections", ~name=Ui.Nav.Label("Sections"),
  ~keys=[|"Overview", "Resources"|], ~current="Overview",
  ~to_view=p => Html.ul(p.list,
    Array.to_list(Array.map(item => Html.li([], [
      Html.a(item.link @ [Html.href("/" ++ item.key)], [Html.text(item.key)]),
    ]), p.items))),
  (),
);
```

`view` itself renders the native nav and applies root. Its callback renders
contents, not another nav or another copy of root. `parts` is the wrapper-free
alternative; apply root to exactly one native nav.

## Identity, naming and current destination

The keys array is supplied anew on every render, never stored. Duplicate keys
raise Invalid_argument, including non-current duplicates: this prevents two
bundles claiming the same id/current state. Apply every link bundle once.
Keys can be any string; collision-free ids derive from sanitized keys, not index.

Required `name` is `Label(nonempty_text)` (aria-label; whitespace-only raises
Invalid_argument), or `Labelledby` (aria-labelledby points to label part).
For Labelledby, render label once with nonempty visible text in or outside nav.
The label bundle is available with both naming strategies.

Only the item matching current gets aria-current and data-current. Default
token is page; step/location/date/time/true are supported. Absent or unknown
current marks no link. Removing a current item does not silently reassign it;
reintroducing the key restores its marker. Empty and single-item lists work.
This is a destination marker, not aria-selected.

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Native next/previous link focus |
| Enter on a link | Native link navigation |
| Space, arrows, Home, End, modified keys, IME | Native browser behavior |

No keys/defaults are intercepted; no events or tabindex are supplied. Caller
provides href (and optionally a routing intent handler). Native same-page
fragment navigation may move focus to the target: the example makes its
destination programmatically focusable with tabindex=-1.

## Parts and styling

Root: nav id and required name. Label: derived label id. List: derived list id,
apply to ul/ol and render each anchor inside li. Link: derived stable id,
aria-current only for current, and presence `data-current=""` only for current.
All noncurrent markers are absent, not false strings. Caller supplies content,
hrefs, classes and any nonconflicting router events.

The example switches naming modes, every token, and empty/single/multiple
collections while keeping the current destination parent-owned.

## Full public API

```reason
type name = Label(string) | Labelledby;
type current_token = Page | Step | Location | Date | Time | True;
type item('msg) = {
  key: string,
  link: list(Tea_html.Html.attr('msg)),
};
type parts('msg) = {
  root: list(Tea_html.Html.attr('msg)),
  label: list(Tea_html.Html.attr('msg)),
  list: list(Tea_html.Html.attr('msg)),
  items: array(item('msg)),
};
let parts:
  (
    ~id: string,
    ~name: name,
    ~keys: array(string),
    ~current: string=?,
    ~current_token: current_token=?,
    unit
  ) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~name: name,
    ~keys: array(string),
    ~current: string=?,
    ~current_token: current_token=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
```

## Scope and verification

No disabled, read-only or RTL options apply to this ordinary link navigation helper.
No private browser capabilities, global listeners, locks or dependencies on CSS.
Parts are typed in the parent message; there is no submodel or to_parent mapping.
Caller-provided labels, hrefs and nonconflicting markup remain the caller's responsibility.

Run `dune build @test/ui/nav/runtest --force` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs nav`.
The standalone example is `examples/ui/nav/`. jsdom mounts a real Tea app,
checks normalized state/ARIA, rapid parent changes and listener-free shutdown.
Chromium checks the keyboard table, pointer use, focus, exact ARIA snapshots
and unfiltered axe with zero violations across states. No assistive-technology
or Safari/device compatibility claim is implied by Chromium testing.
