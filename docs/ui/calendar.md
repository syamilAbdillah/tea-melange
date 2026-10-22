# Calendar

A headless, single-selection civil-date grid following the
[APG Date Picker Dialog date grid](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/examples/datepicker-dialog/).
It is not a dialog: the parent owns opening, closing, Escape, focus return and
keyboard-help announcements. Selection emits `Changed(date)`, allowing a future
Date_picker to close its dialog and update its input.

## Usage and integration

```reason
open Tea;
module Calendar = Ui.Calendar;
let initial_focus =
  Option.get(Calendar.Date.of_iso("2024-01-31"));
let calendar =
  Calendar.init(
    ~id="appointment",
    ~focus=initial_focus,
    ~first_day_of_week=1,
    (),
  );
let limits = {
  Calendar.min: Calendar.Date.of_iso("2024-01-01"),
  max: Calendar.Date.of_iso("2025-12-31"),
  is_unavailable: d =>
    Calendar.Date.day_of_week(d) == 0
    || Calendar.Date.day_of_week(d) == 6,
};
```

In the parent's update, call
`Calendar.update(~constraints=limits, message, calendar)`, store the returned
submodel, map the command through the parent's Calendar message constructor,
and handle its out messages. Map `Calendar.subscriptions(calendar)` similarly
(it currently returns `Sub.none`).

Render with
`Calendar.view(~model=calendar, ~constraints=limits, ~today=initial_focus,
~to_parent, ~to_view, ())`. Apply `root` to the container, `heading` to a
heading with `heading_text`, `grid` to a table, `weekdays` to th elements
within thead/tr, and `weeks` to td elements within tbody/tr. Render each cell's
`text`, including empty placeholders. Apply navigation attributes to native
buttons with visible content. Year buttons may be omitted.
See the [complete runnable renderer](../../examples/ui/calendar/main.re).

Keep the table and live heading at stable DOM positions. All bundles are typed
in the parent's message type; no VDOM mapping around the callback is needed.
Cells own click and keydown; navigation owns click. Append only nonconflicting
attributes. Use `on_key_down` to compose unhandled plain keys (e.g. Escape);
the hook returns parent messages and never prevents default, including on Tab.
Browser/AT shortcuts and IME do not invoke it. Do not nest buttons in date cells.

`set_selected` replaces or clears the domain value without out messages.
`set_focus` replaces the roving date and visible month without DOM effects;
return `focus_grid(next_model)` as a command to request focus after the new grid
draws. Forward the command back through update and map `Calendar.subscriptions`.
Requests target the current roving cell and yield to any intervening focus move;
newer requests, a setter, unmount or shutdown cancel obsolete work. Initial focus is explicit: the
parent chooses selected date, today, or another fallback. Nothing reads a clock.

## Public API

Every signature and public record field:

```reason
module Date: {
  type t;
  let make: (~year: int, ~month: int, ~day: int) => option(t);
  let first: t;
  let last: t;
  let year: t => int;
  let month: t => int;
  let day: t => int;
  let leap_year: int => bool;
  let days_in_month: (~year: int, ~month: int) => int;
  let compare: (t, t) => int;
  let equal: (t, t) => bool;
  let add_days: (t, int) => t;
  let add_months: (t, int) => t;
  let add_years: (t, int) => t;
  let day_of_week: t => int;
  let of_iso: string => option(t);
  let to_iso: t => string;
  let month_grid:
    (~first_day_of_week: int=?, ~fixed_weeks: bool=?, t) =>
    array(array(option(t)));
};
type date = Date.t;
type direction =
  | Ltr
  | Rtl;
type constraints = {
  min: option(date),
  max: option(date),
  is_unavailable: date => bool,
};
let unconstrained: constraints;
type labels = {
  month: int => string,
  weekday: int => string,
  weekday_short: int => string,
  previous_month: string,
  next_month: string,
  previous_year: string,
  next_year: string,
  date: date => string,
};
let english: labels;
type model;
type msg;
type out_msg = Changed(date) | Focused(date) | Month_changed(date);
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
let focus_date: model => date;
let selected: model => option(date);
let visible_month: model => date;
let set_selected: (option(date), model) => model;
let set_focus: (date, model) => model;
let focus_grid: model => Tea_core.Cmd.t(msg);
let update:
  (~constraints: constraints=?, ~readonly: bool=?, msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
type cell('msg) = {
  date: option(date),
  text: string,
  attrs: list(Tea_html.Html.attr('msg)),
};
type weekday('msg) = {
  text: string,
  attrs: list(Tea_html.Html.attr('msg)),
};
type parts('msg) = {
  root: list(Tea_html.Html.attr('msg)),
  heading: list(Tea_html.Html.attr('msg)),
  heading_text: string,
  grid: list(Tea_html.Html.attr('msg)),
  previous_month: list(Tea_html.Html.attr('msg)),
  next_month: list(Tea_html.Html.attr('msg)),
  previous_year: list(Tea_html.Html.attr('msg)),
  next_year: list(Tea_html.Html.attr('msg)),
  weekdays: array(weekday('msg)),
  weeks: array(array(cell('msg))),
};
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
```

### Date domain and arithmetic

`Date.t` is validated and abstract, with Gregorian years 1–9999, months 1–12
and days 1–31. `Date.make`/`of_iso` return None on invalid input.
ISO parsing is strictly ten ASCII characters; printing always yields YYYY-MM-DD.
No year zero, expanded ISO years, leap seconds, timezones, JS Date or timestamps.
`leap_year` applies Gregorian divisibility to an integer;
`days_in_month` returns zero for invalid months (it does not validate the year).

Day additions saturate at the first/last supported date. Month/year additions
first saturate the target month/year, then clamp the day to its month length.
Thus January 31 + 1 month is February 28/29; January 31 - an enormous number
of months becomes January 31 in year 1, not January 1.
Operations accept negative and large signed integer offsets without overflow
from adding the offset before clamping. Repeated clamped operations are not
invertible (January 31 → February 29 → March 29).

`month_grid` defaults to six seven-cell weeks. With `fixed_weeks=false`, it
returns the minimum four to six weeks. Week starts normalize modulo seven.
Adjacent-month dates are included in this pure grid; None only represents
padding beyond years 1–9999. The view separately blanks outside-month dates
unless `show_outside_month=true`. Blanks have no tabindex, events or date id,
and cannot be selected. Displayed dates use ISO keys and collision-free
`Ui_attr.item_id` ids, never collection indices.

### State, constraints and localization

Defaults: absent selection, Sunday-first, Ltr, six weeks, outside-month dates
blank, no constraints, English labels, editable. `today` is an optional
parent-supplied civil date and affects only current-date semantics.

Pass the same current `constraints` and `readonly` to update and view.
Inclusive min/max and unavailable dates prevent selection, **not focus**.
Arrows and paging can reach them without skipping; all-unavailable and reversed
bounds still have exactly one grid Tab stop. Existing selection is retained
when constraints change, until the parent explicitly replaces it. Read-only
allows navigation without selection.

Navigation buttons cannot move farther past the corresponding min/max month
or the civil-date domain bounds. They may browse back toward valid dates when
keyboard focus has moved beyond those bounds. Year buttons are disabled if
their target month would cross the corresponding bound. They do not search
for an available day. They use aria-disabled plus tabindex=-1 rather than the
native disabled property, preventing body-focus loss when a focused button
becomes disabled. Disabled navigation clicks and native Enter/Space activation
are guarded, including batched/stale events. Tab skips disabled navigation.

User focus changes emit `Focused`, followed by `Month_changed` if needed;
selection then emits `Changed`, including a successful re-selection.
Month_changed carries the month's first date. Browsing updates the roving date
without moving DOM focus off the navigation button. Programmatic setters emit
nothing. Key messages carry intents and update against the latest model, so
rapid events before a frame accumulate correctly. Stale clicks on dates that
are no longer rendered are ignored.

Supply all localization through `labels` functions/strings. Weekday numbering
is always Sunday=0, month numbering January=1, independent of week start.
The date accessible name must contain the visible unpadded day as a word
(WCAG Label in Name). The heading combines the localized month and numeric year.
The parent owns locale policy; Intl label generation is not implemented.

## Keyboard

| Key | Behavior |
| --- | --- |
| Left / Right | Previous / next day; reversed in RTL |
| Up / Down | Previous / next week (−7 / +7 days) |
| Home / End | First / last day of current week, honoring configured week start |
| PageUp / PageDown | Previous / next month; clamp missing day to month end |
| Shift+PageUp / Shift+PageDown | Previous / next year; clamp February 29 if needed |
| Enter / Space on date | Select once on keydown unless read-only, unavailable or outside bounds; prevent default |
| Enter / Space on navigation | Native button activation; browse without taking focus from the button |
| Tab / Shift+Tab | Native traversal, one Tab stop inside each grid; never prevented |
| Ctrl / Alt / Meta chords, IME | Untouched |
| Other plain keys | Untouched; optional parent hook, never prevents default |

Date focus saturates at the supported domain bounds. Focus moving across a
month boundary changes the visible month and its live heading even when the
destination cannot be selected. RTL changes horizontal arrows, not chronology
or Home/End's logical week edges.

## Semantics and styling

- Table role=grid, aria-labelledby resolving to the heading; aria-readonly.
- Heading has aria-live=polite and aria-atomic=true.
- th has scope=col, abbr=full weekday and aria-label=full weekday.
- td role=gridcell; one rendered date has tabindex=0, all others -1.
- Only the selected date has aria-selected=true. Today has aria-current=date.
- Unavailable/out-of-bound dates have aria-disabled=true but remain focusable.
- Navigation buttons have localized aria-labels and type=button.
- Presence flags: data-active, data-selected, data-today, data-disabled,
  data-outside-month on date cells; data-disabled on bounded navigation;
  data-readonly on the grid.
- No inline styles or scroll locks. Focus uses generation-keyed
  `Browser_events.focus` subscriptions with temporary focus listeners; always
  forward `Calendar.subscriptions`. Behavior needs no CSS.

## Scope and verification

Single selection only: no partially implemented range or multiselect mode.
No Today task or Intl externals; no private browser capability additions.
No modal behavior, Escape closing, focus trap/return, input parsing UX or delayed
keyboard-help live region: those belong to the Date_picker/dialog composition.
The parent must render the bundles as documented and keep ids unique.
Chromium/jsdom coverage does not claim manual screen-reader or Safari/device
validation.

The example is Monday-first with min/max, weekends unavailable, a fixed
reproducible today, editable/read-only/all-unavailable/single-day controls and an
RTL instance with variable weeks and visible adjacent-month days.
The tests cover every keyboard row, pointer and native Tab traversal,
announcements, focus retention at bounds, rapid intent events, stale clicks,
selected/current/disabled/read-only semantics, localization, cleanup and
unfiltered axe/ARIA snapshots. Pure tests exhaust all 3,652,059 supported dates
and all week starts across a complete 400-year Gregorian layout cycle.

```sh
dune build @test/ui/calendar/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs calendar
```
