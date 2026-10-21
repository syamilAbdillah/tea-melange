# tea.ui — accessible headless components (design spec)

Status: normative for every task that builds `tea.ui`. Deviations need a written
reason in the task report and must be approved by the integrator.

Inspiration: Foldkit UI (https://foldkit.dev/ui/overview). Behaviour is defined
by the **WAI-ARIA Authoring Practices Guide (APG)**, https://www.w3.org/WAI/ARIA/apg/patterns/,
not by any one library. All code is original; do not copy Foldkit, Headless UI,
Radix or React Aria source.

"Production ready" here means: complete APG keyboard model, correct ARIA in
every state, focus never lost or trapped by accident, works with pointer,
touch and keyboard, no leaked listeners/locks after close or unmount, no
dependency on CSS for correctness, typed/documented API, and tests that prove
all of that in jsdom **and** a real Chromium.

## 1. Layers

```
tea.core / tea.vdom / tea.html / tea.dom      (existing)
        │
tea.browser   (new, melange)  DOM effects + document/window subscriptions
        │
tea.ui        (new, melange)  components; Tea.Ui = Tea_ui
```

### 1.1 Runtime ordering fix (tea.core)

Today `Runtime.drain` runs a message's commands **before** asking the animator
for a frame. A command that schedules work with `requestAnimationFrame` (e.g.
"focus the menu that this update just opened") therefore runs *before* the
render that creates the element. Fix: request the frame **before** running the
commands, so every rAF callback scheduled by a command runs after the draw it
depends on (rAF and the 16 ms setTimeout fallback are FIFO). Update
`runtime.rei` docs, `docs/runtime-contract.md`, and add a runtime test that
records the order (draw before command callback) for all three animator states
(`No_request`, `Pending`, `Extra_request`).

### 1.2 tea.browser (`src/browser`, `(modes melange)`, public `tea.browser`)

Elm's `Browser.Dom` + `Browser.Events`, plus what overlays need. All JS in
`Bindings`-style private modules; no `%raw` unless unavoidable and commented.

`Browser_dom` — tasks/commands. **Every DOM effect runs after the next render**
(scheduled through `Tea_dom.Browser_platform.request_animation_frame`, which is
ordered after the draw by 1.1). Elements are addressed by `id`.

- `type error = Not_found(string)`
- `focus: (~prevent_scroll: bool=?, string) => Task.t(unit, error)`, `blur`
- `focus_cmd: string => Cmd.t('msg)` (ignores Not_found; convenience)
- `focus_first: (~within: string) => Task.t(unit, error)` — first tabbable
  descendant (tabbable = focusable, not disabled, not `tabindex=-1`, not inert,
  visible); used for dialogs with no explicit initial focus.
- `scroll_into_view: (~block: [ `Nearest | `Center | `Start | `End ]=?, string) => Task.t(unit, error)`
- `get_element: string => Task.t(element_info, error)` with the element's
  bounding rect, viewport width/height and scroll offsets (floats, CSS px).
- `show_modal / show / close: string => Task.t(unit, error)` for `<dialog>`.
- `wait_for_animations: string => Task.t(unit, error)` — after render, await
  `el.getAnimations({subtree: true})` finished/cancelled (allSettled). Resolves
  immediately when there are none or `getAnimations` is unsupported. Must never
  hang: if the element is removed or an animation is cancelled, it resolves.
- `active_element_id: Task.t(option(string), 'e)`.

`Browser_events` — subscriptions (lifecycle = while the model asks for them, so
no manual cleanup; `(home, key)` identities documented per function):

- `on_key_down(~key: string, Decode.t('msg))` on `document` (capture flag opt).
- `on_pointer_down_outside(~ids: list(string), 'msg)` — capture-phase
  `pointerdown` whose target is not inside any element with those ids (ids that
  do not exist count as "not inside"). Also fires for `focusin` outside when
  `~focus: true` (default false).
- `on_resize`, `on_scroll` (capture, any scroll container), `on_visibility_change`.
- `on_element_resize(~id)` (ResizeObserver; no-op if unsupported).
- `on_media_query(~query)` (e.g. `(prefers-reduced-motion: reduce)`), delivers
  the current value on start and on change.
- `scroll_lock(~key)` — while subscribed, the document does not scroll. Reference
  counted across keys; restores the exact previous inline styles; compensates
  scrollbar width (no layout shift); works on iOS Safari (no body position hack
  unless needed — document what you chose and why).
- `restore_focus(~key, ~within=?, ())` — on start remembers `document.activeElement`; on stop
  (after the next render) refocuses it if it is still connected and focus is on
  `body`, inside a removed subtree, or inside the element `within` (an overlay
  kept mounted while it animates out).

### 1.3 tea.ui (`src/ui`, `(modes melange)`, public `tea.ui`, wrapped `Tea_ui`)

One component = one module `src/ui/<name>.re` + `.rei` (e.g. `listbox.re` →
`Tea_ui.Listbox`, `Tea.Ui.Listbox`). The library stanza must pick up new modules
without edits (no `(modules ...)` list). Shared private helpers are created by
the foundation task only and listed in `(private_modules ...)`:

- `Ui_keyboard` — keydown decoders reading `key`, `shiftKey`, `ctrlKey`,
  `altKey`, `metaKey`, `isComposing`, `timeStamp`; helpers that prevent default
  **only** for handled keys. IME composition never reaches handlers; modifiers
  do (Alt+ArrowDown, Tab), so handlers check `Ui_keyboard.plain` before claiming
  shortcut-like keys and never prevent default on Tab.
- `Ui_nav` — pure roving/active-index math over `array(bool /*disabled*/)`:
  next/prev with and without wrap, first/last enabled, page jumps, all-disabled.
- `Ui_typeahead` — pure: buffer + last timeStamp (from the event, not a clock)
  → match index; 500 ms reset (configurable); repeated same letter cycles;
  case/diacritic-insensitive prefix match (`String.lowercase_ascii` plus a small
  Latin-1 fold; document limits).
- `Ui_attr` — `data_flag(name, bool)` emits `data-<name>=""` or nothing;
  `aria_bool`, `aria_ids` (space-joined, omitted when empty), id derivation
  `Ui_attr.part_id(id, "button")` = `id ++ "-button"`.

If a component needs another private helper, put it inside its own module and
hide it with the `.rei`. Components may depend on other components' public API
(e.g. Menu uses Anchor and Animation).

## 2. Component API conventions

### 2.1 Stateless render helpers

Button, Input, Textarea, Checkbox, Fieldset, Switch, Select, Meter, Progress,
Disclosure, Nav. The parent owns all state.

```reason
/* switch_.rei (module names that clash with keywords get a trailing _) */
type parts('msg) = {
  root: list(Html.attr('msg)),         /* the role="switch" button */
  label: list(Html.attr('msg)),
  description: list(Html.attr('msg)),
};
let view:
  (
    ~id: string,
    ~checked: bool,
    ~on_change: bool => 'msg,
    ~disabled: bool=?,
    ~to_view: parts('msg) => Html.t('msg),
  ) => Html.t('msg);
```

### 2.2 Stateful submodels

```reason
type model;                       /* abstract; accessors below */
type msg;                         /* opaque to the parent */
type out_msg('value) = | Changed('value) | Opened | Closed /* … */;

let init: (~id: string, /* initial domain state */ unit) => model;
let update:
  (~items: array(item('value)) /* only if needed */, msg, model) =>
  (model, Cmd.t(msg), list(out_msg('value)));
let subscriptions: model => Sub.t(msg);

/* programmatic control, same shape as update */
let open_: model => (model, Cmd.t(msg));
let close: model => (model, Cmd.t(msg));

type parts('value, 'parent) = { /* attribute bundles + derived state */ };
let view:
  (
    ~model: model,
    ~items: array(item('value)),
    ~to_parent: msg => 'parent,
    ~to_view: parts('value, 'parent) => Html.t('parent),
  ) => Html.t('parent);
```

Rules:

1. **Messages carry intent, not precomputed results.** A keydown produces e.g.
   `Key(Arrow_down, modifiers, time_stamp)`; `update` computes the new index.
   Never compute the next state inside a view closure: renders are frame
   batched, so two events in one frame would both read the stale model.
2. **Collections are passed, not stored.** Items live in the parent; it passes
   the same array to `update` and `view`. Each item has a stable
   `key: string` (used for ids, selection, active descendant) — never use list
   index or polymorphic equality as identity. Items may change between
   messages; the component must clamp/repair (active key no longer present,
   disabled, empty list).
3. **Domain value** (selected key, checked, slider value, …) lives in the
   submodel with an accessor and a setter; every user change also emits an
   `out_msg`. Programmatic setters do **not** emit out messages.
4. **Attribute bundles are typed in the parent's message type** (`to_parent`
   applied with `Vdom.map_fact`). Each part documents which DOM events it owns;
   because one element holds one handler per event name, a part that owns
   `keydown` exposes `~on_key_down` style hooks instead of letting users stack
   handlers. Users append their own non-conflicting attrs freely.
5. **ids**: `init(~id)` takes a document-unique prefix; all part ids derive
   from it via `Ui_attr.part_id`. Item element ids derive from the item key,
   sanitised (`[A-Za-z0-9_-]`, other chars hex-escaped) so any key string is safe.
6. **Styling hooks** are presence data attributes (`data-open`, `data-closed`,
   `data-active`, `data-selected`, `data-checked`, `data-indeterminate`,
   `data-disabled`, `data-invalid`, `data-readonly`, `data-orientation="…"`,
   `data-placement="…"`, `data-enter`, `data-leave`, `data-transition`). Never
   emit inline styles except geometry (Anchor positions, Slider/Progress
   percentages via CSS custom properties like `--tea-slider-percent`).
7. **No CSS required for correctness.** Hidden parts use the `hidden` property
   or are not rendered; never rely on a stylesheet to hide closed content.
8. Disabled vs read-only follow APG: disabled parts are skipped by keyboard
   navigation unless the APG says they stay focusable (e.g. menu items stay
   reachable with `aria-disabled`); read-only allows navigation, forbids change.
9. **RTL**: horizontal arrow keys swap when `~dir=Rtl` is configured (Tabs,
   Slider, Radio Group, Calendar).
10. Every public item in a `.rei` has an odoc comment. Each `.rei` starts with
    a summary, the APG pattern link, a keyboard table and the data attributes.
11. No `Obj.magic`, no exceptions for control flow, no `Printf`, `Hashtbl`,
    `Fun.protect`, or `Format` (bundle size; see CHANGES.md). Prefer
    `String.equal`/pattern matches to polymorphic `==` on structured values.

### 2.3 Animation contract (used by every overlay)

`Animation` submodel states: `Closed | Entering | Open | Leaving`.
Parts get `data-closed` (initial/final frame), `data-enter`/`data-leave` and
`data-transition` exactly like Headless UI so either CSS transitions or
keyframes work. Entering: render with `data-closed data-enter data-transition`,
next frame drop `data-closed`, then `wait_for_animations` → Open. Leaving: add
`data-leave data-transition data-closed`, wait → `Closed`, emit
`out_msg Left` so the parent can unmount. The element stays mounted (and
`hidden` is false) during Leaving. Re-opening during Leaving reverses cleanly.
Must behave instantly with no CSS and with `prefers-reduced-motion` CSS that
removes transitions.

## 3. Tests (definition of done for every component)

1. **jsdom** (`test/ui/<name>/`, run by `dune build @runtest`): mounts a real
   program with `Tea.App.element`, dispatches real `KeyboardEvent`/`PointerEvent`
   /`MouseEvent`s, flushes frames, asserts ARIA attributes, ids/idrefs that
   resolve, data attributes, `document.activeElement`, out messages (record them
   in the test program's model), and zero leaked document listeners/locks after
   close and after `shutdown`. The shared jsdom setup lives in `test/ui/setup.js`.
2. **Real Chromium** (`test/ui-e2e/specs/<name>.spec.mjs`, run by
   `npm run test:e2e`): drives `examples/ui/<name>/` with Playwright
   (`playwright-core`, browser from `TEA_E2E_CHROMIUM` or Playwright's own),
   covering every row of the keyboard table, pointer use, focus return,
   `locator.ariaSnapshot()` for each state, and **axe-core with zero
   violations** in every state (open, closed, disabled, invalid…).
3. Pure helpers get exhaustive unit tests (edge cases: empty, all disabled,
   single item, wrap, RTL).
4. Gates: `./scripts/check-no-ml.sh && ./scripts/check-fmt.sh && dune build &&
   dune build @runtest --force && dune build @doc` (no warnings) and the
   component's e2e spec. Report the exact commands and their results.

## 4. Deliverables per component

- `src/ui/<name>.re` + `.rei`
- `test/ui/<name>/` (dune + Reason test), `test/ui-e2e/specs/<name>.spec.mjs`
- `examples/ui/<name>/` (dune, index.html with minimal but visible CSS that
  exercises the data attributes and transitions, main.re demonstrating every
  option including disabled/read-only/RTL where relevant)
- `docs/ui/<name>.md`: usage, full API, keyboard table, ARIA, data attributes,
  known limitations.
- Do **not** edit shared files (README, CHANGES, `src/tea/*`, other components,
  foundation helpers, root package.json). If one needs changing, stop and say
  exactly what and why in the report.

## 5. Task DAG

```
F0 foundation ──┬─ A1 form helpers (Input Textarea Checkbox Fieldset Switch Select)
                ├─ A2 display helpers (Meter Progress Nav Disclosure)
                ├─ B1 Animation ─────────┬─ D1 Dialog
                │                        ├─ D7 Toast
                ├─ B2 Anchor ────────────┼─ D2 Popover ──┐
                │                        ├─ D3 Menu      │
                │                        ├─ D4 Listbox   │
                │                        ├─ D5 Combobox  │
                ├─ B3 Hover Intent ──────┴─ D6 Tooltip   │
                ├─ C1 Radio Group + Tabs                 │
                ├─ C2 Slider                             │
                ├─ C3 Calendar ──────────────────────────┴─ E1 Date Picker
                ├─ C4 Drag and Drop + File Drop
                └─ C5 Virtual List
all ── Z1 showcase example, docs index, README/CHANGES, full e2e sweep, review
```

F0 also ships **Button** end to end as the reference implementation of §2–§4.
