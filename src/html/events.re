module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
let on = V.on;
let prevent_default_on = (name, decoder) =>
  on(name, V.May_prevent_default(decoder));
let stop_propagation_on = (name, decoder) =>
  on(name, V.May_stop_propagation(decoder));
let custom = (name, decoder) => on(name, V.Custom(decoder));
let on_input = f =>
  on("input", V.Normal(D.map(f, D.at(["target", "value"], D.string))));
let on_change = f =>
  on("change", V.Normal(D.map(f, D.at(["target", "value"], D.string))));
let on_check = f =>
  on("change", V.Normal(D.map(f, D.at(["target", "checked"], D.bool))));
let on_submit = msg => prevent_default_on("submit", D.succeed((msg, true)));
let on_key_down = f =>
  on(
    "keydown",
    V.Normal(
      D.and_then(
        key =>
          switch (f(key)) {
          | Some(msg) => D.succeed(msg)
          | None => D.fail("Ignored key")
          },
        D.field("key", D.string),
      ),
    ),
  );
let on_click = msg => on("click", V.Normal(D.succeed(msg)));
let on_double_click = msg => on("dblclick", V.Normal(D.succeed(msg)));
let on_blur = msg => on("blur", V.Normal(D.succeed(msg)));
let on_focus = msg => on("focus", V.Normal(D.succeed(msg)));
let on_mouse_down = msg => on("mousedown", V.Normal(D.succeed(msg)));
let on_mouse_up = msg => on("mouseup", V.Normal(D.succeed(msg)));
let on_mouse_enter = msg => on("mouseenter", V.Normal(D.succeed(msg)));
let on_mouse_leave = msg => on("mouseleave", V.Normal(D.succeed(msg)));
let on_mouse_move = msg => on("mousemove", V.Normal(D.succeed(msg)));
let on_mouse_over = msg => on("mouseover", V.Normal(D.succeed(msg)));
let on_mouse_out = msg => on("mouseout", V.Normal(D.succeed(msg)));
