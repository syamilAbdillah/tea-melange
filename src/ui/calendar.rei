/** Single-selection civil-date grid.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/examples/datepicker-dialog/

    Keyboard table: Left/Right move one day (swapped in RTL); Up/Down move
    -/+7 days; Home/End move to the first/last day of the configured week;
    PageUp/PageDown move -/+1 month; Shift+PageUp/PageDown move -/+1 year,
    clamping the day at month end. Enter/Space select unless unavailable or
    read-only. Tab/Shift+Tab remain native, with one grid tab stop. Native
    month/year buttons activate with Enter/Space. Ctrl/Alt/Meta and IME are
    untouched. Focus movement saturates at 0001-01-01 and 9999-12-31.

    Data presence flags: data-active, data-selected, data-today, data-disabled,
    data-outside-month on date cells; data-disabled on navigation buttons;
    data-readonly on the grid. No CSS is needed for behavior.
    Apply cell attrs to td, weekday attrs to th, grid attrs to table, heading
    attrs to a heading, and navigation attrs to native buttons. Date cells own
    click and keydown (compose unhandled keys with on_key_down). Navigation
    buttons own click; do not override these.
    This is the grid only: dialog closing, Escape, focus return and keyboard
    instructions belong to the parent Date_picker. */

/** Pure proleptic Gregorian arithmetic. No clock, timezone, or JS Date. */
module Date: {
  /** Valid date in years 1 through 9999 inclusive. */
  type t;
  /** Construct, rejecting invalid year, month and day. */
  let make: (~year: int, ~month: int, ~day: int) => option(t);
  /** First supported date, 0001-01-01. */
  let first: t;
  /** Last supported date, 9999-12-31. */
  let last: t;
  /** Gregorian year. */
  let year: t => int;
  /** Month, 1 through 12. */
  let month: t => int;
  /** Day, 1 through 31. */
  let day: t => int;
  /** Gregorian leap-year rule. */
  let leap_year: int => bool;
  /** Month length; invalid months return zero. */
  let days_in_month: (~year: int, ~month: int) => int;
  /** Chronological ordering: negative, zero, positive. */
  let compare: (t, t) => int;
  /** Civil-date equality. */
  let equal: (t, t) => bool;
  /** Add days, saturating at domain bounds, independent of DST. */
  let add_days: (t, int) => t;
  /** Add months, saturating the target month then clamping day to its length. */
  let add_months: (t, int) => t;
  /** Add years, saturating year then clamping day (February 29 to 28). */
  let add_years: (t, int) => t;
  /** Sunday=0 through Saturday=6. */
  let day_of_week: t => int;
  /** Strict ASCII YYYY-MM-DD parser, rejecting whitespace and invalid dates. */
  let of_iso: string => option(t);
  /** Canonical ten-character YYYY-MM-DD. */
  let to_iso: t => string;
  /** Seven-cell weeks, six by default, or the minimal 4–6 with fixed_weeks=false.
      first_day_of_week defaults to Sunday and is normalized modulo seven.
      Includes adjacent months; None represents dates beyond the domain bounds. */
  let month_grid:
    (~first_day_of_week: int=?, ~fixed_weeks: bool=?, t) =>
    array(array(option(t)));
};
/** Calendar civil date. */
type date = Date.t;
/** Horizontal keyboard direction. */
type direction =
  | Ltr
  | Rtl;
/** Parent-owned constraints, passed consistently to update and view. Dates
    outside inclusive bounds or unavailable remain focusable, not selectable.
    Reversed bounds disable all selection. Existing selection is retained until
    explicitly replaced, even when constraints change. */
type constraints = {
  /** Inclusive lower selection bound; None means domain start. */
  min: option(date),
  /** Inclusive upper selection bound; None means domain end. */
  max: option(date),
  /** Parent's pure unavailable-date predicate. */
  is_unavailable: date => bool,
};
/** No bounds and no unavailable dates. */
let unconstrained: constraints;
/** Localized strings/functions, supplied by the parent; no Intl dependency. */
type labels = {
  /** Month name, called with 1–12. */
  month: int => string,
  /** Full weekday name, called with Sunday=0 through Saturday=6. */
  weekday: int => string,
  /** Visible abbreviated weekday, same numbering. */
  weekday_short: int => string,
  /** Previous month button accessible name. */
  previous_month: string,
  /** Next month button accessible name. */
  next_month: string,
  /** Previous year button accessible name. */
  previous_year: string,
  /** Next year button accessible name. */
  next_year: string,
  /** Full accessible date-cell name. Include the unpadded visible day number
      as a separate word to satisfy WCAG Label in Name. */
  date: date => string,
};
/** English month/weekday/button labels; date cells read e.g. 1 January 2024. */
let english: labels;
/** Opaque single-selection submodel. Collections and constraints are not stored. */
type model;
/** Opaque event intent, evaluated against the latest submodel. */
type msg;
/** User events; programmatic setters emit none. */
type out_msg =
  | /** Successful selection, including reselecting the same date. */
    Changed(
      date,
    )
  | /** Focus date changed (including browsing without DOM focus movement). */
    Focused(
      date,
    )
  | /** Visible month changed; value is its first day. */
    Month_changed(date);
/** Initialize. focus is required: parent chooses selected date, today or another
    fallback. Selection is initially absent. Defaults: Sunday-first, Ltr, six
    weeks, blank outside-month cells. id must be document-unique. */
let init:
  (
    ~id: string,
    ~focus: date,
    ~selected: date=?,
    ~first_day_of_week: int=?,
    ~dir: direction=?,
    ~fixed_weeks: bool=?,
    ~show_outside_month: bool=?,
    unit
  ) =>
  model;
/** Current roving date. */
let focus_date: model => date;
/** Current selection. */
let selected: model => option(date);
/** First day of visible month. */
let visible_month: model => date;
/** Replace/clear selection without out messages or DOM effects. */
let set_selected: (option(date), model) => model;
/** Replace focus date and visible month without DOM effects/out messages,
    cancelling a previously pending focus request.
    Pair with focus_grid if DOM focus should move after rendering. */
let set_focus: (date, model) => model;
/** Request post-render focus of the current roving cell through update and
    subscriptions, ignoring a missing grid. Intervening focus movement, a newer
    request or shutdown cancels it. Always forward returned commands to update
    and map subscriptions into the parent. */
let focus_grid: model => Tea_core.Cmd.t(msg);
/** Update against current constraints/read-only state. Keyboard can reach
    unavailable/out-of-bound dates. Navigation buttons stop at min/max months;
    they retain DOM focus, but update the grid's roving date and heading.
    Navigation uses aria-disabled and tabindex=-1 at bounds, not native disabled,
    so a button becoming disabled never drops existing focus onto the body;
    disabled buttons cannot activate and are skipped by Tab.
    Stale clicks on dates no longer rendered are ignored. */
let update:
  (~constraints: constraints=?, ~readonly: bool=?, msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Forward while mounted. Moves focus after render, including for
    [focus_grid], and yields to focus the user moved in the meantime. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** A td bundle. Blank cells have no date, events, id or tabindex. */
type cell('msg) = {
  /** Displayed civil date, None for blank placeholders. */
  date: option(date),
  /** Visible day number or empty string. */
  text: string,
  /** Apply to td, never a nested button. Owns click and keydown. */
  attrs: list(Tea_html.Html.attr('msg)),
};
/** A column header. */
type weekday('msg) = {
  /** Abbreviated weekday text. */
  text: string,
  /** Apply to th; includes scope, abbr and full aria-label. */
  attrs: list(Tea_html.Html.attr('msg)),
};
/** Parent-message-typed, wrapper-free parts. Keep a stable table/heading DOM
    position when rendering, and use one tr per week. Render every placeholder
    td to preserve seven-column alignment. Year buttons are optional to render. */
type parts('msg) = {
  /** Outer container, id and dir. */
  root: list(Tea_html.Html.attr('msg)),
  /** Heading id, polite atomic live region. */
  heading: list(Tea_html.Html.attr('msg)),
  /** Localized month and numeric year. */
  heading_text: string,
  /** Table role=grid, labelledby heading, readonly state. */
  grid: list(Tea_html.Html.attr('msg)),
  /** Native previous month button; owns click. */
  previous_month: list(Tea_html.Html.attr('msg)),
  /** Native next month button; owns click. */
  next_month: list(Tea_html.Html.attr('msg)),
  /** Optional native previous year button; owns click. */
  previous_year: list(Tea_html.Html.attr('msg)),
  /** Optional native next year button; owns click. */
  next_year: list(Tea_html.Html.attr('msg)),
  /** Seven column headers in week order. */
  weekdays: array(weekday('msg)),
  /** Four to six seven-cell rows, depending on fixed_weeks. */
  weeks: array(array(cell('msg))),
};
/** Render via parts. today is supplied by the parent, never read from a clock.
    readonly defaults to false, labels to english, constraints to unconstrained.
    Use the same constraints/readonly as update. Exactly one rendered date has
    tabindex=0, including when every date is unavailable. No range selection.
    on_key_down receives unhandled plain keys (including Escape and Tab), emits
    parent messages without preventing default, and never receives IME events.
    Required grid behavior takes precedence over this hook. */
let view:
  (
    ~model: model,
    ~constraints: constraints=?,
    ~readonly: bool=?,
    ~today: date=?,
    ~labels: labels=?,
    ~on_key_down: string => option('parent)=?,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
