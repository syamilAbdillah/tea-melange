# Progress

[WAI-ARIA progressbar](https://www.w3.org/TR/wai-aria-1.2/#progressbar).
There is no dedicated APG progressbar pattern and no interactive keyboard model.
Use this for task completion, not a meter. A neutral div/span allows a fully
custom track. The parent supplies each new reading.

```reason
Ui.Progress.view(
  ~id="import", ~value=42.,
  ~format=value => Js.Float.toString(value) ++ " records",
  ~to_view=p => Html.div([], [
    Html.span(p.label, [Html.text("Import progress")]),
    Html.div(p.root, []),
    Html.div(p.region, [Html.text("Imported content")]),
  ]),
  (),
);
```

Omit `~value` for indeterminate progress. Defaults: min=0, max=100.
Bounds must be finite with min<max; invalid/reversed/equal bounds raise
`Invalid_argument`. NaN value becomes min, infinities/out-of-range clamp to
the finite endpoints. Formatter receives the clamped value only in determinate
mode. Percent arithmetic remains finite even for opposite large endpoints.

## States, ARIA and styling

| State | Condition | aria-valuenow | aria-busy on region | CSS percentage |
| --- | --- | --- | --- | --- |
| indeterminate | value absent | omitted | true | omitted |
| loading | value < max | clamped value | true | percentage |
| complete | value = max | max | false | 100% |

Root always owns role=progressbar, id, aria-valuemin/max and aria-labelledby.
Determinate mode has optional formatted aria-valuetext; indeterminate mode
omits both valuenow and valuetext. Label text must be nonempty and rendered
outside root (descendants of range roles are presentational).
`data-state="indeterminate|loading|complete"` and
`--tea-progress-percent` (e.g. `42%`) are styling hooks. The custom property
is removed, not set to zero, for indeterminate progress.

Apply optional `region` to the content being loaded: aria-busy stays true
until complete and aria-describedby references this progressbar. If describing
the region separately, use `busy(loading)` alone. No live region is installed:
choose application-appropriate announcements; do not announce every frame.

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Skip the progressbar; not focusable |
| Other keys, pointer | No interaction |

No owned DOM events, focus changes, global listeners or subscriptions.
The example demonstrates all three states, formatting, busy region and
numeric normalization.

## Full public API

```reason
type parts('msg) = {
  root: list(Tea_html.Html.attr('msg)),
  label: list(Tea_html.Html.attr('msg)),
  region: list(Tea_html.Html.attr('msg)),
  value: option(float),
  percent: option(float),
  state: string,
};
let busy: bool => Tea_html.Html.attr('msg);
let parts:
  (
    ~id: string,
    ~value: float=?,
    ~min: float=?,
    ~max: float=?,
    ~format: float => string=?,
    unit
  ) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~value: float=?,
    ~min: float=?,
    ~max: float=?,
    ~format: float => string=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
```

## Scope and verification

No disabled, read-only or RTL options apply to this noninteractive display helper.
No private browser capabilities, global listeners, locks or dependencies on CSS.
Parts are typed in the parent message; there is no submodel or to_parent mapping.
Caller-provided labels, hrefs and nonconflicting markup remain the caller's responsibility.

Run `dune build @test/ui/progress/runtest --force` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs progress`.
The standalone example is `examples/ui/progress/`. jsdom mounts a real Tea app,
checks normalized state/ARIA, rapid parent changes and listener-free shutdown.
Chromium checks the keyboard table, pointer use, focus, exact ARIA snapshots
and unfiltered axe with zero violations across states. No assistive-technology
or Safari/device compatibility claim is implied by Chromium testing.
