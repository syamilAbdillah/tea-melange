# Switch (Switch_)

[APG reference](https://www.w3.org/WAI/ARIA/apg/patterns/switch/). Native button with role=switch and binary aria-checked. The Reason keyword-safe public module is **Tea.Ui.Switch_** (source `switch_.re`), not Switch.

## Usage

```reason
open Tea;
/* In update: Toggle_updates => {...model, updates: !model.updates} */
Ui.Switch_.view(
  ~id="updates", ~checked=model.updates, ~on_change=Toggle_updates,
  ~name="updates", ~value="subscribed", ~has_description=true,
  ~to_view=p => Html.div([], [
    Html.label(p.label, [Html.text("Tasting updates")]),
    Html.button(p.root, [Html.text(model.updates ? "On" : "Off")]),
    Html.p(p.description, [Html.text("Receive tasting news.")]),
    p.hidden_input,
  ]), (),
);
```

## Public API

```reason
type parts('msg) = {
  root: list(Tea_html.Html.attr('msg)),
  label: list(Tea_html.Html.attr('msg)),
  description: list(Tea_html.Html.attr('msg)),
  hidden_input: Tea_html.Html.t('msg),
};
let parts:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: 'msg,
    ~disabled: bool=?,
    ~has_description: bool=?,
    ~name: string=?,
    ~value: string=?,
    unit
  ) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: 'msg,
    ~disabled: bool=?,
    ~has_description: bool=?,
    ~name: string=?,
    ~value: string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
```

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Native focus navigation; disabled button skipped. |
| Enter | Native click activation on keydown. |
| Space down | Native button prevents scrolling; no toggle yet. |
| Space up | Native button click toggles exactly once. |
| Other keys/chords/IME | Browser-owned; no synthetic key handler. |

## Behavior and limits

Apply `root` **only to a native button**. It sets `type=button` to avoid accidental form submission. `label` belongs on a native `<label>` linked with `for`; clicking it invokes the button once and focuses it. `aria-labelledby` provides the accessible name. Keep that label constant as required by APG.

`on_change` is a **toggle intent message**; parent update toggles its latest state, so two clicks in one frame toggle twice. This intentional clarification of the illustrative bool-callback signature in spec §2.1 follows the normative intent rule in §2.2. No keydown/keyup handler is installed, preventing Space double activation.

Render `hidden_input` for form participation. With `name`, a hidden input submits the supplied `value` (default `on`) only when checked and enabled; unchecked is omitted, like a checkbox. Without a name there is no hidden input. Native fieldset disabling also excludes it. Parent state owns reset behavior.

## Styling and ownership

Presence flags: **data-checked, data-disabled**. True emits an empty attribute; false omits
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

- jsdom: `dune build @test/ui/switch/runtest --force`.
- Chromium: `TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs switch`.
- Chromium checks real keyboard/pointer behavior, focus, exact ARIA snapshots
  and unfiltered axe with zero violations across component states.
