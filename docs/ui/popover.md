# Popover

A non-modal floating disclosure, with optional dialog or region semantics.
There is **no dedicated APG popover pattern**. The trigger follows the
[APG disclosure pattern](https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/):
a native button, expanded state, and a controlled panel. Interactive panel
content uses its ordinary HTML semantics and natural Tab order. A named
[`dialog`](https://www.w3.org/TR/wai-aria-1.2/#dialog) can identify a distinct
interaction, but this is not the APG **modal** dialog pattern: no aria-modal,
focus trap, inert background, or scroll lock is introduced. In particular,
opening need not move focus. Use a modal Dialog when the task requires modality.

## Integrate

```reason
open Tea;
module P = Ui.Popover;
type msg = Popover(P.msg);
/* In parent init: */
let popover = P.init(~id="filters", ~role=P.Dialog("Filters"), ());
/* In parent update, store popover and map command; handle outs as desired: */
let update = (Popover(message), popover) => {
  let (popover, command, outs) = P.update(message, popover);
  /* outs: Opened or Closed(reason), not animation completion notifications. */
  ignore(outs);
  (popover, Cmd.map(message => Popover(message), command));
};
let subscriptions = popover =>
  Sub.map(message => Popover(message), P.subscriptions(popover));
let view = popover => P.view(
  ~model=popover, ~to_parent=message => Popover(message),
  ~to_view=parts => Html.div([], [
    Html.button(parts.trigger, [Html.text("Filters")]),
    Html.div(parts.panel, [
      Html.label([Html.for_("query")], [Html.text("Name")]),
      Html.input([Html.id("query")], []),
      Html.button(parts.close_button, [Html.text("Done")]),
    ]),
  ]), (),
);
```

Apply the trigger to a **native button** and the panel to a div. The panel must
be **immediately after the trigger in DOM order**. Never portal it or put other
focusable controls between them. Native top-layer promotion preserves that
order, so Tab from the trigger reaches the panel's first control. Do not use
positive tabindex to reorder the application or put autofocus in the panel.
Provide at least one tabbable control when expecting Tab to enter a panel.

The callback does not add a wrapper. Keep the panel mounted with its bundle,
or conditionally render it while `parts.rendered` is true. The latter includes
Leaving; never unmount at `parts.open_ == false`. Always forward subscriptions
through Leaving, including for programmatic changes. To remove the whole
component, remove its subscriptions in the same update. Shutdown performs the
same disposal. Parents choose a focus fallback when they remove an open
component or use the silent `close` function.

### Semantics and focus options

- `role=None` (default): no panel role; contents retain their ordinary semantics.
  Appropriate for ordinary navigation flyouts. It is **not** a menu: links use
  Tab, not menu arrow-key navigation.
- `role=Region`: a region named by its trigger (`aria-labelledby`). Useful when
  the additional landmark helps navigation; avoid excessive regions.
- `role=Dialog(label)`: a non-modal dialog with `aria-label=label`; callers must
  provide a nonempty, meaningful name. The trigger also gets
  `aria-haspopup="dialog"`. It never gets aria-haspopup=menu or aria-modal.

`focus_on_open=None` retains focus. `First_tabbable` chooses the first eligible
panel descendant in native tabindex order, excluding hidden/inert ancestors,
disabled controls, negative tabindex and closed details content. Checked radio
group members take precedence over their unchecked peers. `Element(id)` focuses
that explicit document-local target. Supply an existing, focusable panel target;
missing targets/empty panels are harmless and retain focus. Initial focus waits
only for Anchor's first visible measurement, **not** for animation completion.
This avoids focusing the initially visibility-hidden positioning surface.
Opening focus is cancelled on close/unmount or intervening focus movement,
even with focus-out dismissal disabled; restoring focus does not steal
a newer intentional destination.

`close_on_focus_out=true` closes when focus leaves the union of trigger and panel,
including programmatic focus and blur to body. Shift+Tab from the panel to the
trigger stays inside the union. `false` disables only this focus dismissal;
Escape, the trigger, close button and outside pointer dismissal still work.

## Keyboard and dismissal

| Input | Behavior |
| --- | --- |
| Enter on trigger | Native click toggles on keydown |
| Space on trigger | Native button prevents scroll; toggles on keyup |
| Tab | Natural next control; leaving trigger/panel closes by default, never prevents Tab |
| Shift+Tab | Natural previous control; first panel control returns to trigger and stays open |
| Escape in trigger or panel | Close, return focus to trigger; while expanded, prevent default and stop propagation so an enclosing Dialog or overlay is not also dismissed |
| Enter/Space on close button | Close with Programmatic reason and return focus to trigger |
| Other keys | Native behavior; optional non-preventing keydown hooks |
| Ctrl/Alt/Meta chords, IME | Unchanged; no Popover shortcut is claimed |
| Trigger pointer click | Toggle |
| Outside pointerdown | Close without moving focus; preserve the clicked destination |
| Focus outside / blur to body | Close by default without moving focus |

User notifications are `Opened` and `Closed(Escape | Outside | Focus_out |
Trigger | Programmatic)`, emitted at the logical change, not at the last animation
frame. The close-button helper uses Programmatic. Repeated dismissal when closed
emits nothing. `open_` and `close` are idempotent, silent programmatic setters;
`close` deliberately does not choose a focus destination. A form submit handler
can commit the application's values, close, and explicitly focus a logical target.

Trigger/panel bundles own keydown. Hooks are typed in the parent's message type,
receive only unhandled plain non-Tab keys, and never prevent defaults. Required
Escape and native trigger activation take precedence. Panel hooks also see
bubbled descendant events: do not override text editing or native form behavior.
Close-button bundles also own click, keydown and keyup through Button.

## Groups (parent-owned composition)

The runnable example's Plants and Tools flyouts implement Group semantics
without introducing another state owner or menu roles:

1. Give each member `group_triggers=["plants-trigger", "tools-trigger"]`.
   These peer triggers are part of the focus containment union, but **not** the
   outside-pointer union. Moving focus between their triggers keeps the current
   panel open, with no close/reopen flicker.
2. Forward a member's update and record its returned model/command.
3. If its outs include `Opened`, call `close` on every other member **in the same
   parent update**, record all returned models, and map/batch their commands.
   Do the same exclusivity pass for programmatic group opening. Do not call
   `init` to close peers; it would discard active animation/positioning state.
4. Render each trigger immediately before its own panel. Use an ordinary named
   `role=group` wrapper for independent buttons, not role=toolbar without a
   toolbar's roving keyboard model, and not role=menu.

Focus entering another member's content closes the previous member normally.
Group exclusivity concerns logical expanded state: an old panel may finish its
exit transition while the new one enters. Peer programmatic closes emit no fake
user notifications. See `examples/ui/popover/main.re` for the complete working
parent update, not just a partial pseudo-component.

## Animation, positioning and ownership

Popover embeds [Animation](animation.md) with id `prefix-panel` and
[Anchor](anchor.md) with anchor `prefix-trigger` and floating `prefix-panel`.
Open calls Anchor.start and Animation.show. Close calls only Animation.hide;
Anchor stays active/promoted throughout Leaving and stops only on Animation.Left.
Reopening Leaving reverses using the same submodels. Both command/subscription
streams are forwarded. Multiple same-frame activations are Toggle intents, not
precomputed booleans from a stale view.

Panel facts contain **one id and one hidden property**. Anchor's hidden fact is
removed before composing Animation.attrs, so hidden is true exactly in
Animation.Closed, and false throughout Entering/Open/Leaving. Anchor still owns
measurement visibility. `aria-expanded` describes logical openness (false during
Leaving); `aria-controls` is emitted only for the required rendered lifetime,
including Leaving, and removed once Closed, even with kept mounts.

Top layer defaults to true and uses Anchor's manual native popover. It escapes
`overflow:hidden` and transformed ancestors without moving DOM nodes. Native
Popover API absence uses Anchor's fixed-position fallback; that fallback cannot
escape transformed containing blocks. The host must provide a suitable DOM
location for such browsers. Do not mutate native popover state, owned attrs or
geometry externally. Anchor's options and geometry bounds apply unchanged;
Popover defaults offset to 8 CSS pixels. `position.arrow_offset` is the arrow's
leading cross-axis offset; the example renders a 12px arrow at that offset.

Escape and close-button dismissal use `Browser_events.restore_focus` with
`within=panel_id` during the restoring exit, after focusing the trigger. The
lease is deliberately **not** acquired on open: a lease active during an outside
click on nonfocusable content would restore the trigger and steal that click's
focus outcome. Programmatic closes, focus-out and outside dismissals never
request restoration. No scroll locks or focus traps are acquired.

### Data attributes

- `data-open`: trigger and panel while logically expanded.
- `data-closed`, `data-enter`, `data-leave`, `data-transition`: panel's Animation
  lifecycle; these describe transition endpoints, not just logical openness.
- `data-placement="side-alignment"`, `data-reference-hidden`: Anchor geometry.

Flags are presence attributes, never `"false"`. Only Anchor geometry/arrow
position use inline styles. Hidden/native top-layer state, not CSS, determines
closed visibility. The local example CSS demonstrates reversible scale motion
and honors reduced motion. Do not style hidden content back into view.

## Anchoring to another element

By default the panel is positioned against the trigger. Pass
`~anchor_id` to position it against another element instead, for example an
input group that contains the trigger (a date picker's text field plus its
"Choose date" button). It only changes geometry: the trigger keeps
`aria-expanded`/`aria-controls`, toggling and focus return, and outside-pointer
detection still uses the trigger and panel ids.

## Full public API

Available as `Tea_ui.Popover` / `Tea.Ui.Popover`. Every public item is documented
in `src/ui/popover.rei`.

```reason
type role = Dialog(string) | Region | None;
type focus_on_open = None | First_tabbable | Element(string);
type reason = Escape | Outside | Focus_out | Trigger | Programmatic;
type out_msg = Opened | Closed(reason);
type model;
type msg;
let init: (
  ~id: string, ~role: role=?, ~focus_on_open: focus_on_open=?,
  ~close_on_focus_out: bool=?, ~group_triggers: list(string)=?,
  ~placement: Anchor.placement=?, ~offset: float=?, ~flip: bool=?,
  ~shift_padding: option(float)=?, ~match_width: bool=?,
  ~dir: Anchor.direction=?, ~top_layer: bool=?, ~arrow_size: float=?,
  ~anchor_id: string=?, unit
) => model;
let trigger_id: model => string;
let panel_id: model => string;
let is_open: model => bool;
let should_render: model => bool;
let phase: model => Animation.phase;
let position: model => option(Anchor.position);
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
let open_: model => (model, Tea_core.Cmd.t(msg));
let close: model => (model, Tea_core.Cmd.t(msg));
type parts('parent) = {
  trigger: list(Tea_html.Html.attr('parent)),
  panel: list(Tea_html.Html.attr('parent)),
  close_button: list(Tea_html.Html.attr('parent)),
  open_: bool, rendered: bool, position: option(Anchor.position),
};
let view: (
  ~model: model, ~to_parent: msg => 'parent,
  ~on_trigger_key_down: string => option('parent)=?,
  ~on_panel_key_down: string => option('parent)=?,
  ~to_view: parts('parent) => Tea_html.Html.t('parent), unit
) => Tea_html.Html.t('parent);
```

Defaults: initially closed, role/focus None, close-on-focus-out true, no peers,
placement `(Anchor.Bottom, Anchor.Start)`, offset 8, flip true, shift padding
Some(8), match width false, LTR, top layer true, no arrow calculation. Ids derive
from the document-unique prefix: `-trigger`, `-panel`, `-close`. Use the close
bundle at most once; multiple custom close actions may instead call the parent's
silent close and choose focus explicitly. Geometry configuration is immutable;
retain the model for its mounted lifetime rather than reinitializing open panels.

## Private browser capabilities and limits

Focus containment remains a private lifecycle capability in Popover:

- Capture-phase focusin/focusout containment with separate Focus_out reasons,
  group trigger membership, and deferred active-element checks for blur to body.
  The public outside source conflates pointer/focus reasons when `focus=true`.

Initial/return focus uses the generation-keyed `Browser_events.focus` source.
It captures focus immediately when opening, waits for positioning readiness,
and only proceeds if that focus has not moved. Arbitrary programmatic openers
therefore retain the configured policy, while even with
`close_on_focus_out=false`, moving focus during positioning keeps the new
destination. Moving away and back also cancels the attempt. First-tabbable selection uses the
shared synchronous `Browser_dom.first_tabbable` query; ordering and exclusions
are documented in [foundation.md](foundation.md). No mutable DOM state, refs,
subscriptions or cancellation handles are stored in the model.

No Animation/Anchor changes or workarounds are required. Their bounds still
apply: document-local ids, no shadow-root/iframe traversal or fallback portal,
no continuously polled positioning during unrelated layout animation, and a
5000ms animation watchdog. Do not nest modal content here. This component does
not implement a global nested-overlay dismissal stack; parents coordinating
nested independent overlays must choose their dismissal policy.

## Verification

```sh
dune build @test/ui/popover/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs popover
```

jsdom mounts real programs and checks ARIA/idrefs, exact bundle fact ownership,
DOM hidden state, intent batching, both focus policies, tabbable exclusions,
native radio/tabindex priority, all dismissal reasons, focus cancellation,
Leaving reversal, kept/unmounted panels and teardown during an unresolved
animation. jsdom blocks focus inside manual popovers but lacks showPopover;
focus tests explicitly opt out of top layer rather than polyfilling browser
behavior. Chromium verifies real native promotion, every keyboard row, natural
Tab order, focus-out, outside focus noninterference, group exclusivity,
positioning flip, clipping/transform escape, animation reversal, ARIA snapshots
and unfiltered axe in closed, open, leaving and focus-policy states.
