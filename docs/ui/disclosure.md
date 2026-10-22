# Disclosure

[APG disclosure](https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/).
Parent-owned open state, native button activation, and a panel hidden without
CSS. The public label is `~open_` because `open` is a Reason keyword.

```reason
Ui.Disclosure.view(
  ~id="details", ~open_=model.details_open, ~on_toggle=Toggle_details,
  ~to_view=p => Html.div([], [
    Html.button(p.button, [Html.text("Details")]),
    p.mounted
      ? Html.div(p.panel, [Html.text("More information")])
      : Html.text(""),
  ]),
  (),
);
```

In update, Toggle_details **toggles the latest parent model**. Never construct
an on_toggle message containing `!model.open_` in the view: two clicks before
a draw must toggle twice. The helper never captures a next-open value.

## Mounting and ARIA

| State | Panel | aria-expanded | aria-controls |
| --- | --- | --- | --- |
| open | mounted, hidden=false | true | panel id |
| closed, keep_mounted=true (default) | mounted, hidden=true | false | panel id |
| closed, keep_mounted=false | omitted by caller | false | omitted |

Button must be a **native button**, uses type=button, and owns click/keydown/
keyup through Button. It must have a nonempty accessible name. Do not override
owned events/semantics. Panel owns derived id and the boolean hidden property,
not a string "false". Callback **must respect mounted**; if false, do not render
the panel. Controls is removed in that state so no idref dangles.
Default keep-mounted preserves panel DOM/local input values while hidden.
Unmounting does not preserve local DOM state; place important state in the parent.

Both parts carry exactly one of presence `data-open=""` or `data-closed=""`.
Neither CSS nor animation is needed for hiding. Do not override hidden with
display CSS. There are no document listeners, subscriptions or focus effects.
Pointer/keyboard activation retains native button focus. Before programmatically
closing a panel containing focus, the parent should explicitly focus its button
(e.g. Browser_dom.focus_cmd on the derived `id-button`) to avoid hiding focus.

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Native navigation, skipping closed panel contents |
| Enter | Native button click toggles |
| Space down | Native button prevents scroll; no toggle yet |
| Space up | Native button click toggles |
| Other keys, shortcuts, IME | Unmodified native button behavior |

Synthetic key events do not simulate native clicks in jsdom; Chromium tests
verify the actual Enter/Space model. No custom key handler claims shortcut keys.

## Accordion composition

The example shows an [APG accordion](https://www.w3.org/WAI/ARIA/apg/patterns/accordion/):
parent stores an optional stable active key; toggling a different disclosure
closes the old one and opens the new one. Toggling the active one closes it.
Wrap each disclosure button **alone** in a native heading. The **user's markup
chooses heading level** appropriate to the page (the example uses h3 under h2).
Keep accordion panels mounted so every header's aria-controls remains valid.

Optional region/aria-labelledby on panels may be supplied by the application;
avoid landmark proliferation. Optional arrow/Home/End navigation is not
implemented. All headers and open-panel controls remain in ordinary Tab order.
Mandatory-one-open accordions (requiring aria-disabled on the uncollapsible
header) are outside this helper's contract; zero-or-one-open is supported.

## Full public API

```reason
type parts('msg) = {
  button: list(Tea_html.Html.attr('msg)),
  panel: list(Tea_html.Html.attr('msg)),
  mounted: bool,
};
let parts:
  (~id: string, ~open_: bool, ~on_toggle: 'msg, ~keep_mounted: bool=?, unit) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~open_: bool,
    ~on_toggle: 'msg,
    ~keep_mounted: bool=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
```

## Scope and verification

No disabled, read-only or RTL options apply to this native disclosure helper.
No private browser capabilities, global listeners, locks or dependencies on CSS.
Parts are typed in the parent message; there is no submodel or to_parent mapping.
Caller-provided labels, hrefs and nonconflicting markup remain the caller's responsibility.

Run `dune build @test/ui/disclosure/runtest --force` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs disclosure`.
The standalone example is `examples/ui/disclosure/`. jsdom mounts a real Tea app,
checks normalized state/ARIA, rapid parent changes and listener-free shutdown.
Chromium checks the keyboard table, pointer use, focus, exact ARIA snapshots
and unfiltered axe with zero violations across states. No assistive-technology
or Safari/device compatibility claim is implied by Chromium testing.
