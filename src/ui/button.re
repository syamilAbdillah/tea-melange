module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
type button_type =
  | Button
  | Submit
  | Reset;
type parts('msg) = {root: list(Html.attr('msg))};
let parts =
    (
      ~id,
      ~on_press,
      ~native=true,
      ~button_type=Button,
      ~disabled=false,
      ~focusable_when_disabled=false,
      ~pressed=?,
      ~on_key_down=_ => None,
      ~on_key_up=_ => None,
      (),
    ) => {
  let key = (down, event: Ui_keyboard.event) => {
    switch (event.key) {
    | "Enter"
    | " " when disabled => Some((None, true))
    | "Enter" when !native && down => Some((Some(on_press), true))
    | " " when !native => Some((down ? None : Some(on_press), true))
    | "Enter"
    | " " => None
    | key =>
      Option.map(
        msg => (Some(msg), false),
        down ? on_key_down(key) : on_key_up(key),
      )
    };
  };
  {
    root: [
      Html.id(id),
      native
        ? Html.type_(
            switch (button_type) {
            | Button => "button"
            | Submit => "submit"
            | Reset => "reset"
            },
          )
        : Html.role("button"),
      native
        ? Html.disabled(disabled && !focusable_when_disabled) : Html.none,
      !native || disabled && focusable_when_disabled
        ? Html.tabindex(disabled && !focusable_when_disabled ? (-1) : 0)
        : Html.none,
      disabled ? Ui_attr.aria_bool("disabled", true) : Html.none,
      switch (pressed) {
      | None => Html.none
      | Some(pressed) => Ui_attr.aria_bool("pressed", pressed)
      },
      Ui_attr.data_flag("disabled", disabled),
      Ui_attr.data_flag("pressed", Option.value(~default=false, pressed)),
      Html.on(
        "click",
        V.Custom_optional(
          D.succeed({
            V.message: disabled ? None : Some(on_press),
            stop_propagation: false,
            prevent_default: disabled || !native,
          }),
        ),
      ),
      Html.on("keydown", V.Custom_optional(Ui_keyboard.custom(key(true)))),
      Html.on("keyup", V.Custom_optional(Ui_keyboard.custom(key(false)))),
    ],
  };
};
let view =
    (
      ~id,
      ~on_press,
      ~native=true,
      ~button_type=Button,
      ~disabled=false,
      ~focusable_when_disabled=false,
      ~pressed=?,
      ~on_key_down=_ => None,
      ~on_key_up=_ => None,
      ~to_view,
      (),
    ) =>
  to_view(
    parts(
      ~id,
      ~on_press,
      ~native,
      ~button_type,
      ~disabled,
      ~focusable_when_disabled,
      ~pressed?,
      ~on_key_down,
      ~on_key_up,
      (),
    ),
  );
let button =
    (
      ~id,
      ~on_press,
      ~as_="button",
      ~button_type=Button,
      ~disabled=false,
      ~focusable_when_disabled=false,
      ~pressed=?,
      ~attrs=[],
      ~on_key_down=_ => None,
      ~on_key_up=_ => None,
      children,
    ) => {
  let parts =
    parts(
      ~id,
      ~on_press,
      ~native=String.equal(as_, "button"),
      ~button_type,
      ~disabled,
      ~focusable_when_disabled,
      ~pressed?,
      ~on_key_down,
      ~on_key_up,
      (),
    );
  Html.node(as_, parts.root @ attrs, children);
};
