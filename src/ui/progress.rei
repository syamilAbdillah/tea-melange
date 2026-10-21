/** Stateless progress; parent owns the value.
    ARIA (no dedicated APG pattern): https://www.w3.org/TR/wai-aria-1.2/#progressbar
    Keyboard: none; not focusable. Tab/Shift+Tab skip the display.
    Data attributes: data-state="indeterminate|loading|complete".
    CSS custom property --tea-progress-percent is a percentage; absent when indeterminate.
    Apply root to a non-native div/span for unrestricted styling, not <progress>.
    Render label outside root (range-role descendants are presentational). */

/** Message-typed attributes and normalized derived state. */
type parts('msg) = {
  /** Apply to a neutral noninteractive element; owns id, role, ARIA, state and percentage. */
  root: list(Tea_html.Html.attr('msg)),
  /** Apply to visible label text; always render this part. */
  label: list(Tea_html.Html.attr('msg)),
  /** Optional loaded-region attributes: aria-busy and aria-describedby=root id. */
  region: list(Tea_html.Html.attr('msg)),
  /** Clamped value, absent when indeterminate. */
  value: option(float),
  /** Percentage, absent when indeterminate. */
  percent: option(float),
  /** Indeterminate, loading or complete. */
  state: string,
};
/** Explicit aria-busy attribute for a loaded region; false clears busy. */
let busy: bool => Tea_html.Html.attr('msg);
/** Build attributes. Defaults: min=0, max=100; omitted value is indeterminate.
    Raises Invalid_argument unless bounds are finite and min < max, including
    equal bounds (APG range invariant). NaN value becomes min; infinities clamp.
    Formatter receives the clamped value only in determinate mode.
    Indeterminate mode omits valuenow, valuetext and the CSS percentage. Region busy is true until complete. */
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
/** Render through caller's parts callback; owns no DOM events.
    Use the same contract and defaults as [parts]. */
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
