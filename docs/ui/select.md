# Select

[APG reference](https://www.w3.org/WAI/ARIA/apg/patterns/combobox/). A controlled **native** select, not a custom Listbox. Native platform behavior deliberately takes precedence over the APG custom combobox/listbox keyboard models.

## Usage

```reason
open Tea;
module S = Ui.Select;
let teas = [|
  S.Option({key:"sencha", label:"Sencha", disabled:false}),
  S.Group({key:"oxidized", label:"Oxidized teas", disabled:false,
    items:[|{key:"oolong", label:"Oolong", disabled:false}|]}),
|];
S.view(
  ~id="tea", ~items=teas, ~value=model.selection,
  ~on_change=value => Selection_changed(value),
  ~name="tea", ~placeholder="Choose a tea", ~required=true,
  ~to_view=p => Html.div([], [
    Html.label(p.label, [Html.text("First tea")]),
    Html.keyed("select", p.control, p.options),
  ]), (),
);
/* For multiple: ~value=Multiple([|"sencha"|]).
   When items can change between messages:
   Selection_changed(value) => {...model,
     selection: S.normalize(~items=model.items, value)} */
```

## Public API

```reason
type option_item = {
  key: string,
  label: string,
  disabled: bool,
};
type item =
  | Option(option_item)
  | Group({
      key: string,
      label: string,
      disabled: bool,
      items: array(option_item),
    });
type selection =
  | Single(option(string))
  | Multiple(array(string));
type parts('msg) = {
  label: list(Tea_html.Html.attr('msg)),
  control: list(Tea_html.Html.attr('msg)),
  description: list(Tea_html.Html.attr('msg)),
  error: list(Tea_html.Html.attr('msg)),
  options: list((string, Tea_html.Html.t('msg))),
};
let normalize: (~items: array(item), selection) => selection;
let parts:
  (
    ~id: string,
    ~items: array(item),
    ~value: selection,
    ~on_change: selection => 'msg,
    ~placeholder: string=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~name: string=?,
    unit
  ) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~items: array(item),
    ~value: selection,
    ~on_change: selection => 'msg,
    ~placeholder: string=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~name: string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
```

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Native focus navigation; disabled control skipped. |
| Single ArrowDown / ArrowUp | Native next/previous enabled option. |
| Single Home / End | Native first/last enabled option. |
| Printable text | Native platform typeahead. |
| Alt+ArrowDown / Space | Open native popup where the platform supports it. |
| Enter / Escape | Native popup commit/dismiss. |
| Multiple ArrowDown / ArrowUp | Native focus/selection movement, skipping disabled options. |
| Multiple Shift+Arrow | Native range extension. |
| Multiple Ctrl+Arrow / Space | Platform-native focus movement/toggling; Chromium Linux covered. |
| Multiple Home / End | Native first/last option. |
| Other shortcuts / IME | Browser/OS-owned, never intercepted. |

## Part presence and validation

`label.for` resolves to the derived `id-control`. `description` and `error`
have derived ids. `has_description` and `has_error` default false: set them
**only if those parts are rendered**. False flags also make a rendered part
hidden. `aria-describedby` lists description then error, only when present.
`aria-errormessage` references error only when invalid AND present.
`aria-invalid=true` appears only when invalid. The caller renders error text;
no CSS is needed to hide absent parts.

## Behavior and limits

Use `Html.keyed("select", p.control, p.options)`, not `Html.select`: options and groups have stable keys. Option keys are unique across **all** groups; group keys are unique among groups. Keys are native raw option values and form submission values, never indices. Option ids are collision-safe encodings of keys. Group ids use separate prefixes. Collections live in the parent.

`Single(None)` renders a selected disabled empty-valued placeholder. If no placeholder text was supplied, it is also hidden; the browser never silently picks the first item. `Multiple` omits the placeholder. Required, invalid, disabled and name use native semantics; group/option disabled flags propagate natively. A real option with key `""` is distinguished from the placeholder by id, but its empty native form value fails native required validation. This is HTML behavior, not a reserved logical key.

Actual selectedOptions values are read on change, using data-only decoders over the HTMLCollection. Numeric collection properties are used solely to enumerate DOM options; neither indices nor selectedIndex identify options. `normalize(~items, selection)` drops missing/duplicate keys, retaining disabled selections for programmatic control. View normalizes for display. Parent update must also normalize against its current collection when items change before a frame. Empty/all-disabled/single-item collections are valid. No programmatic normalization emits change.

Multiple values serialize as repeated name entries in DOM order. There is no custom popup, focus trap, roving tabindex, or custom typeahead. OS/browser key differences are intentional, not an APG custom-listbox implementation. Native reset does not reset parent state; handle it in parent update.

## Styling and ownership

Presence flags: **data-invalid, data-disabled**. True emits an empty attribute; false omits
it. No styles, global listeners, subscriptions or browser effects are needed.
Parts own the ids, semantics, state attributes and events documented in the
interface. Append only nonconflicting attributes; one element has one handler
per event type. Stateless parts already carry the parent's message type.
`view(..., ~to_view, ())` adds no wrapper; `parts(..., ())` permits direct
bundle composition. Use document-unique id prefixes.

## Example and verification

Run `node scripts/serve.js ui/forms`. The combined tasting form demonstrates
validation errors, text read-only state, mixed native/custom checkboxes, native
and custom fieldset disabling, optgroups, single/multiple selection and
empty/all-disabled/single/reordered collections. Submission prints **actual
FormData entries**, not reconstructed model values.

- jsdom: `dune build @test/ui/select/runtest --force`.
- Chromium: `TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs select`.
- Chromium checks real keyboard/pointer behavior, focus, exact ARIA snapshots
  and unfiltered axe with zero violations across component states.
