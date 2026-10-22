# Fieldset

[APG reference](https://www.w3.org/WAI/ARIA/apg/patterns/checkbox/). Native fieldset and first legend group related controls. No synthetic role or group keyboard navigation.

## Usage

```reason
open Tea;
Ui.Fieldset.view(
  ~id="preferences", ~disabled=model.disabled, ~has_description=true,
  ~to_view=p => Html.fieldset(p.root, [
    Html.legend(p.legend, [Html.text("Preferences")]),
    Html.p(p.description, [Html.text("Your tasting preferences.")]),
    /* Pass p.disabled to custom controls; native descendants inherit it. */
    Ui.Switch_.view(
      ~id="updates", ~checked=model.updates, ~on_change=Toggle_updates,
      ~disabled=p.disabled,
      ~to_view=s => Html.div([], [
        Html.label(s.label, [Html.text("Tasting updates")]),
        Html.button(s.root, [Html.text("Toggle")]), s.hidden_input,
      ]), (),
    ),
  ]), (),
);
```

## Public API

```reason
type parts('msg) = {
  root: list(Tea_html.Html.attr('msg)),
  legend: list(Tea_html.Html.attr('msg)),
  description: list(Tea_html.Html.attr('msg)),
  disabled: bool,
};
let parts:
  (
    ~id: string,
    ~disabled: bool=?,
    ~invalid: bool=?,
    ~has_description: bool=?,
    unit
  ) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~disabled: bool=?,
    ~invalid: bool=?,
    ~has_description: bool=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
```

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Browser navigates enabled descendants; group itself is not a tab stop. |
| Other keys | No group key handlers; descendants retain their keyboard models. |

## Behavior and limits

`disabled` defaults false. `parts.disabled` exposes exactly the supplied disabled state for custom role-based descendants; HTML cannot disable a div with role=checkbox. Native fieldset disabling propagates to native controls and hidden inputs. HTML deliberately exempts descendants of the **first legend**; do not put controls there if they must be disabled. Controls excluded by disabled fieldsets are not successful FormData entries.

Set `has_description` only when the description is rendered. `invalid` supplies a styling hook for the group, not a nonstandard group validation algorithm; individual controls own `aria-invalid`/error references. Native legend supplies the accessible group name. No required/read-only group semantics are invented.

## Styling and ownership

Presence flags: **data-disabled, data-invalid**. True emits an empty attribute; false omits
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

- jsdom: `dune build @test/ui/fieldset/runtest --force`.
- Chromium: `TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs fieldset`.
- Chromium checks real keyboard/pointer behavior, focus, exact ARIA snapshots
  and unfiltered axe with zero violations across component states.
