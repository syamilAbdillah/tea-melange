# Checkbox

[APG reference](https://www.w3.org/WAI/ARIA/apg/patterns/checkbox/). Native checkbox by default, or a neutral div/span with the APG checkbox role. The parent owns checked and indeterminate state.

## Usage

```reason
open Tea;
/* Parent update, using the CURRENT model, never a rendered closure: */
| Toggle_samples => ({...model,
    checked: Ui.Checkbox.next_checked(
      ~checked=model.checked, ~indeterminate=model.mixed, ()),
    mixed: false,
  }, Cmd.none)

/* Native view: */
Ui.Checkbox.view(
  ~id="samples", ~checked=model.checked, ~indeterminate=model.mixed,
  ~on_change=Toggle_samples, ~name="samples", ~value="included",
  ~to_view=p => Html.div([], [
    Html.input(p.control, []),
    Html.label(p.label, [Html.text("Include samples")]),
    p.hidden_input,
  ]), (),
);

/* Custom view: label MUST be nested, because a div is not labelable. */
Ui.Checkbox.view(
  ~id="custom-samples", ~native=false, ~checked=model.checked,
  ~on_change=Toggle_samples, ~name="samples", ~value="included",
  ~to_view=p => Html.div([], [
    Html.div(p.control, [Html.span(p.label, [Html.text("Include samples")])]),
    p.hidden_input,
  ]), (),
);
```

## Public API

```reason
type parts('msg) = {
  control: list(Tea_html.Html.attr('msg)),
  label: list(Tea_html.Html.attr('msg)),
  description: list(Tea_html.Html.attr('msg)),
  hidden_input: Tea_html.Html.t('msg),
};
let next_checked: (~checked: bool, ~indeterminate: bool=?, unit) => bool;
let parts:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: 'msg,
    ~indeterminate: bool=?,
    ~native: bool=?,
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
    ~indeterminate: bool=?,
    ~native: bool=?,
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
| Tab / Shift+Tab | Native focus navigation; disabled controls skipped. |
| Space down | Native checkbox behavior; custom prevents scrolling, no message. |
| Space up | Native click activation; custom emits one toggle intent and prevents scroll. |
| Enter | Not claimed; does not toggle. |
| Other keys, Ctrl/Alt/Meta chords, IME | Custom helper does not intercept. Native behavior remains browser-owned. |

## Behavior and limits

`on_change` is a **toggle intent message**, not `bool => msg`. This deliberately follows spec §2 intent semantics instead of capturing a stale checked boolean. Parent update uses `next_checked` and clears indeterminate. Indeterminate activation always means checked, regardless of the underlying checked boolean.

Native click defaults are cancelled: HTML restores checked and indeterminate after cancelled activation, then the next render applies the accepted model. This also preserves mixed state if the parent rejects the intent, without impure render effects or document listeners. Native Space still generates exactly one click. Native input/change events are not synthesized; observe the intent callback, not a second change listener. `indeterminate` is a DOM property, not a meaningless attribute.

Custom control: apply `control` only to a neutral focusable div/span. Put a noninteractive span carrying `label` **inside** it, so pointer clicks bubble once and focus reaches the control. External `<label for=div>` is not supported by HTML. Description is outside. No nested interactive descendants. Apply `hidden_input` alongside the custom control; it is `type=hidden` with supplied name/value, successful iff checked and enabled. Checked+mixed submits just as a native checkbox does. Default form value is `on`; no name means no custom hidden input.

Custom keyup activation is stateless, as specified: it is not an armed Space gesture tracker. For native key cancellation/repeat behavior, prefer the default input variant. Native form reset does not reset parent state: handle reset in the parent if offered.

## Styling and ownership

Presence flags: **data-checked, data-indeterminate, data-disabled**. True emits an empty attribute; false omits
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

- jsdom: `dune build @test/ui/checkbox/runtest --force`.
- Chromium: `TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs checkbox`.
- Chromium checks real keyboard/pointer behavior, focus, exact ARIA snapshots
  and unfiltered axe with zero violations across component states.
