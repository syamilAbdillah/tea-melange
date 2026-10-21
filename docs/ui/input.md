# Input

[APG reference](https://www.w3.org/WAI/ARIA/apg/patterns/). Controlled native text input. APG has no separate native text-entry pattern; the browser owns editing and form behavior.

## Usage

```reason
open Tea;
Ui.Input.view(
  ~id="name", ~name="name", ~value=model.name,
  ~on_input=value => Name_changed(value),
  ~has_description=true, ~has_error=model.invalid, ~invalid=model.invalid,
  ~required=true, ~autocomplete="name",
  ~to_view=p => Html.div([], [
    Html.label(p.label, [Html.text("Your name")]),
    Html.input(p.control, []),
    Html.p(p.description, [Html.text("Your reservation name.")]),
    model.invalid ? Html.p(p.error, [Html.text("Enter your name.")]) : Html.text(""),
  ]), (),
);
```

## Public API

```reason
type parts('msg) = {
  label: list(Tea_html.Html.attr('msg)),
  control: list(Tea_html.Html.attr('msg)),
  description: list(Tea_html.Html.attr('msg)),
  error: list(Tea_html.Html.attr('msg)),
};
let parts:
  (
    ~id: string,
    ~value: string,
    ~on_input: string => 'msg,
    ~on_change: string => 'msg=?,
    ~on_blur: 'msg=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~name: string=?,
    ~type_: string=?,
    ~autocomplete: string=?,
    ~inputmode: string=?,
    unit
  ) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~value: string,
    ~on_input: string => 'msg,
    ~on_change: string => 'msg=?,
    ~on_blur: 'msg=?,
    ~has_description: bool=?,
    ~has_error: bool=?,
    ~invalid: bool=?,
    ~required: bool=?,
    ~disabled: bool=?,
    ~readonly: bool=?,
    ~name: string=?,
    ~type_: string=?,
    ~autocomplete: string=?,
    ~inputmode: string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
```

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Native focus navigation; readonly stays focusable, disabled is skipped. |
| Text, arrows, Home/End | Native text editing. |
| Shift/Ctrl/Alt/Meta chords | Native selection/editing/browser shortcuts. |
| Enter | Native implicit form submission when applicable. |
| IME | Native composition and input events; never intercepted. |

## Part presence and validation

`label.for` resolves to the derived `id-control`. `description` and `error`
have derived ids. `has_description` and `has_error` default false: set them
**only if those parts are rendered**. False flags also make a rendered part
hidden. `aria-describedby` lists description then error, only when present.
`aria-errormessage` references error only when invalid AND present.
`aria-invalid=true` appears only when invalid. The caller renders error text;
no CSS is needed to hide absent parts.

## Behavior and limits

`type_` defaults to `text`; `autocomplete`, `inputmode` and `name` pass through unchanged. Use text-like input types, not file/checkbox/radio controls (use the corresponding dedicated component). File inputs cannot have controlled nonempty string values by HTML design. `on_change` receives the committed DOM value; `on_blur` is a message. Disabled/readonly controls do not emit input/change callbacks, including synthetic events. Readonly values still submit; disabled values do not. The helper does not mutate your validation state.

## Styling and ownership

Presence flags: **data-invalid, data-disabled, data-readonly**. True emits an empty attribute; false omits
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

- jsdom: `dune build @test/ui/input/runtest --force`.
- Chromium: `TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs input`.
- Chromium checks real keyboard/pointer behavior, focus, exact ARIA snapshots
  and unfiltered axe with zero violations across component states.
