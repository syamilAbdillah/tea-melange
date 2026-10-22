# Dialog

[APG modal dialog](https://www.w3.org/WAI/ARIA/apg/patterns/dialog-modal/) and
[APG alertdialog](https://www.w3.org/WAI/ARIA/apg/patterns/alertdialog/).

An immutable stateful submodel backed by a **native `<dialog>`**, opened with
`Browser_dom.show_modal`. The browser's top layer makes the background inert;
there is no JavaScript document-wide focus trap. Native dialog support is
required. A nonmodal `show` variant is deliberately **not provided**.

## Integration

Keep the same submodel across openings. Map commands and subscriptions, and
apply the root bundle to exactly one native dialog. Always subscribe through
Leaving; keep the node mounted until `is_visible` becomes false (or keep it
mounted permanently: Closed sets `hidden=true`).

```reason
open Tea;
module D = Ui.Dialog;
type msg = Show | Dialog(D.msg);
type model = {dialog: D.model, last_value: string};
let init = () => (
  {dialog: D.init(~id="edit", ()), last_value: ""}, Cmd.none,
);
let update = (message, model) => {
  let (dialog, command, outs) = switch (message) {
  | Show =>
    let (dialog, command) = D.open_(model.dialog);
    (dialog, command, []);
  | Dialog(message) => D.update(message, model.dialog)
  };
  let last_value = List.fold_left((value, out) => switch (out) {
    | D.Closed => D.return_value(dialog)
    | _ => value
  }, model.last_value, outs);
  ({dialog, last_value}, Cmd.map(message => Dialog(message), command));
};
let subscriptions = model =>
  Sub.map(message => Dialog(message), D.subscriptions(model.dialog));
let view = model => Html.main([], [
  Ui.Button.button(~id="edit-opener", ~on_press=Show, [Html.text("Edit")]),
  D.view(~model=model.dialog, ~to_parent=message => Dialog(message),
    ~to_view=parts => Html.dialog(parts.root, [
      Html.h2(parts.title, [Html.text("Edit collection")]),
      Html.form([Html.attribute("method", "dialog")], [
        Html.button([Html.type_("submit"), Html.attribute("value", "saved")],
          [Html.text("Save")]),
      ]),
      Html.button(parts.close_button, [Html.text("Close")]),
    ]), ()),
]);
```

`Opened` is emitted after native showModal, not after the enter animation.
`Closed` follows completed leave. Programmatic `open_`/`close` do not emit
`Close_requested`; their eventual native-open/animation-close completion still
emits `Opened`/`Closed`. Idempotent calls and reversed, superseded transitions
do not generate duplicate lifecycle notifications. Reopening during Leaving
reverses Animation without closing, releasing scroll lock or returning focus.

### Guards and return values

`dismissible=true` (default) automatically starts leave on a user request.
`dismissible=false` emits `Close_requested(reason)` **without closing**. The
parent may ignore it, save data first, open a confirmation, or accept it by
calling `close`, mapping and returning that command. `close` always bypasses
the guard. Repeated requests while guarded remain separate intents; repeated
requests during Leaving are ignored.

For a destructive confirmation use
`D.init(~id="confirm", ~kind=D.Alertdialog("confirm-cancel"),
~dismissible=false, ())`. Render an enabled Cancel button with that exact id.
The parent handles `Close_requested(Close_button)` as cancellation and handles
a separate Confirm message by performing the destructive operation and calling
`close`. Escape can be accepted or refused explicitly. See the runnable example.

`method="dialog"` form submissions are intercepted **after native validation**,
so guards and exit animation work. The normalized submitter `formMethod`
override takes precedence over the form method. Other forms are untouched.
`return_value` becomes the submitter's value (empty without a submitter) before
`Close_requested(Native)` is emitted, including when a guard refuses closure.
It survives leave and is reset on a fresh open, not a leave reversal.

A native `close` event from external `.close(value)` is synchronized through
`Close_requested(Native)` and captures `returnValue`. Because that imperative
API has already closed the element and cannot be cancelled, Dialog reopens it
after render to apply the guard/leave policy. This necessarily has a brief
native modality/focus interruption; use the submodel API or intercepted forms
for uninterrupted animations and unsaved-changes guards. Stale queued close
events on an already-reopened element are ignored.

## Full public API

```reason
type kind = Modal | Alertdialog(string);
type name = Title | Label(string);
type labels = {close: string};
let default_labels: labels; /* {close: "Close"} */
type reason = Escape | Backdrop | Close_button | Native;
type out_msg = Opened | Close_requested(reason) | Closed;
type model; /* immutable, abstract */
type msg;   /* opaque intent/progress */

let init:
  (~id: string, ~kind: kind=?, ~initial_focus: string=?,
   ~dismissible: bool=?, ~close_on_backdrop: bool=?, unit) => model;
let phase: model => Animation.phase;
let is_open: model => bool;
let is_visible: model => bool;
let return_value: model => string;
let open_: model => (model, Tea_core.Cmd.t(msg));
let close: model => (model, Tea_core.Cmd.t(msg));
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
let subscriptions: model => Tea_core.Sub.t(msg);

type parts('parent) = {
  root: list(Tea_html.Html.attr('parent)),
  title: list(Tea_html.Html.attr('parent)),
  description: list(Tea_html.Html.attr('parent)),
  close_button: list(Tea_html.Html.attr('parent)),
  phase: Animation.phase,
  is_open: bool,
  is_visible: bool,
  return_value: string,
};
let view:
  (~model: model, ~name: name=?, ~has_description: bool=?,
   ~labels: labels=?, ~on_key_down: string => option('parent)=?,
   ~to_parent: msg => 'parent,
   ~to_view: parts('parent) => Tea_html.Html.t('parent), unit) =>
  Tea_html.Html.t('parent);
```

Defaults: `Modal`, no explicit initial focus, dismissible, backdrop closes for
Modal but **not** Alertdialog; `name=Title`, no description, English close label,
no keyboard hook. `is_open` means Entering/Open; `is_visible` also includes Leaving.
Every part bundle is in the **parent's message type**, not Dialog.msg.

## Keyboard

| Input | Behavior |
| --- | --- |
| Tab | Move forward among modal tabbables, wrapping inside; never prevent default |
| Shift+Tab | Move backward, wrapping inside; never prevent default |
| Escape while visible (including Leaving) | Request close of the focused/top dialog; prevent native closure and stop propagation |
| Enter / Space | Native button activation / form submission; never synthesized |
| Other keys | Unchanged; optional unmodified-key hook, without prevention |
| Ctrl/Alt/Meta chords / IME | Unchanged; no hook invocation |

Escape is intercepted on keydown as well as the dialog's `cancel` event.
`cancel` is always prevented and stopped; only claimed Escape keydown stops
propagation. A closed, mounted dialog leaves Escape unprevented and bubbling.
This avoids Chromium's repeated-Escape close watcher producing a noncancelable
cancel and bypassing a guard or closing during Leaving. `closedby="none"` also
disables newer native light-dismiss behavior; model intent owns dismissal.
The keydown hook receives neither claimed Escape nor Tab; a closed dialog may
forward unclaimed Escape to the hook. Avoid installing another
keydown handler on root; use `on_key_down` to compose application shortcuts.

Native modality excludes background focus targets. Chromium can temporarily
focus the viewport at a Tab boundary: a lifecycle-owned post-navigation frame
repairs **only body focus**, using first/last tabbable or root, without cancelling
Tab or stealing another element's focus. Nested dialogs dismiss only the top
one and restore focus into the first.

## Focus and scroll

Initial focus is applied synchronously in the same frame as `showModal()`, not
after entering completes and not in a later frame, so a Tab or click right after
opening is never pulled back:

1. Alertdialog's required least-destructive-action id takes priority.
2. Modal's `initial_focus` id, when supplied and inside root.
3. First tabbable in sequential tab order (positive tabindex first, then DOM
   order), skipping hidden, inert, disabled and unchecked radio-group members.
4. Root itself (`tabindex=-1`) if no tabbable exists.

Initial fallback and Tab-boundary repair share the synchronous
`Browser_dom.first_tabbable` / `last_tabbable` queries described in
[foundation.md](foundation.md). The selector does not schedule another frame.

A missing/outside/unfocusable explicit target falls back. Alertdialog callers
must render a valid enabled least-destructive action: the library cannot infer
which domain operation is safe. For long structured content, name a static
heading with `tabindex=-1`, and omit `has_description` so assistive technology
can navigate the structure rather than announce it as one large description.

`Browser_events.restore_focus(~key, ~within=root, ())` remembers the opener
before rendering, stays active throughout Leaving, and returns focus after
close only when appropriate. A removed opener is never refocused. Deliberate
focus on another connected element (including another modal) is respected.
The parent chooses any logical replacement when the opener has disappeared.
Shutdown/unmount close the native dialog and release subscriptions.

The keyed `Browser_events.scroll_lock` stays leased during Entering/Open/Leaving.
Nested locks are reference counted. The browser helper compensates scrollbar
width and restores prior html/body inline styles exactly. Do not change these
owned inline styles while locked. Long dialogs scroll internally; background
wheel scrolling remains locked. iOS-specific lock handling belongs to the
shared browser helper, not Dialog.

## Parts, ARIA and styling

With `id="edit"`, root is `edit-dialog`; title, description and close-button ids
are `edit-title`, `edit-description`, `edit-close`. Prefixes must be unique.

- Root owns id, tabindex, role (only explicitly for alertdialog), aria-modal,
  accessible name/description, returnValue, closedby, Animation flags/hidden,
  keydown, cancel, close, pointerdown/up/cancel, click and submit.
- Native dialog supplies role=dialog; Alertdialog explicitly sets
  `role="alertdialog"`. Both have `aria-modal="true"`.
- `name=Title` references the title part: **render it**. `name=Label(nonempty)`
  emits aria-label instead and permits omitting title. No dangling title idref.
- Render description **iff** `has_description=true`; otherwise no description
  idref is emitted. This explicit flag avoids dangling ids.
- Apply close_button to a native button. It owns id, type=button, aria-label
  (default `Close`, localizable through `labels`) and click.
- Append nonconflicting caller attributes. Do not override open, hidden, owned
  handlers or semantics. Do not synthesize a second role on the modal root.

Presence flags are inherited from [Animation](animation.md):

| Phase | Flags | hidden / native open |
| --- | --- | --- |
| Closed | data-closed | true / false |
| Entering, initial | data-closed, data-enter, data-transition | false / true after showModal |
| Entering, active | data-enter, data-transition | false / true |
| Open | none | false / true |
| Leaving | data-closed, data-leave, data-transition | false / true until leave completes |

No CSS is needed for correctness. Put padding in an inner content wrapper and
use `padding:0` on dialog. Backdrop dismissal requires pointerdown, pointerup
**and** click targeting root outside its bounding box; drags originating inside,
pointer cancellation, borders and empty space inside root do not dismiss.
Turn it off with `close_on_backdrop=false` or opt in for an alertdialog.

```css
dialog { padding: 0; }
dialog .content { padding: 1.5rem; }
dialog.animated { transform: translateY(0); }
dialog.animated[data-transition] { transition: transform 300ms ease; }
dialog.animated[data-closed] { transform: translateY(2rem); }
dialog.animated::backdrop { background: rgb(16 42 67 / .65); opacity: 1;
  transition: opacity 300ms ease; }
dialog.animated[data-closed]::backdrop { opacity: .8; }
@media (prefers-reduced-motion: reduce) {
  dialog.animated[data-transition], dialog.animated::backdrop { transition: none; }
}
```

## Bounds and verification

Native dialog support is required: Browser_dom's old-browser `open` fallback
is **not** a modality polyfill. There is no nonmodal, iframe, or shadow-root API.
Do not remove/replace an open root while retaining its subscriptions; remove the
submodel/subscriptions too. Parent code should close nested children before
closing their ancestor. As with Animation, waits snapshot current subtree
animations and have a 5000ms watchdog; intentionally paused/infinite or longer
animations may be cut short. Dialog/backdrop CSS transitions share root flags.

The example demonstrates form return values, a guarded destructive alertdialog,
nesting, long content, explicit/first/empty/missing initial focus, accessible
label without title, reduced motion, animated backdrop and reversal.

```sh
dune build @test/ui/dialog/runtest --force
TEA_E2E_CHROMIUM=/usr/bin/google-chrome node test/ui-e2e/run.mjs dialog
```

jsdom mounts a real Tea program and checks semantics/idrefs, parent-message
hooks, batched intents, all dismissal sources, guards, method overrides, focus,
unmount/shutdown, immutable snapshots, and cancellation of in-flight animation
watchdogs/observers. Chromium verifies native modality and both Tab boundaries,
keyboard/pointer behavior, actual scroll lock/layout, return focus (including
removed openers and deliberate transfer), nested dialogs, real reversible
transitions and repeated Escape during enter/leave. ARIA snapshots and unfiltered
axe cover closed, open, alert, enter and leave states.
