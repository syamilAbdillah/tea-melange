/** Private keyboard decoders. Never consume Tab, ctrl/alt/meta browser
    shortcuts, or composing events. Shift is left to the component's APG model. */
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
/** Whether a component may handle this event without swallowing a shortcut. */
let available: event => bool;
/** Reject unhandled/protected keys. Successful callbacks choose an optional
    message and preventDefault flag. Supports preventing Space scroll without
    dispatch; does not stop propagation. */
let custom:
  (event => option((option('msg), bool))) =>
  Tea_core.Decode.t(Tea_vdom.Vdom.custom(option('msg)));
/** Keydown fact: prevent default only on a handled key, dispatch its intent.
    Owns keydown; compose hooks inside the supplied function, not another fact. */
let on_key_down: (event => option('msg)) => Tea_html.Html.attr('msg);
