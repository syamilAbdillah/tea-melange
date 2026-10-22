# Accessible headless UI

`tea.ui` supplies semantics, keyboard behavior and presence data attributes;
your application owns state and presentation. It depends on `tea.browser` for
post-render DOM effects and lifecycle-owned subscriptions. The umbrella exposes
`Tea.Ui`, `Tea.Browser_dom` and `Tea.Browser_events` as module aliases.

## Build, test, run

Use the repository's opam switch, Node 22 and root `npm ci`.

```sh
dune build
dune build @runtest --force
dune build @doc
npx playwright-core install --with-deps chromium
npm run test:e2e
node scripts/serve.js ui/button
```

Open http://127.0.0.1:8000/. Examples are built by
`dune build @examples/melange`. For an existing Chromium installation:

```sh
TEA_E2E_CHROMIUM=/usr/bin/google-chrome npm run test:e2e
npm run test:e2e -- button
TEA_E2E_SPEC=browser npm run test:e2e
```

The no-ML and formatting gates are `./scripts/check-no-ml.sh` and
`./scripts/check-fmt.sh`. Format changed Reason sources with `refmt --in-place`.

## Conventions

[spec.md](spec.md) is normative. In particular:

- One module and documented interface per component. The library discovers new
  modules automatically; never edit its Dune stanza to add a component.
- Stateless parts are attribute bundles in the parent's message type. Stateful
  messages carry intent; update derives results from the latest model.
- Collections stay in the parent and use stable keys, never array indices.
- Apply each part's owned event handlers once. Compose through documented hooks.
- Use document-unique id prefixes, private `Ui_attr` derivation and explicit
  ARIA idrefs. Presence data flags carry styling state.
- Prefer native controls. Disabled and read-only differ; no CSS is required for
  correctness. Preserve Tab, browser shortcuts and IME composition.
- Effects are lazy and deferred through the shared frame scheduler. Runtime
  updates request their draw before executing commands and subscription effects.
- Subscription identity is `(home,key)`; kept sources receive current taggers.
  Return `Sub.none` or shut down to dispose. Flush the closing render before
  asserting restored focus/styles.

## Components

Parent-owned helpers have no submodel: update your own state in response to
intent messages. Stateful components use an opaque submodel: retain the returned
model, map **every command**, and map subscriptions for the entire mounted
lifetime, including enter/leave. Collections remain application-owned in both
cases. The `.rei` files are the authoritative public API.

Example paths below are relative to `examples/`; run, for example,
`node scripts/serve.js ui/showcase`. The [Fieldwork showcase](../../examples/ui/showcase/)
composes these into one workspace app, with a form, nested overlays, notifications,
file attachments, sortable tasks and a virtual audit log. It is local-only:
no persistence, upload or backend is implied.

| Component | Model kind | Documentation | Example path | APG pattern / semantics |
| --- | --- | --- | --- | --- |
| Button | Parent-owned | [Button](button.md) | `ui/button` | [Button](https://www.w3.org/WAI/ARIA/apg/patterns/button/) |
| Input | Parent-owned | [Input](input.md) | `ui/forms` | Native input; [APG overview](https://www.w3.org/WAI/ARIA/apg/patterns/) |
| Textarea | Parent-owned | [Textarea](textarea.md) | `ui/forms` | Native textarea; [APG overview](https://www.w3.org/WAI/ARIA/apg/patterns/) |
| Checkbox | Parent-owned | [Checkbox](checkbox.md) | `ui/forms` | [Checkbox](https://www.w3.org/WAI/ARIA/apg/patterns/checkbox/) |
| Fieldset | Parent-owned | [Fieldset](fieldset.md) | `ui/forms` | Native grouping; [Checkbox groups](https://www.w3.org/WAI/ARIA/apg/patterns/checkbox/) |
| Switch_ (Switch) | Parent-owned | [Switch](switch.md) | `ui/forms` | [Switch](https://www.w3.org/WAI/ARIA/apg/patterns/switch/) |
| Select | Parent-owned | [Select](select.md) | `ui/forms` | Native select, not the custom [Combobox](https://www.w3.org/WAI/ARIA/apg/patterns/combobox/) keyboard model |
| Meter | Parent-owned | [Meter](meter.md) | `ui/meter` | [Meter](https://www.w3.org/WAI/ARIA/apg/patterns/meter/) |
| Progress | Parent-owned | [Progress](progress.md) | `ui/progress` | No dedicated APG pattern; [progressbar](https://www.w3.org/TR/wai-aria-1.2/#progressbar) |
| Nav | Parent-owned | [Nav](nav.md) | `ui/nav` | [Disclosure navigation](https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/examples/disclosure-navigation/) guidance; ordinary links |
| Disclosure | Parent-owned | [Disclosure](disclosure.md) | `ui/disclosure` | [Disclosure](https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/), [Accordion](https://www.w3.org/WAI/ARIA/apg/patterns/accordion/) composition |
| Animation | Stateful submodel | [Animation](animation.md) | `ui/animation` | No widget semantics; [APG overview](https://www.w3.org/WAI/ARIA/apg/patterns/) |
| Anchor | Stateful submodel | [Anchor](anchor.md) | `ui/anchor` | Geometry only; [APG overview](https://www.w3.org/WAI/ARIA/apg/patterns/) |
| Hover_intent | Stateful submodel | [Hover intent](hover_intent.md) | `ui/hover_intent` | Pointer timing only; related [Tooltip](https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/) |
| Radio_group | Stateful submodel | [Radio group](radio_group.md) | `ui/radio_group` | [Radio group](https://www.w3.org/WAI/ARIA/apg/patterns/radio/) |
| Tabs | Stateful submodel | [Tabs](tabs.md) | `ui/tabs` | [Tabs](https://www.w3.org/WAI/ARIA/apg/patterns/tabs/) |
| Slider | Stateful submodel | [Slider](slider.md) | `ui/slider` | [Slider](https://www.w3.org/WAI/ARIA/apg/patterns/slider/), [multi-thumb](https://www.w3.org/WAI/ARIA/apg/patterns/slider-multithumb/) |
| Calendar | Stateful submodel | [Calendar](calendar.md) | `ui/calendar` | [Date picker grid](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/examples/datepicker-dialog/) |
| Drag_and_drop | Stateful submodel | [Drag and drop](drag_and_drop.md) | `ui/drag_and_drop` | No dedicated APG pattern; [Button](https://www.w3.org/WAI/ARIA/apg/patterns/button/) handles and live announcements |
| File_drop | Stateful submodel | [File drop](file_drop.md) | `ui/file_drop` | No dedicated APG pattern; native [Button](https://www.w3.org/WAI/ARIA/apg/patterns/button/) alternative |
| Virtual_list | Stateful submodel | [Virtual list](virtual_list.md) | `ui/virtual_list` | No imposed widget; [active-descendant guidance](https://www.w3.org/WAI/ARIA/apg/practices/keyboard-interface/#kbd_focus_activedescendant) |
| Dialog | Stateful submodel | [Dialog](dialog.md) | `ui/dialog` | [Modal dialog](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/), [Alertdialog](https://www.w3.org/WAI/ARIA/apg/patterns/alertdialog/) |
| Popover | Stateful submodel | [Popover](popover.md) | `ui/popover` | [Disclosure](https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/); optional nonmodal dialog |
| Menu | Stateful submodel | [Menu](menu.md) | `ui/menu` | [Menu button](https://www.w3.org/WAI/ARIA/apg/patterns/menu-button/), [menu/submenu](https://www.w3.org/WAI/ARIA/apg/patterns/menubar/) |
| Listbox | Stateful submodel | [Listbox](listbox.md) | `ui/listbox` | [Listbox](https://www.w3.org/WAI/ARIA/apg/patterns/listbox/) |
| Combobox | Stateful submodel | [Combobox](combobox.md) | `ui/combobox` | [Combobox](https://www.w3.org/WAI/ARIA/apg/patterns/combobox/) |
| Tooltip | Stateful submodel | [Tooltip](tooltip.md) | `ui/tooltip` | [Tooltip](https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/) (APG work in progress), WCAG hover/focus content |
| Toast | Stateful submodel | [Toast](toast.md) | `ui/toast` | No dedicated APG pattern; [Alert/live region](https://www.w3.org/WAI/ARIA/apg/patterns/alert/) guidance |
| Date_picker | Stateful submodel | [Date picker](date_picker.md) | `ui/date_picker` | [Date picker dialog](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/examples/datepicker-dialog/) |

The separate `tea.browser` library exposes **Browser_dom** (lazy post-render DOM
tasks: focus, geometry, scrolling, native dialogs and animation waits) and
**Browser_events** (lifecycle-owned document/window events, observers, media
queries, scroll locks and conditional focus restoration). See
[foundation.md](foundation.md) and `src/browser/*.rei`; these are effects and
subscriptions, not component submodels or APG widgets.

## Choosing a component

- Prefer **Select** for a short, ordinary form choice and native platform UX.
  **Listbox** adds styled single/multiple selection with a focus-owning popup.
  **Combobox** adds editable search with input-retained focus; the application
  filters suggestions, handles remote loading and supplies cached labels.
- **Popover** is a nonmodal disclosure for filters or supplementary controls;
  Tab is natural, not trapped. **Dialog** uses native modal containment for a
  task requiring attention; use `Alertdialog(cancel_id)` for a destructive
  confirmation. **Menu** holds commands (and submenus/toggles), not form fields
  or website navigation. Use **Nav** for ordinary links.
- **Tooltip** is noninteractive supplemental text on hover/keyboard focus.
  Choose **Popover** if the content needs links, buttons or other interaction.
  For grouped tooltips, fold notifications through `Tooltip.track` into the
  parent-owned `Hover_intent.Group`; close other visible members immediately
  and forget them when a member opens. The showcase demonstrates this policy.
- **Calendar** is only a civil-date grid. **Date_picker** adds input parsing,
  popup dismissal, keyboard containment and focus return. **Meter** measures a
  bounded quantity; **Progress** describes completion, including indeterminate
  work. Neither is an input.

## Overlay layering & Escape

Keep nested popups **inside their ancestor's DOM**, not in a body portal.
Anchor's native manual-popover promotion puts them in the top layer without
moving nodes. A handled Escape in Listbox, Combobox, Date_picker, Menu, Popover
or Tooltip prevents default and stops propagation, so a bubbling Dialog handler
sees only a subsequent unclaimed Escape. A submenu first closes its own branch.
A closed editable Combobox may consume Escape to clear its text/selection.

There is no global dismissal stack. Earlier document capture listeners and
independent sibling overlays are not coordinated by stopped bubbling; do not
install an unconditional global Escape-to-close-everything handler. Parents
close child overlays before closing their modal ancestor, retain subscriptions
through Leaving, and choose focus fallbacks if openers disappear. The
[showcase integration test](../../test/ui-e2e/specs/showcase.spec.mjs) checks
inner Escape behavior using real native modality.

## Focus management

Use document-unique id prefixes and apply every part to its documented element.
Do not replace owned keyboard handlers with a second handler of the same name;
use the component's unhandled-key hooks. Forward returned commands: effects run
after the next render, not against the previous DOM.

Dialog uses native `<dialog>` containment. Its opening focus order is explicit
initial target, first tabbable, then root; an alertdialog requires a rendered,
enabled least-destructive action id. Popover retains focus by default, with
opt-in first/explicit target focus. Date_picker contains keyboard traversal via
its required first/last boundary spans, but is not native inert modality.
Listbox focuses its popup; Combobox keeps focus in the text input and points to
an active option. Radio groups, Tabs and Calendar use roving tab stops.

Focus return is conditional: disconnected openers are skipped and deliberate
focus elsewhere is respected. An application closing a Disclosure containing
focus must choose a destination first. Virtualized composites must synchronously
pin their active row in the same render so active-descendant idrefs resolve;
Virtual_list does not add selection or keyboard semantics for you.

## Animations

The overlays embed **Animation**; standalone use also needs mapped update,
commands and subscriptions. Style presence attributes, not library-owned inline
geometry or `hidden`:

```css
.panel[data-transition] { transition: opacity 150ms ease; }
.panel[data-closed] { opacity: 0; }
@media (prefers-reduced-motion: reduce) {
  .panel[data-transition] { transition: none; }
}
```

Entering first renders `data-closed data-enter data-transition`, then drops
`data-closed` across a paint, waits for subtree animations, and settles Open.
Leaving adds `data-closed data-leave data-transition` and stays mounted/visible
until the wait completes. Only Closed is hidden. Keep the same model across
reversals and keep subscriptions through Leaving; use completion/visibility
accessors rather than removing the element at the close request. With no CSS,
completion is immediate after the render handoffs. Paused/infinite effects are
bounded by a 5000 ms watchdog, not awaited forever.

## Testing your app

Use the [jsdom harness](../../test/ui/README.md) to mount a real `App.element`,
dispatch DOM events, flush render/effect frames, and assert ARIA/idrefs, focus,
latest-model intent handling, emitted outputs and teardown. The shared setup's
leak gate checks document/window listeners, timers and observers; shut down the
runtime and flush closing effects before asserting restored locks/focus. Do not
replace lifecycle sources with permanently installed test listeners.

Then drive your actual emitted example with the
[Chromium harness](../../test/ui-e2e/README.md): native Tab/Shift+Tab, trusted
pointer actions, nested Escape, focus return, real transitions, snapshots and
unfiltered axe on load **and with overlays open**. Axe cannot establish keyboard
correctness or screen-reader usability; test supported assistive technologies
and devices manually too. The showcase smoke test also exercises dark/mobile
styling, file selection, keyboard sorting and bounded virtual DOM.

```sh
./scripts/check-no-ml.sh && ./scripts/check-fmt.sh
dune build && dune build @runtest --force && dune build @doc
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs showcase
TEA_E2E_CHROMIUM=/usr/bin/google-chrome npm run test:e2e
```

## Known limitations

These are deliberate boundaries, not a claim of universal accessibility or
browser coverage. Follow the linked component docs for exact contracts.

- **Browser/overlays:** current light-DOM document ids only; no shadow-root or
  iframe traversal. Dialog requires native modal dialog support, with no
  modality polyfill or nonmodal variant. Date_picker contains keyboard focus
  but background pointer interaction dismisses rather than making the entire
  document inert. Close nested children before closing a native modal ancestor.
- **Anchor:** no portal fallback, visual-viewport/pinch-zoom support, vertical
  writing modes, clipping-ancestor hiding or continuous polling of transforms
  and unrelated layout shifts. Without native Popover API, fixed panels cannot
  escape transformed/clipping ancestors. Use `Anchor.refresh` for unobserved
  layout changes; without ResizeObserver, element-only resize is not observed.
- **Animation:** waits snapshot current subtree animations, not future ones;
  the 5000 ms bound can cut short intentionally paused/infinite/long effects.
- **Forms:** state, validation, reset and persistence belong to the application.
  Hidden form mirrors do not perform native required validation; custom controls
  need parent validation. Fieldset cannot disable role-based descendants: pass
  its disabled state explicitly (the first native legend is exempt). Input is
  for text-like types, not files/checkboxes/radios. Select deliberately retains
  OS-specific keys. Custom Button/Checkbox Space handling is stateless, not an
  armed-gesture tracker; prefer native controls for cancellation/repeat details.
- **Disclosure/Nav:** no mandatory-one-open accordion or optional accordion
  arrow navigation. Closing a focused panel needs application focus management.
  Nav is ordinary link navigation, not an arrow-key menubar. Meter needs a
  visible label part or `aria_label`; Meter/Progress are noninteractive displays.
- **Radio_group/Tabs/Slider:** no toolbar-specific radio model, radio typeahead,
  tab Delete-to-close or tab popup menus. Slider thumbs never cross; Escape
  does not revert a drag. Numeric grids must satisfy the documented finite/
  precision bounds. Touch AT may not synthesize slider keys reliably; actual
  device testing remains necessary. Keep drag geometry stable.
- **Calendar/Date_picker:** single dates only, proleptic Gregorian years 1–9999;
  no date ranges, timezone conversion, clock or automatic Intl localization.
  Applications supply today, parsers/formatters, labels and availability rules.
- **Menu/Listbox/Combobox:** options/menuitems cannot contain interactive
  descendants. Menu is not a menubar, embedded-widget container or virtualized
  collection and does not dismiss on every programmatic focus move. Listbox
  virtualizers must pin active options; missing-opener fallbacks/reset belong
  to the application. Combobox filtering/requests/chip labels are parent-owned;
  matching is lowercase/NFD with documented folds, not locale-aware collation
  or a full Unicode casefold. Keep options inside their own popup.
- **Hover_intent/Tooltip/Popover:** temporal grace, not a geometric safe polygon;
  no touch long-press tooltip or interactive tooltip content. Hover_intent alone
  supplies no focus/ARIA/dismissal semantics. Popover is nonmodal and has no
  global nested-overlay stack; use Dialog for modal tasks.
- **Toast:** no swipe-to-dismiss or hover-expanded stacking. Mount both live
  lanes before pushing; finite durations are at least 5000 ms, which may still
  be too short for long content. Action toasts must persist and have an
  equivalent route outside the toast. Urgent notifications interrupt speech.
- **Drag_and_drop/File_drop:** single-item list sorting only; no virtualized
  unmounted targets, nested sortables, arbitrary 2D grids or cross-window drag.
  File_drop has no paste, directory enumeration, upload transport or persistent
  selection; validate again on the server. Events must reach document, targets
  must not nest, and drops outside the target keep native browser behavior.
- **Virtual_list:** vertical rows only; browser scroll-size limits still apply
  (Chromium roughly 33 million CSS pixels), without segmented scrolling.
  Unmounted content is absent from Find/printing/screen-reader browse mode;
  offer pagination/full rendering when needed. No automatic keyboard/selection
  policy; without ResizeObserver, estimates cannot track idle size changes.

Foundation APIs and implementation decisions: [foundation.md](foundation.md).
Test conventions: [jsdom](../../test/ui/README.md),
[Chromium](../../test/ui-e2e/README.md),
[examples](../../examples/ui/README.md).
