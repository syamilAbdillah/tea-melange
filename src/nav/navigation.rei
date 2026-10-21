/** Navigation targets classified relative to the current origin. */
type url_request =
  | Internal(Tea_url.Url.t)
  | External(string);
/** Push history and notify URL subscriptions. */
let push_url: string => Tea_core.Cmd.t('msg);
/** Replace history and notify URL subscriptions. */
let replace_url: string => Tea_core.Cmd.t('msg);
/** Traverse backward by a number of entries. */
let back: int => Tea_core.Cmd.t('msg);
/** Traverse forward by a number of entries. */
let forward: int => Tea_core.Cmd.t('msg);
/** Perform a full-page navigation. */
let load: string => Tea_core.Cmd.t('msg);
/** Reload the current page. */
let reload: unit => Tea_core.Cmd.t('msg);
/** Read the current HTTP(S) URL; raises Invalid_argument for other schemes. */
let current_url: unit => Tea_url.Url.t;
/** Subscribe to popstate and the library's push/replace notifications. */
let on_url_change: (Tea_url.Url.t => 'msg) => Tea_core.Sub.t('msg);
/** Delegate unmodified primary link clicks; returns an idempotent disposer.
    Download links and targets other than _self retain browser behavior. */
let intercept_links:
  (~on_url_request: url_request => unit, Dom.element, unit) => unit;
