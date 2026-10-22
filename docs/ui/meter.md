# Meter

[APG meter](https://www.w3.org/WAI/ARIA/apg/patterns/meter/).
A read-only bounded measurement, not task progress. A non-native div/span with
role=meter allows unrestricted track/fill styling without browser-specific
native `<meter>` pseudo-elements. It retains the APG semantics; prefer a native
meter if you do not need custom rendering.

```reason
Ui.Meter.view(
  ~id="storage", ~value=42., ~min=0., ~max=100.,
  ~low=25., ~high=75., ~optimum=50.,
  ~format=value => Js.Float.toString(value) ++ " GB",
  ~to_view=p => Html.div([], [
    Html.span(p.label, [Html.text("Storage used")]),
    Html.div(p.root, []),
  ]),
  (),
);
```

## Numeric contract and thresholds

Defaults: min=0, max=1; low=min, high=max, optimum=(min+max)/2.
Bounds must be finite and strictly increasing, otherwise `Invalid_argument`.
This includes min>max as well as min=max: APG explicitly requires min<max.
Unlike HTML meter's repair of malformed bounds, invalid configuration is rejected.

NaN value becomes min. Negative/positive infinity and out-of-range values clamp
to min/max. The formatter receives this clamped value. Missing formatter omits
aria-valuetext. Finite endpoints that span more than the largest float use
half-scaled arithmetic for the percentage; no NaN/Infinity leaks to ARIA or CSS.

Threshold normalization mirrors the [HTML meter algorithm](https://html.spec.whatwg.org/multipage/form-elements.html#the-meter-element):
nonfinite low/high/optimum are treated as absent (HTML cannot parse them as
valid floating-point attribute values). Low clamps to [min,max], high to
[low,max], optimum to [min,max]. Normalize in that order.

| Optimum position | Optimum values | Suboptimal values | Critical values |
| --- | --- | --- | --- |
| optimum < low | value <= low | low < value <= high | value > high |
| optimum > high | value >= high | low <= value < high | value < low |
| low <= optimum <= high | low <= value <= high | value < low or value > high | none |

Boundaries are inclusive as shown, including low=high and optimum=low/high.
No supplied thresholds means the entire range is optimum.

## Keyboard

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Skip the meter; it is not focusable |
| All other keys, pointer | No interaction |

## ARIA and styling

Root owns id, role=meter, aria-valuemin/max/now, optional aria-valuetext and
aria-labelledby pointing to the label part by default (`~has_label=true`).
This default requires nonempty label text **outside** root: range-role descendants
are presentational. If there is no visible label, pass `~has_label=false` and
`~aria_label="Storage used"`; this omits aria-labelledby and names the meter
without a label element. Missing aria_label in that mode raises Invalid_argument.
A supplied aria_label is trimmed with JavaScript trim and must be nonblank in
either mode; empty/whitespace-only names raise Invalid_argument. When both are
present, the visible label takes precedence. The helper cannot inspect callback
markup: has_label must match actual label presence. Do not put interactive
content inside root or override owned attributes. There are no event handlers.

`data-state="optimum|suboptimal|critical"` is the normalized threshold state.
`--tea-meter-percent` contains a CSS percentage, e.g. `42%`; use
`width: var(--tea-meter-percent)` on a pseudo-element or fill child.
The example shows both a custom 0–100 range and default 0–1 range.

## Full public API

```reason
type parts('msg) = {
  root: list(Tea_html.Html.attr('msg)),
  label: list(Tea_html.Html.attr('msg)),
  value: float,
  percent: float,
  state: string,
};
let parts:
  (
    ~id: string,
    ~value: float,
    ~has_label: bool=?,
    ~aria_label: string=?,
    ~min: float=?,
    ~max: float=?,
    ~low: float=?,
    ~high: float=?,
    ~optimum: float=?,
    ~format: float => string=?,
    unit
  ) =>
  parts('msg);
let view:
  (
    ~id: string,
    ~value: float,
    ~has_label: bool=?,
    ~aria_label: string=?,
    ~min: float=?,
    ~max: float=?,
    ~low: float=?,
    ~high: float=?,
    ~optimum: float=?,
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

Run `dune build @test/ui/meter/runtest --force` and
`TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs meter`.
The standalone example is `examples/ui/meter/`. jsdom mounts a real Tea app,
checks normalized state/ARIA, rapid parent changes and listener-free shutdown.
Chromium checks the keyboard table, pointer use, focus, exact ARIA snapshots
and unfiltered axe with zero violations across states. No assistive-technology
or Safari/device compatibility claim is implied by Chromium testing.
