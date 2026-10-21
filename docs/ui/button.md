# Button

[APG button pattern](https://www.w3.org/WAI/ARIA/apg/patterns/button/).
A stateless button or toggle button. The parent owns disabled and pressed state.

```reason
open Tea;
let view = muted =>
  Ui.Button.button(
    ~id="mute",
    ~pressed=muted,
    ~on_press=Toggle_mute,
    ~attrs=[Html.class_("control")],
    [Html.text("Mute")],
  );
```

Keep a toggle's accessible name constant; `aria-pressed` communicates its state.
Always supply a visible label or an accessible name.

## API

```reason
type button_type = Button | Submit | Reset;
type parts('msg) = {root: list(Tea_html.Html.attr('msg))};

let parts:
  (~id: string, ~on_press: 'msg, ~native: bool=?,
   ~button_type: button_type=?, ~disabled: bool=?,
   ~focusable_when_disabled: bool=?, ~pressed: bool=?,
   ~on_key_down: string => option('msg)=?,
   ~on_key_up: string => option('msg)=?, unit) => parts('msg);

let view:
  (~id: string, ~on_press: 'msg, ~native: bool=?,
   ~button_type: button_type=?, ~disabled: bool=?,
   ~focusable_when_disabled: bool=?, ~pressed: bool=?,
   ~on_key_down: string => option('msg)=?,
   ~on_key_up: string => option('msg)=?,
   ~to_view: parts('msg) => Tea_html.Html.t('msg), unit) =>
  Tea_html.Html.t('msg);

let button:
  (~id: string, ~on_press: 'msg, ~as_: string=?,
   ~button_type: button_type=?, ~disabled: bool=?,
   ~focusable_when_disabled: bool=?, ~pressed: bool=?,
   ~attrs: list(Tea_html.Html.attr('msg))=?,
   ~on_key_down: string => option('msg)=?,
   ~on_key_up: string => option('msg)=?,
   list(Tea_html.Html.t('msg))) => Tea_html.Html.t('msg);
```

`view` follows the parts-callback convention; `button` is a direct-element
convenience. The final unit in `view` makes optional arguments erasable.
Defaults: native `button`, type `Button`, enabled, not pressed (ordinary button,
no `aria-pressed`), not focusable when disabled, no caller attributes/hooks.
Explicit `~pressed=false` creates an unpressed toggle. For custom rendering:

```reason
Ui.Button.view(
  ~id="action", ~native=false, ~on_press=Act,
  ~to_view=parts =>
    Html.div(parts.root @ [Html.class_("control")], [Html.text("Act")]),
  (),
);
```

`parts` is also available for direct attribute-bundle composition.
Parts own `click`, `keydown`, `keyup`, id, type/role, tabindex and state
attributes. Append only nonconflicting attributes. Hooks receive unhandled,
unmodified non-Tab keys, return an optional message and do not prevent default.
Required activation behavior takes precedence. No model-dependent next state is
computed in a handler: `on_press` is the parent's intent message.

## Keyboard

| Key | Native button | Neutral element |
| --- | --- | --- |
| Tab / Shift+Tab | Browser focus navigation | Browser focus navigation |
| Enter | Browser click activation | Activate on keydown; prevent default |
| Space down | Browser behavior | Prevent scroll, no message |
| Space up | Browser click activation | Activate; prevent default |
| Other keys | Unchanged, or hook | Unchanged, or hook |
| Ctrl/Alt/Meta chords, IME | Never intercepted | Never intercepted |

Disabled controls never activate, including pointer/assistive-technology clicks.
Native controls use `disabled` by default. With
`focusable_when_disabled=true`, they instead use `aria-disabled=true` and stay
in tab order; click and activation defaults are suppressed. Neutral controls
use `role=button`, `aria-disabled=true` and `tabindex=-1` unless that option
requests `tabindex=0`. Native buttons otherwise retain native semantics and
form behavior. Submit/reset types are opt-in.

## ARIA and styling

- `aria-disabled="true"` when disabled.
- `aria-pressed="true|false"` only when `pressed` is supplied.
- `data-disabled=""` when disabled; absent otherwise.
- `data-pressed=""` only when pressed; absent otherwise.

No inline styles, listeners outside the element, locks or subscriptions.

## Limits

Use `as_`/non-native parts only on neutral elements such as div or span, not
anchors with href or other native interactive controls. Do not nest interactive
descendants. Non-native buttons do not acquire native form behavior; prefer a
native button for submit/reset. The stateless helper implements the specified
keydown/keyup model without pointer-down or Space-arming state. Native controls
are recommended when native key-repeat/cancellation details matter.

Run `dune build @test/ui/button/runtest` and `npm run test:e2e -- button`.
The example demonstrates every option, both toggle states, form behavior and
composed hooks. Chromium checks focus order, keyboard/pointer activation,
ARIA snapshots and unfiltered axe in enabled, pressed, unpressed and disabled
states.
