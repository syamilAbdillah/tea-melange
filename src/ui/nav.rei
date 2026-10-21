/** Stateless named navigation landmark, not a menu/menubar.
    APG guidance: https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/examples/disclosure-navigation/
    Keyboard: Tab/Shift+Tab traverse native links; Enter follows a link.
    Space/arrows/Home/End retain browser behavior. No keys are intercepted.
    Data attributes: data-current on the unique current link.
    Owns no events. Caller renders a ul/ol with li and native anchors with href. */

/** Required landmark naming strategy. */
type name =
  | /** Nonempty aria-label; whitespace-only raises Invalid_argument. */
    Label(
      string,
    )
  | /** Render the provided label part with nonempty text. */
    Labelledby;
/** Permitted aria-current tokens. */
type current_token =
  | /** Current page (default). */
    Page
  | /** Current step. */
    Step
  | /** Current location. */
    Location
  | /** Current date. */
    Date
  | /** Current time. */
    Time
  | /** Generic current item. */
    True;
/** One stable-key link. Render each item once. */
type item('msg) = {
  /** Caller-supplied identity, not position. */
  key: string,
  /** Apply to a native anchor; append href. */
  link: list(Tea_html.Html.attr('msg)),
};
/** Message-typed attributes. */
type parts('msg) = {
  /** Apply once to nav when using [parts]; [view] applies it automatically. */
  root: list(Tea_html.Html.attr('msg)),
  /** Visible landmark label; required for Labelledby. */
  label: list(Tea_html.Html.attr('msg)),
  /** Apply to ul/ol containing li elements. */
  list: list(Tea_html.Html.attr('msg)),
  /** Fresh collection in the order of keys. */
  items: array(item('msg)),
};
/** Build bundles; duplicate keys raise Invalid_argument so at most one item
    can be current. Unknown/absent current marks none. Empty keys array is valid.
    Keys may contain any string; derived ids are collision-free and sanitized.
    Do not duplicate link bundles or override owned state/id attributes. */
let parts:
  (
    ~id: string,
    ~name: name,
    ~keys: array(string),
    ~current: string=?,
    ~current_token: current_token=?,
    unit
  ) =>
  parts('msg);
/** Render a native nav wrapper with root attributes. Callback renders its
    contents (label/list/links), not another nav or another copy of root. */
let view:
  (
    ~id: string,
    ~name: name,
    ~keys: array(string),
    ~current: string=?,
    ~current_token: current_token=?,
    ~to_view: parts('msg) => Tea_html.Html.t('msg),
    unit
  ) =>
  Tea_html.Html.t('msg);
