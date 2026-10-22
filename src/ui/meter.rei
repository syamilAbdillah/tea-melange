/** Stateless meter; parent owns the value.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/meter/
    Keyboard: none; not focusable. Tab/Shift+Tab skip the display.
    Data attributes: data-state="optimum|suboptimal|critical".
    CSS custom property --tea-meter-percent is a percentage.
    Apply root to a non-native div/span for unrestricted styling, not <meter>.
    Render a nonempty label outside root (range-role descendants are
    presentational), or set has_label=false and supply a nonblank aria_label. */

/** Message-typed attributes and normalized derived state. */
type parts('msg) = {
  /** Apply to a neutral noninteractive element; owns id, role, ARIA, state and percentage. */
  root: list(Tea_html.Html.attr('msg)),
  /** Apply to nonempty visible label text; required when has_label=true. */
  label: list(Tea_html.Html.attr('msg)),
  /** Clamped finite value. */
  value: float,
  /** Percentage in [0,100]. */
  percent: float,
  /** HTML threshold classification. */
  state: string,
};

/** Build attributes. Defaults: min=0, max=1, has_label=true.
    has_label=true requires rendering the label part with nonempty text outside
    root. Set has_label=false to omit aria-labelledby; then aria_label is required.
    aria_label is trimmed and, when supplied, must be nonblank (Invalid_argument
    otherwise). A visible label takes precedence when both are supplied.
    Caller markup is not inspected; has_label must match actual label presence.
    Raises Invalid_argument unless bounds are finite and min < max, including
    equal bounds (APG range invariant). NaN value becomes min; infinities clamp.
    Formatter receives the clamped value.
    Nonfinite thresholds act as absent. low defaults to min and clamps to
    [min,max]; high defaults to max and clamps to [low,max]; optimum defaults to
    the midpoint and clamps to [min,max]. If optimum < low: value <= low is
    optimum, low < value <= high suboptimal, above high critical. If optimum >
    high: value >= high optimum, low <= value < high suboptimal, below low
    critical. Otherwise [low,high] is optimum and outside suboptimal. Equality
    at low/high therefore belongs to the middle band when optimum is inside. */
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
/** Render through caller's parts callback; owns no DOM events.
    Use the same contract and defaults as [parts]. */
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
