/** Private keyboard decoders. Events during IME composition are never
    handled. Modifiers are left to each component's APG model: Alt+ArrowDown
    opens a combobox, Tab closes a menu. Handlers must check {!plain} before
    claiming a key that browsers or assistive technology bind with
    ctrl/alt/meta, and must never prevent default on Tab. */
/** A decoded intent, not a model-dependent navigation result. */
type event = {
  /** DOM key (Space is " "). */
  key: string,
  /** Shift modifier. */
  shift: bool,
  /** Control modifier. */
  ctrl: bool,
  /** Alt modifier. */
  alt: bool,
  /** Meta modifier. */
  meta: bool,
  /** IME composition in progress (also recognizes legacy keyCode 229). */
  composing: bool,
  /** Monotonic DOM event timestamp in milliseconds. */
  time_stamp: float,
  /** Auto-repeated keydown. */
  repeat: bool,
};
/** Decode prototype-backed DOM KeyboardEvent properties. */
let decoder: Tea_core.Decode.t(event);
/** No ctrl, alt or meta modifier (shift allowed). */
let plain: event => bool;
/** Not part of an IME composition; only such events reach handlers. */
let available: event => bool;
/** Reject unhandled and composing keys. Successful callbacks choose an optional
    message and preventDefault flag. Supports preventing Space scroll without
    dispatch; does not stop propagation. */
let custom:
  (event => option((option('msg), bool))) =>
  Tea_core.Decode.t(Tea_vdom.Vdom.custom(option('msg)));
/** Keydown fact: prevent default only on a handled key, dispatch its intent.
    Owns keydown; compose hooks inside the supplied function, not another fact. */
let on_key_down: (event => option('msg)) => Tea_html.Html.attr('msg);
