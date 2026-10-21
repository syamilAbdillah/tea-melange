/** Keyed HTML containers; also available as [Html.Keyed]. */
/** Construct a keyed unordered list, dropping no-op attributes. */
let ul:
  (list(Html.attr('msg)), list((string, Html.t('msg)))) => Html.t('msg);
/** Construct a keyed ordered list, dropping no-op attributes. */
let ol:
  (list(Html.attr('msg)), list((string, Html.t('msg)))) => Html.t('msg);
/** Construct a keyed div, dropping no-op attributes. */
let div:
  (list(Html.attr('msg)), list((string, Html.t('msg)))) => Html.t('msg);
