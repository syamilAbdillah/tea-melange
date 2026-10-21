/** Construct an event fact from a handler description. */
let on: (string, Tea_vdom.Vdom.handler('msg)) => Tea_vdom.Vdom.fact('msg);
/** Decode a message and whether to prevent the browser default. */
let prevent_default_on:
  (string, Tea_core.Decode.t(('msg, bool))) => Tea_vdom.Vdom.fact('msg);
/** Decode a message and whether to stop propagation. */
let stop_propagation_on:
  (string, Tea_core.Decode.t(('msg, bool))) => Tea_vdom.Vdom.fact('msg);
/** Decode a message plus both event flags. */
let custom:
  (string, Tea_core.Decode.t(Tea_vdom.Vdom.custom('msg))) =>
  Tea_vdom.Vdom.fact('msg);
/** Decode [target.value] on input. */
let on_input: (string => 'msg) => Tea_vdom.Vdom.fact('msg);
/** Decode [target.value] on change. */
let on_change: (string => 'msg) => Tea_vdom.Vdom.fact('msg);
/** Decode [target.checked] on change. */
let on_check: (bool => 'msg) => Tea_vdom.Vdom.fact('msg);
/** Submit a message and always prevent default. */
let on_submit: 'msg => Tea_vdom.Vdom.fact('msg);
/** Decode the event [key]; [None] rejects the event without a message. */
let on_key_down: (string => option('msg)) => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [click]. */
let on_click: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [dblclick]. */
let on_double_click: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [blur]. */
let on_blur: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [focus]. */
let on_focus: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mousedown]. */
let on_mouse_down: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseup]. */
let on_mouse_up: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseenter]. */
let on_mouse_enter: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseleave]. */
let on_mouse_leave: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mousemove]. */
let on_mouse_move: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseover]. */
let on_mouse_over: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseout]. */
let on_mouse_out: 'msg => Tea_vdom.Vdom.fact('msg);
