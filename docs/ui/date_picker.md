# Date picker

An editable civil-date input with an anchored Calendar dialog, following the
[APG Date Picker Dialog](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/examples/datepicker-dialog/).
No JS Date, Intl, timezone conversion, or device clock is involved. The parent
supplies today's/fallback date and can replace ISO parsing and formatting.

## Composition and modality

Date_picker embeds **Calendar and Popover**, retaining both immutable submodels
through openings and reversals. Popover supplies Anchor (manual native popover,
top layer) and Animation. Its trigger is the Choose date button in the input
group; Popover's `anchor_id` targets the `group` bundle's id so Bottom/Start
aligns the popup with the whole input group, not the button. The popup remains
the trigger's immediate DOM sibling. Top-layer promotion does not portal or rearrange nodes.

Popover is configured with no role or opening focus and with focus-out dismissal
disabled. Date_picker adds role=dialog, aria-modal=true, and aria-labelledby
pointing to the embedded Calendar's live month/year heading. Initial grid focus
waits for visible Anchor geometry. Calendar's selected date supplies focus on
opening, overridden by valid typed text; otherwise the explicit fallback is used.

Date_picker owns a focus boundary rather than pretending Popover's nonmodal
semantics implement a modal dialog. Two empty focusable span sentinels redirect
native Tab traversal to the first/last rendered control. Capture-phase focusin
also redirects attempted programmatic background focus into the roving date.
**Tab is never prevented.** Disabled bounded navigation is skipped, including
when all four navigation buttons are disabled. No positioning or focus behavior
requires CSS. Do not use positive tabindex, portals, hidden sentinels, or custom
focusable descendants outside the boundaries. Arbitrary composite widgets with
their own focus policies are not part of this date-picker anatomy.

This is focus-modal while expanded, not a native inert `<dialog>`: pointerdown
outside dismisses *before* the browser moves focus, leaving the clicked
focus destination alone. Background nodes are not made inert, because that
would suppress the requested outside-pointer focus destination. Once the outside
pointerdown has dismissed the picker, ordinary background interaction resumes.
`aria-modal` exposes the expanded interaction as modal to assistive technology.
This pointer-dismissal policy and anchored presentation are intentional choices
from the task brief, rather than the APG example's centered dialog presentation.
Native Dialog was not chosen because its inert background and automatic native
focus restoration conflict with this outside-pointer policy. Inside a native
Tea_ui.Dialog, the picker's promoted panel remains a DOM descendant, so the
ancestor's native modality still permits its controls. The first Escape belongs
to the picker; after it closes, the second belongs to the ancestor Dialog.

## Integrate

```reason
open Tea;
module P = Ui.Date_picker;
module C = Ui.Calendar;
type msg = Date_picker(P.msg);
let today = Option.get(C.Date.of_iso("2026-03-05"));
let picker = P.init(~id="visit", ~focus=today, ());
let update = (Date_picker(message), picker) => {
  let (picker, command, outs) = P.update(message, picker);
  /* Handle Changed, Invalid, Opened, Closed in the parent. */
  ignore(outs);
  (picker, Cmd.map(message => Date_picker(message), command));
};
let subscriptions = picker =>
  Sub.map(message => Date_picker(message), P.subscriptions(picker));
```

Render using `P.view(~model=picker, ~to_parent, ~to_view, ())`:

1. Apply `group` to a container around the input, trigger and dialog. Apply
   `label` to label, `input` to a native text input, and `trigger` to a native
   button whose visible text is `labels.choose` (English: Choose date).
2. Render the `dialog` div immediately after the trigger. Keep it mounted, or
   mount while `rendered` is true (includes Leaving, not just logical openness).
3. Put the empty `start_boundary` span first and `end_boundary` span last inside
   the dialog. Neither needs CSS, children, or extra events.
4. Render `calendar.root`, `calendar.heading` with `heading_text`, navigation
   buttons, and `calendar.grid` as a table with weekday th and date td bundles.
   Keep the heading/table at stable DOM positions. Render every placeholder td.
   Do not nest buttons in date cells. Year navigation buttons are optional.
5. In confirm mode render `confirm_button` with `labels.confirm`. Render
   `cancel_button` with `labels.cancel` in either mode. Optional `close_button`
   has `labels.close` as its accessible name and reports Programmatic.
6. Render `error` with `error_text` (it hides when valid). Render `description`
   iff declaring `has_description=true`. Input conventions/idrefs match Input.
7. Optionally render `instructions` with `instructions_text`. Keep this live
   region mounted. It can be visually hidden, not hidden/aria-hidden. The text
   changes after initial grid focus so the polite announcement follows focus.
8. Render `hidden_input` as an input for forms; it carries `name` and the ISO
   committed value, not the localized editable text.

The callback adds no wrapper. Bundles are in the parent's message type. Append
only nonconflicting attributes; owned event names must not be overridden.
The [complete example](../../examples/ui/date_picker/main.re) demonstrates all
parts, ISO, DD/MM/YYYY parsing/formatting, unavailable weekends and inclusive
bounds, confirm/cancel, form submission, RTL, disabled/read-only, and nesting
inside a native Dialog. Its CSS is presentation only and honors reduced motion.

## Editable text, selection and constraints

The model owns both committed `value` and editable `text`. `format` and `parse`
are immutable, parent-supplied pure functions configured at init. Defaults are
Calendar.Date.to_iso/of_iso (strict ASCII YYYY-MM-DD, years 0001–9999).

- Input events change text, not the domain value. Blur or unmodified Enter
  commits the latest edit; Enter is prevented to avoid submitting the form with
  a value that has not been rendered into its hidden input yet. A later blur
  does not duplicate the same commit. Other text editing remains native.
- Empty text commits None. Successful parsing commits Some(date), normalizes
  text through format, and emits Changed. Recommitting a newly edited identical
  value still reports the successful user commit.
- Invalid text remains visible and emits Invalid(raw_text, reason). The previous
  committed value and hidden ISO value are retained. Error is visible, the input
  has aria-invalid and aria-errormessage, and both description/error idrefs
  resolve. A subsequent successful commit/setter clears validation.
- Constraints are supplied identically to update/view/open_, never stored:
  inclusive min, inclusive max, and pure is_unavailable. Typed failures are
  distinguished as Syntax, Before_min, After_max, or Unavailable. Reversed bounds
  permit no date; lower-bound validation takes priority when failures overlap.
- Opening with valid uncommitted typed text focuses it without committing it.
  Invalid/unavailable typed text falls back to committed value, then init's focus.
  Native input blur on button activation normally commits before opening.
- Immediate mode (default) commits a selected date, formats the input, emits
  Changed then Closed(Selection), and returns focus to Choose date.
- Confirm mode stages Calendar selection. OK revalidates the staged date against
  current constraints, commits and emits Changed then Closed(Confirm). With no
  staged selection, OK commits None. Cancel, Escape, outside dismissal, generic
  close, and silent close discard staging; reopening resets focus/selection from
  the committed value. Invalid newly constrained staging disables OK.
- Disabled prevents opening, editing and grid interaction. Its native controls
  and hidden form input are disabled. Read-only keeps input readonly and allows
  opening/browsing, but forbids typed commits, date selection, and OK. Cancel
  and Escape remain available. Pass current modes to both update and view.
- `set_value` is silent, clears validation and formats text. It does not validate
  constraints, close the dialog, or move focus. Existing values are retained if
  the parent later changes constraints, matching Calendar's contract.

`open_`/`close` are silent programmatic setters. User Opened/Closed notifications
are logical transitions, not animation completions. Use `close_button` for a
notified Programmatic dismissal with focus return. Silent close deliberately
leaves the parent in control of focus. All focus returns respect a subsequent
intentional connected destination; outside pointerdown cancels pending returns.

The hidden input submits the last committed ISO date (empty string for None).
The editable input has no name, preventing duplicate/localized submissions.
`required` uses the native text input; custom invalidity is ARIA validation,
not setCustomValidity or a form-submission guard. Applications deciding not to
submit while `invalid_reason != None` should enforce that in their form update.

## Keyboard

| Input | Behavior |
| --- | --- |
| Enter / Space on Choose date | Native activation opens; expanded activation cancels |
| Enter in editable input | Commit latest edit; prevent implicit form submission |
| Blur from input | Commit latest edit, once |
| Left / Right in grid | Previous / next day; swapped in RTL |
| Up / Down | Previous / next week |
| Home / End | First / last day of configured week |
| PageUp / PageDown | Previous / next month, clamp day at month end |
| Shift+PageUp / Shift+PageDown | Previous / next year, clamp leap day |
| Enter / Space on date | Select unless read-only/unavailable/out of bounds; close in immediate mode |
| Enter / Space on navigation | Native activation, retains button focus |
| Enter / Space on OK / Cancel / Close | Confirm / cancel / generic dismissal, return focus to Choose date |
| Tab / Shift+Tab | Native control traversal, wrap inside dialog; never prevent default |
| Escape while expanded | Close, prevent default **and** stop propagation; return focus to Choose date |
| Ctrl / Alt / Meta shortcuts, IME, unhandled keys | Unchanged; propagate |
| Outside pointerdown (including the text input) | Close without moving focus; the input receives native pointer focus |

Calendar navigation reaches disabled dates (not selectable), saturates at the
civil-date domain, and keeps one roving grid Tab stop. Month/year controls honor
bounds and remain focus-stable when becoming aria-disabled. See [Calendar](calendar.md).

## ARIA, ids and styling

With prefix `visit`, the trigger/panel ids are `visit-trigger`/`visit-panel`, and
Calendar prefix is `visit-calendar`; the anchor container is `visit-group`. Input uses prefix `visit-input`, yielding
`visit-input-control`, `-label`, `-description`, `-error`. Focus boundaries are
`visit-start`/`visit-end`; instructions are `visit-instructions`; action buttons
are `visit-confirm`, `visit-cancel`, `visit-close`. All prefixes must be unique.

- Native text input/label with description/error conventions from Input.
- Choose date has aria-haspopup=dialog, aria-expanded, and aria-controls only
  during the popup's rendered lifetime. Its accessible name includes the
  committed date in English by default, independently of editable formatting.
- Dialog has role=dialog, aria-modal=true, and aria-labelledby the live month/year
  heading. Calendar supplies grid/weekday/date semantics, selected/current date,
  readonly/disabled state, and polite atomic month announcements.
- Focus boundaries are aria-hidden empty implementation spans, never persistent
  focused controls; focusin immediately redirects to real controls.
- Picker flags: data-open on trigger/panel; data-disabled/data-readonly on input,
  data-readonly on trigger; data-invalid on input. Calendar's date/navigation
  flags, Anchor's data-placement/data-reference-hidden, and Animation's
  data-closed/data-enter/data-leave/data-transition are forwarded unchanged.
- All boolean data hooks are presence attributes. Only Anchor geometry uses
  inline styles. Closed content uses hidden/native popover state, not CSS.

## Full public API

All public fields and signatures (qualified modules as in Tea_ui):

```reason
type date = Calendar.date;
type invalid_reason = Syntax | Before_min | After_max | Unavailable;
type reason = Escape | Outside | Selection | Confirm | Cancel | Programmatic;
type out_msg = Changed(option(date)) | Invalid(string, invalid_reason)
             | Opened | Closed(reason);
type labels = {
  choose: string, choose_selected: date => string, confirm: string,
  cancel: string, close: string, instructions: string,
  error: invalid_reason => string,
};
let english: labels;
type model;
type msg;
let init: (
  ~id: string, ~focus: date, ~selected: date=?, ~confirm: bool=?,
  ~format: date => string=?, ~parse: string => option(date)=?,
  ~first_day_of_week: int=?, ~dir: Calendar.direction=?,
  ~fixed_weeks: bool=?, ~show_outside_month: bool=?,
  ~placement: Anchor.placement=?, ~top_layer: bool=?, unit
) => model;
let value: model => option(date);
let text: model => string;
let invalid_reason: model => option(invalid_reason);
let is_open: model => bool;
let should_render: model => bool;
let phase: model => Animation.phase;
let focus_date: model => date;
let set_value: (option(date), model) => model;
let open_: (~disabled: bool=?, ~constraints: Calendar.constraints=?, model)
  => (model, Tea_core.Cmd.t(msg));
let close: model => (model, Tea_core.Cmd.t(msg));
let update: (
  ~constraints: Calendar.constraints=?, ~disabled: bool=?, ~readonly: bool=?,
  msg, model
) => (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
type parts('parent) = {
  group: list(Tea_html.Html.attr('parent)),
  label: list(Tea_html.Html.attr('parent)),
  input: list(Tea_html.Html.attr('parent)),
  trigger: list(Tea_html.Html.attr('parent)),
  description: list(Tea_html.Html.attr('parent)),
  error: list(Tea_html.Html.attr('parent)),
  error_text: string,
  hidden_input: list(Tea_html.Html.attr('parent)),
  dialog: list(Tea_html.Html.attr('parent)),
  start_boundary: list(Tea_html.Html.attr('parent)),
  end_boundary: list(Tea_html.Html.attr('parent)),
  instructions: list(Tea_html.Html.attr('parent)),
  instructions_text: string,
  calendar: Calendar.parts('parent),
  confirm_button: list(Tea_html.Html.attr('parent)),
  cancel_button: list(Tea_html.Html.attr('parent)),
  close_button: list(Tea_html.Html.attr('parent)),
  confirm: bool, open_: bool, rendered: bool,
  value: option(date), text: string,
};
let view: (
  ~model: model, ~constraints: Calendar.constraints=?, ~disabled: bool=?,
  ~readonly: bool=?, ~required: bool=?, ~has_description: bool=?, ~name: string=?,
  ~today: date=?, ~labels: labels=?, ~calendar_labels: Calendar.labels=?,
  ~to_parent: msg => 'parent,
  ~to_view: parts('parent) => Tea_html.Html.t('parent), unit
) => Tea_html.Html.t('parent);
```

## Private capabilities, lifecycle and limits

No composed components or foundation files are changed. Private browser
capabilities in Date_picker only:

- Capture-phase document focusin and synchronous control or roving-date focus
  at span boundaries. Control selection uses `Browser_dom.first_tabbable` /
  `last_tabbable`, excluding boundary spans: positive tabindex comes first,
  followed by DOM order, with the shared browser exclusions.
- Subscription-owned, live-guarded post-render initial/return focus, using
  Browser_dom.active_element_id as the deferred frame handoff. A generation
  prevents stale completions; no mutable values/DOM nodes are stored in models.

Existing Calendar/Popover/Anchor/Animation bounds apply: document-local ids,
no iframe/shadow-root traversal, no positive-tabindex reordering or generic
nested composite focus policy, no range/multiselect, native Popover API required
for top-layer promotion (fixed-position fallback otherwise). The animation
watchdog may finish intentional animations longer than five seconds. Keep
subscriptions through Leaving. Removing a submodel removes its subscriptions
with its DOM; shutdown releases promotion, listeners and animation waits.
Native modal ancestors should close children before closing themselves.
Manual screen-reader and non-Chromium device validation are not claimed.

## Verification

```sh
dune build @test/ui/date_picker/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs date_picker
```

jsdom mounts real Tea programs: all keyboard rows, idrefs/attribute ownership,
immutable snapshots, batched intents, parser/constraint failures, controlled
text/form value, staging, current modes, localization, RTL, cancellation,
unmount and shutdown during an unresolved animation. The shared harness checks
listeners, timers and MutationObservers. jsdom's lack of native popover focus
support is handled by the public top_layer=false option, not a browser polyfill.
Chromium exercises real top-layer focus, native Enter/Space activation, every
Calendar key, both Tab boundaries (including disabled navigation), pointer
selection, exact open/closed ARIA snapshots, typed input/errors/constraints,
confirm/cancel, custom formatting, successful form controls, read-only/disabled,
nested Escape layering, input-group alignment, viewport-edge flip/shift,
text-input outside dismissal, unmount and unfiltered axe in each meaningful state.

Initial date focus waits for visible positioning and only proceeds while focus
remains at the Choose button, date input, existing panel, or body. A focus move
before the focus frame supersedes it. If initial focus is skipped, the opening
is dismissed with `Closed(Outside)` and never activates its modal focus boundary.
The boundary and live instructions become active only after initial focus succeeds.
Return focus is also cancellable and does not override a later focus destination.
