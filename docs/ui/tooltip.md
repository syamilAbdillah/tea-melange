# Tooltip

`Tea_ui.Tooltip` / `Tea.Ui.Tooltip` implements the
[APG tooltip pattern](https://www.w3.org/WAI/ARIA/apg/patterns/tooltip/) and
[WCAG 1.4.13](https://www.w3.org/WAI/WCAG22/Understanding/content-on-hover-or-focus.html):
help is dismissible without moving focus, hoverable, and persistent while
hovered or keyboard-focused. Use a Popover/Dialog, not a tooltip, for links,
buttons or other interactive content.

## Integration

```reason
open Tea;
module T = Ui.Tooltip;
type msg = Tooltip(T.msg);
let tooltip = T.init(~id="project-help", ());
/* Store tooltip in the parent. In update: */
let update = (Tooltip(message), tooltip) => {
  let (tooltip, command, notifications) = T.update(message, tooltip);
  /* Handle notifications in the parent as appropriate. */
  ignore(notifications);
  (tooltip, Cmd.map(message => Tooltip(message), command));
};
let subscriptions = tooltip =>
  Sub.map(message => Tooltip(message), T.subscriptions(tooltip));
let view = tooltip => T.view(
  ~model=tooltip, ~to_parent=message => Tooltip(message),
  ~to_view=parts => Html.div([], [
    Html.label([Html.for_("project-help-trigger")], [Html.text("Project name")]),
    Html.input(parts.trigger @ [Html.type_("text")], []),
    Html.div(parts.tooltip, [Html.text("Use a name your team will recognize.")]),
  ]), (),
);
```

Always forward subscriptions and embedded messages, including during Leaving.
Keep the tooltip element mounted; `hidden` is true **exactly** when Animation
is Closed. Opening starts Anchor and shows Animation. Closing hides Animation
but leaves Anchor measuring/top-layer-active until Animation.Left stops it.
Reopening reverses Leaving. No focus movement, scroll locks, CSS-dependent
hiding, live region or portal is introduced. The returned command must be
mapped even though some embedded commands are currently empty.

The id prefix generates `id-trigger` and `id-tooltip`. Trigger owns id, its
chosen ARIA idref, focus, blur, keydown, pointerenter/leave/cancel and state
flags. Tooltip owns id, role, hidden, positioning, Animation flags and hover
handlers. Append nonconflicting attrs; use the key hook rather than stacking
handlers. The parent retains the trigger's native type, label, activation and
form behavior. `disabled` means disabling **the tooltip**, not its trigger.

## Names and descriptions

Description (default) adds `aria-describedby="id-tooltip"`. Supply the trigger's
own accessible name, normally a native visible label. Label adds
`aria-labelledby="id-tooltip"` for icon-only buttons; put `aria-hidden=true` on
the decorative icon. **Keep the text in the DOM even while hidden**: hidden,
directly referenced text participates in accessible-name/description computation.
Do not conditionally render it using `parts.visible`. Do not add a competing
`aria-label`, `aria-labelledby` or native `title`. Chromium tests inspect the
computed accessible name and description through CDP both open and hidden, not
merely the attribute strings. Label text should describe the action consistently
in every state; it is the actual control name, not optional supplementary help.

## Input and dismissal

| Input | Behavior |
| --- | --- |
| Tab / Shift+Tab | Native navigation, never prevented; keyboard focus opens immediately, blur closes |
| Escape, open trigger | Close without moving focus; prevent default and stop propagation |
| Escape, open hover-only help | Close from elsewhere without requiring focus movement; claim before enclosing overlays |
| Escape, closed | Propagate normally, e.g. close enclosing Dialog/Popover/Menu |
| Enter / Space | Native activation, no tooltip toggle |
| Other keys | Native behavior, or optional unmodified-key hook without prevention |
| Ctrl / Alt / Meta chords, IME | Unchanged |
| Mouse / pen hover | Open after delay (600ms default); leave both regions closes after 150ms grace |
| Pointer onto tooltip | Cancel shared closing timer; remain open with no reading timeout |
| Touch | Ignore hover and pointer-originated focus; no long-press implementation |

Keyboard focus persists even when the pointer leaves. Blur closes immediately
(starting any leave animation). Escape clears hover intent and focus persistence
so it does not reopen merely because the trigger still has focus. A fresh hover
entry or blur/refocus is a new interaction. Closed Escape is not prevented.
Requested `is_open` becomes false on dismissal; Leaving retains the visible
animation but no longer claims another Escape, allowing the enclosing overlay
to handle the next one.

Modality is tracked per immutable model. A subscription-owned **document capture
pointerdown** marks pointer modality before the browser's focus event, including
touch and clicks outside the trigger. A non-composing, unmodified key other than
a modifier itself restores keyboard modality. Thus Tab is recognized even when
it starts elsewhere; no `:focus-visible` heuristic misclassifies clicked text
inputs. Initial modality is keyboard so intentional programmatic/initial focus
opens help. Merely pressing a key after mouse-focus does not synthesize another
focus event. Subscriptions must be present for this policy to work.

Trigger Escape uses Ui_keyboard and stops propagation only when claimed. A
private document capture listener, active only while requested open, handles
Escape **outside** that trigger for hover-only dismissal. It ignores already
prevented events and shortcuts/composition. This prevents an enclosing dialog
from dismissing first even when its other control has focus. Trigger events
still use the trigger handler, verified against a document bubble listener.

## Warm-up groups

Store one immutable `Hover_intent.Group.t` in the parent. The parent already
knows which member produced `Tip(key, message)`, so notifications deliberately
stay `Opened | Closed(float)` rather than duplicating an id. Use a stable key
unique **within this group** (the tooltip's id prefix is a convenient choice).
Fold every notification using `T.track(~key, out, group)`:

- `Opened` calls `Group.record_open(~key, group)`.
- `Closed(time_stamp)` calls `Group.record_close(~key, ~time_stamp, group)`,
  removing that member and recording monotonic milliseconds in the same origin
  as `PointerEvent.timeStamp` / `performance.now()`.

An open member warms the group even during its 150ms close grace. After the
last member closes, entries less than `skip_delay_ms` later are warm; exactly
the boundary is cold. Zero/negative skip disables warm-up. Backwards timestamps
are cold when no member is open. Pass the newly committed group to the next
member's update; changing it does not shorten an already pending opening.

**One visible tooltip per group is a parent policy**, not global mutable state.
In the same update that opens a member, close every other visible member with
`close_immediately` and `Group.forget`. Check `is_visible`, not `is_open`: an
already Leaving member must disappear too. Unlike normal `close`, immediate
close skips the leave animation, clears hover/focus persistence, cancels timers
and animation waits, and stops Anchor. Commit all member models and the group
atomically before the next render. This exact pattern also appears in
`examples/ui/tooltip/main.re`:

```reason
module G = Ui.Hover_intent.Group;
type entry = {key: string, tip: T.model};
type msg = Tip(string, T.msg);
let group = G.init(~skip_delay_ms=1800, ());

/* In the parent's Tip(key, message) branch, pass only this group's members. */
let update_member = (~key, message, tips: list(entry), group) => {
  let current = List.find(entry => String.equal(entry.key, key), tips);
  let (tip, command, outs) = T.update(~group, message, current.tip);
  let group = List.fold_left((group, out) => T.track(~key, out, group), group, outs);
  let taking_over = List.mem(T.Opened, outs);
  List.fold_right((entry, (tips, commands, group)) => {
    if (String.equal(entry.key, key)) {
      ([{...entry, tip}, ...tips], commands, group);
    } else if (taking_over && T.is_visible(entry.tip)) {
      let (tip, command) = T.close_immediately(entry.tip);
      ([{...entry, tip}, ...tips],
       [Cmd.map(message => Tip(entry.key, message), command), ...commands],
       G.forget(~key=entry.key, group));
    } else {
      ([entry, ...tips], commands, group);
    };
  }, tips, ([], [Cmd.map(message => Tip(key, message), command)], group));
};
/* Commit returned tips/group and return Cmd.batch(commands). */
```

Programmatic `close`, `close_immediately`, disabling and unmount emit no user
notifications: call `G.forget(~key, group)` in that same parent update. Forget
does not invent a close timestamp or extend the skip window. Unmount also means
removing that member's subscriptions (do not retain an open key for removed
content). If using programmatic `open_`, explicitly record it with
`G.record_open(~key, group)` only if it actually opens, and apply the same
one-visible-member policy; a disabled member must not warm the group.

The example's icon-only document toolbar is a **labelled group of native
buttons**, not an ARIA toolbar promising a roving-arrow keyboard model. Only
its two members share a group, using the defaults: 600ms opening, 150ms closing
and an 8px placement gap. Moving between icons opens the next immediately and
hides the old one, without sacrificing the grace for crossing onto the tooltip.
Leaving the toolbar and re-entering within its 1800ms skip window is immediate;
after expiry the normal opening delay returns.

## Disabled native controls

Disabled native controls do not reliably deliver hover/focus events. Use a
wrapper as the trigger. Make it focusable so help is available by keyboard,
and remove the disabled child's pointer hit testing:

```reason
T.view(~model=tooltip, ~to_parent=message => Tooltip(message),
  ~to_view=parts => Html.div([], [
    Html.span(parts.trigger @ [Html.tabindex(0), Html.role("group"),
      Html.aria("label", "Publish unavailable")], [
      Html.button([Html.type_("button"), Html.disabled(true),
        Html.class_("disabled-control")], [Html.text("Publish")]),
    ]),
    Html.div(parts.tooltip, [Html.text("Add a project name before publishing.")]),
  ]), (),
);
/* .disabled-control { pointer-events: none; } */
```

The native control remains truly disabled. Do not mark the Tooltip model itself
disabled if its purpose is explaining that disabled control. The wrapper example
is checked with both real Chromium hover and Tab traversal.

## Position, arrow and styling

Default placement is `(Anchor.Top, Anchor.Center)`, with flip and 8px offset.
`top_layer=true` uses Anchor's native manual-popover promotion. Keep modal help
inside its Dialog subtree. The optional `arrow_size` computes the cross-axis
leading edge; `parts.arrow` supplies `aria-hidden=true`, `data-placement` and
`--tea-arrow-offset`. Render a decorative arrow and attach it to the resolved
side using CSS. `parts.position` exposes the full Anchor geometry. Anchor owns
its id and geometry, Animation alone owns hidden; conflicting duplicate facts
are filtered before either reaches the renderer.

Presence flags: trigger `data-open`, `data-closed`, `data-disabled`; tooltip
`data-open`, `data-disabled`, Animation's `data-closed`, `data-enter`,
`data-leave`, `data-transition`, and Anchor's `data-reference-hidden`.
`data-placement="top-center|bottom-center|..."` reflects resolved geometry.
No opacity/display CSS is necessary. The example animates a small translation,
preserves text contrast and honors `prefers-reduced-motion`.

## Full public API

```reason
type mode = Description | Label;
type model;
type msg;
type out_msg = Opened | Closed(float);
let init: (
  ~id: string, ~mode: mode=?, ~disabled: bool=?,
  ~open_delay_ms: int=?, ~close_delay_ms: int=?,
  ~placement: Anchor.placement=?, ~offset: float=?, ~flip: bool=?,
  ~shift_padding: option(float)=?, ~dir: Anchor.direction=?,
  ~top_layer: bool=?, ~arrow_size: float=?, unit
) => model;
let is_open: model => bool;
let is_visible: model => bool;
let phase: model => Animation.phase;
let position: model => option(Anchor.position);
let disabled: model => bool;
let open_: model => (model, Tea_core.Cmd.t(msg));
let close: model => (model, Tea_core.Cmd.t(msg));
let close_immediately: model => (model, Tea_core.Cmd.t(msg));
let track: (~key: string, out_msg, Hover_intent.Group.t) => Hover_intent.Group.t;
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));
let update: (~group: Hover_intent.Group.t=?, msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
type parts('parent) = {
  trigger: list(Tea_html.Html.attr('parent)),
  tooltip: list(Tea_html.Html.attr('parent)),
  arrow: list(Tea_html.Html.attr('parent)),
  open_: bool, visible: bool, position: option(Anchor.position),
};
let view: (
  ~model: model, ~to_parent: msg => 'parent,
  ~on_key_down: string => option('parent)=?,
  ~to_view: parts('parent) => Tea_html.Html.t('parent), unit
) => Tea_html.Html.t('parent);
```

Defaults not listed above: enabled, Ltr, `shift_padding=Some(8.)`, no arrow;
negative delays clamp to zero. Setters return no user notifications. `Closed`
reports dismissal intent, not animation completion; inspect `phase` for that.

## Verification and boundaries

```sh
dune build @test/ui/tooltip/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs tooltip
```

jsdom drives a real TEA program and deterministic timer boundaries: real DOM
ARIA/idrefs, focus, default prevention, Escape propagation, grouped delays,
hoverability/persistence, touch exclusion, disabled changes, latest-model batched
intents, every animation reversal, one-at-a-time takeover during close grace
and Leaving, keyed open/close notifications, programmatic/unmount forget, and
in-flight animation wait cleanup.
The harness independently rejects timer/MutationObserver/listener leaks. Tests
also inspect bundles for duplicate hidden/id/data facts. Chromium checks trusted
hover/touch, Tab and Shift+Tab, native activation, computed names/descriptions,
ARIA snapshots, real arrow/motion/top-layer state, disabled wrapper, modal Escape
layering, default-delay sibling switching and skip-window re-entry/expiry,
and unfiltered axe across closed/open/disabled/modal states.

No long-press, interactive tooltip content, geometric safe polygon, portal,
shadow-root/iframe id resolution or alternate-document modality tracking.
Anchor's unsupported-Popover-API fixed-position fallback cannot escape
transformed clipping ancestors. Geometry follows initial/resize/scroll reads,
not continuous transform polling. Animation inherits the shared 5000ms bounded
wait. The APG tooltip pattern itself is marked work in progress by APG; behavior
here follows its documented keyboard/focus rules plus WCAG hoverability.
The only private browser capabilities added are capture pointerdown modality
tracking and a cancellable, claimed outside-trigger Escape listener. No shared
browser/infrastructure modules were changed.
