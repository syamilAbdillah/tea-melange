# Animation

A reversible, immutable lifecycle submodel for overlays. It does not implement
an interactive widget, focus management, a dialog, or an ARIA role. See the
[APG patterns](https://www.w3.org/WAI/ARIA/apg/patterns/), especially
[modal dialog focus rules](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/).
The example uses [disclosure controls](https://www.w3.org/WAI/ARIA/apg/patterns/disclosure/).

## Full API

```reason
type phase = Closed | Entering | Open | Leaving;
type model;  /* immutable, abstract */
type msg;    /* opaque asynchronous progress */
type out_msg = Entered | Left;
let init: (~id: string, ~open_: bool, ~appear: bool=?, unit) => model;
let phase: model => phase;
let show: model => (model, Tea_core.Cmd.t(msg));
let hide: model => (model, Tea_core.Cmd.t(msg));
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);
let attrs: model => list(Tea_html.Html.attr('msg));
let is_visible: model => bool;
let should_render: model => bool;
```

`id` names the exact element being animated, not a prefix. Set its `Html.id`
yourself. Its descendants' animations are awaited too. `attrs` has no event
handlers, so its message type is freely the parent's message type without
`to_parent`. Map commands and subscriptions into the parent normally. There is
no wrapper/view: apply the bundle to the overlay's existing element.

**Always forward `subscriptions`**, including during Leaving. It starts all
asynchronous work, including initial appear. Commands are currently `Cmd.none`;
return/map them anyway. This subscription-owned design allows initial mount
without an additional start command and suppresses callbacks on shutdown or
when the parent removes the submodel. Keep the same model across reversals;
do not reset it with `init` on each open.

`appear` defaults to false. Initially closed models start Closed, regardless of
appear. Initially open models start Open without an event, unless appear is
true, in which case they start Entering and eventually emit Entered.

`is_visible` and `should_render` both mean phase != Closed. They describe the
lifecycle, not computed CSS visibility or whether the parent permits input.
Repeated show during Entering/Open and hide during Leaving/Closed are no-ops.
Only a completed current transition emits Entered/Left, once. A reversed or
superseded transition emits neither. Completion due to a missing element is
still completion, not an error that can strand a parent's state.

## Frame and attribute protocol

All flags are presence attributes, e.g. `data-enter=""`, not boolean strings.

| Phase/stage | data-closed | data-enter | data-leave | data-transition | hidden |
| --- | --- | --- | --- | --- | --- |
| Closed | present | — | — | — | true |
| Entering, initial | present | present | — | present | false |
| Entering, active | — | present | — | present | false |
| Open | — | — | — | — | false |
| Leaving | present | — | present | present | false |

1. `show` commits Entering with the initial flags. The parent mounts the element.
2. A **double requestAnimationFrame**, through the shared browser scheduler,
   crosses a paint. A single rAF would run in the same pre-paint callback batch
   as Tea's opening draw and could collapse the closed/open CSS endpoints.
3. The second callback dispatches generation-tagged progress. Tea batches that
   model's next render, which removes data-closed. Thus the initial flags survive
   a paint; the active attributes appear on the subsequent Tea render, not by
   imperatively mutating the DOM in the callback.
4. A lifecycle-gated post-render callback starts
   `Browser_dom.wait_for_animations(id)`, whose own post-render callback snapshots
   animations. The extra gate avoids starting obsolete waits during same-frame
   reversals. The wait settles on finished/cancelled animations, missing elements
   and element removal. Then update commits Open and emits Entered.
5. `hide` immediately commits Leaving. After its flags render, the same gated
   wait finishes, commits Closed and emits Left. Only now is unmounting safe.

Generations are arbitrary-length decimal strings, not wrapping machine
integers. Every frame handoff and wait completion carries its generation;
stale and duplicate messages are ignored even if replayed directly to update.
Subscription identities include a length-prefixed id, generation and stage.
Reversal stops the old subscription and starts the new one. Callback gates
suppress delivery after stop; they also prevent scheduling a second enter
frame when already stopped.

No CSS/getAnimations/animations, or reduced-motion styles removing animations,
complete over the next frames. There is deliberately no synchronous CSS-free
fast path: it would break identical parent lifecycle/focus behavior. No inline
styles or new browser bindings are used.

## Overlay integration

Focus as soon as Entering starts, **not** on Entered: waiting for visual motion
would leave keyboard/assistive-technology users outside an already-open overlay.
Schedule focus with `Browser_dom.focus_cmd` in the same update that calls show;
it resolves after the mounting draw. Subscribe to `restore_focus` while the
panel should render, using `~within` so a kept-mounted leaving subtree can
return focus. Keep it through Leaving; stopping on Left restores focus after
the closing draw. A different connected element intentionally focused by the
user is not stolen from. If the opener was removed, the parent must choose a
logical fallback.

Here is a complete program showing the lifecycle/focus/unmount composition for
an ordinary **nonmodal region**, deliberately not a partial dialog. It needs
only `<div id="app"></div>` and the standard Tea import map. A real Dialog/Menu/
Listbox must add its own complete APG roles, keyboard model, dismissal, focus
containment and interaction policy. Never mark this region aria-modal.

```reason
open Tea;
module A = Ui.Animation;
type msg = Show | Hide | Animation(A.msg);
type model = {animation: A.model, entered: int, left: int};
let update = (message, model) => {
  switch (message) {
  | Show =>
    let starting = switch (A.phase(model.animation)) {
      | A.Closed | A.Leaving => true
      | A.Entering | A.Open => false
    };
    let (animation, command) = A.show(model.animation);
    ({...model, animation}, Cmd.batch([
      Cmd.map(message => Animation(message), command),
      starting ? Browser_dom.focus_cmd("details-close") : Cmd.none,
    ]));
  | Hide =>
    let (animation, command) = A.hide(model.animation);
    ({...model, animation}, Cmd.map(message => Animation(message), command));
  | Animation(message) =>
    let (animation, command, outs) = A.update(message, model.animation);
    let model = List.fold_left((model, out) => switch (out) {
      | A.Entered => {...model, entered: model.entered + 1}
      | A.Left => {...model, left: model.left + 1}
    }, {...model, animation}, outs);
    (model, Cmd.map(message => Animation(message), command));
  };
};
let view = model => Html.main([], [
  Html.h1([], [Html.text("Details")]),
  Ui.Button.button(~id="details-show", ~on_press=Show,
    [Html.text("Show details")]),
  A.should_render(model.animation)
    ? Html.div(A.attrs(model.animation) @ [
        Html.id("details"), Html.class_("details"),
        Html.role("region"), Html.aria("label", "More details"),
      ], [
        Html.p([], [Html.text("Additional information.")]),
        Ui.Button.button(~id="details-close", ~on_press=Hide,
          [Html.text("Close details")]),
      ])
    : Html.text(""),
]);
let subscriptions = model => Sub.batch([
  Sub.map(message => Animation(message), A.subscriptions(model.animation)),
  A.should_render(model.animation)
    ? Browser_events.restore_focus(~key="details", ~within="details", ())
    : Sub.none,
]);
let app = App.element(
  ~init=() => ({animation: A.init(~id="details", ~open_=false, ()),
    entered: 0, left: 0}, Cmd.none),
  ~update, ~view, ~subscriptions, ~node="#app", ~flags=(), (),
);
```

The view unmounts precisely when Left sets Closed; the output also gives the
parent an explicit completion hook. Alternatively always render the node and
apply attrs: Closed's hidden property ensures CSS-independent correctness.
Do not use `display:none`, `hidden`, or unmounting during Leaving. For a native
modal dialog, keep the dialog open/top-layer until Left, then close it and
release modal resources; otherwise the exit animation cannot run. Scope any
scroll/focus locks to the appropriate overlay lifetime, not to Animation itself.
Animation intentionally does not assume all overlays have the same policy.

## CSS

Transitions on text-bearing content can animate geometry without reducing text
contrast. Neither stylesheet is required for lifecycle correctness:

```css
.details { transform: translateY(0); }
.details[data-transition] { transition: transform 180ms ease; }
.details[data-closed] { transform: translateY(.75rem); }
@media (prefers-reduced-motion: reduce) {
  .details[data-transition] { transition: none; }
}
```

Equivalent keyframes (do not restart the enter animation when data-closed drops):

```css
.details[data-enter] { animation: details-in 180ms ease both; }
.details[data-leave] { animation: details-out 180ms ease both; }
@keyframes details-in {
  from { transform: translateY(.75rem); }
  to { transform: translateY(0); }
}
@keyframes details-out {
  from { transform: translateY(0); }
  to { transform: translateY(.75rem); }
}
@media (prefers-reduced-motion: reduce) {
  .details[data-transition] { animation: none; }
}
```

Opacity fades to zero are appropriate for decorative surfaces, but fading text
can violate contrast at intermediate frames. The runnable example intentionally
uses a small .85-to-1 opacity range with adequate contrast in every frame;
hidden/unmounting, not opacity, makes Closed inaccessible. It demonstrates both
CSS techniques, no animation CSS, reduced motion, appear on/off, kept mounts,
unmounting, repeated show/hide, focus return and rapid reversal.

## Keyboard and ARIA

| Input | Animation behavior |
| --- | --- |
| Tab / Shift+Tab | Unchanged; never prevented |
| Enter / Space | Unchanged; native example buttons activate normally |
| Arrows / Escape / other keys | Unchanged; embedding overlay owns its APG model |
| Ctrl/Alt/Meta chords / IME | Unchanged; no key listeners |
| Pointer | Unchanged; parent calls show/hide on intent |

No role, ARIA state, live region or ids are added by Animation. During Leaving,
content remains mounted and not hidden. The parent owns expanded state, focus,
input suppression and any inert/ARIA policy, consistently with its widget.

## Bounds and tests

`Browser_dom.wait_for_animations` snapshots the current subtree, not animations
created later. It has a **5000ms watchdog**, so intentionally infinite/paused or
longer animations are bounded rather than hanging an overlay. Completion may
therefore precede a deliberately longer animation. Unsupported getAnimations
is the CSS-free path. Id lookup is document-local, not shadow-root/iframe aware.

Stopping a subscription suppresses results immediately, including shutdown.
Frames use the shared scheduler's noncancellable callback API and become
harmless no-ops after stopping. An in-flight wait uses
`Browser_dom.watch_animations`, whose canceller releases the observer and
watchdog at once, so a stopped Animation leaves no timers, observers or
listeners behind.

Run:

```sh
dune build @test/ui/animation/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs animation
```

jsdom tests use manual frames and stubbed getAnimations, covering all phases,
attribute bundles, appear, idempotence, both reversals at every handoff, stale/
duplicate tokens, decimal carries, 101 same-frame intents, cancellation,
multiple subtree animations, missing/removed elements, focus noninterference,
unmount and shutdown without leaked listeners. Chromium uses real transitions
and keyframes, samples flags/frame boundaries/computed opacity, checks actual
CSS end events and completion timing, pauses midflight only for deterministic
accessibility assertions, exercises reversal/removal/reduced motion, checks ARIA
snapshots/focus and runs unfiltered axe in every lifecycle phase.
