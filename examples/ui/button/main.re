open Tea;
module Button = Ui.Button;
type msg =
  | Pressed
  | Toggle_native
  | Toggle_custom
  | Disable(bool)
  | Hook
  | Submitted
  | Reset;
type model = {
  count: int,
  native_pressed: bool,
  custom_pressed: bool,
  disabled: bool,
  hooks: int,
  submitted: int,
  resets: int,
};
let update = (msg, model) => (
  switch (msg) {
  | Pressed => {
      ...model,
      count: model.count + 1,
    }
  | Toggle_native => {
      ...model,
      native_pressed: !model.native_pressed,
    }
  | Toggle_custom => {
      ...model,
      custom_pressed: !model.custom_pressed,
    }
  | Disable(disabled) => {
      ...model,
      disabled,
    }
  | Hook => {
      ...model,
      hooks: model.hooks + 1,
    }
  | Submitted => {
      ...model,
      submitted: model.submitted + 1,
    }
  | Reset => {
      ...model,
      resets: model.resets + 1,
    }
  },
  Cmd.none,
);
let attrs = [Html.class_("control")];
let view = model =>
  Html.main(
    [Html.id("button-example")],
    [
      Html.h1([], [Html.text("Buttons, without the guesswork.")]),
      Html.p(
        [],
        [
          Html.text(
            "Native semantics first. The same keyboard contract on a neutral element when you need one. Use Tab to explore; Enter and Space activate.",
          ),
        ],
      ),
      Html.label(
        [],
        [
          Html.input(
            [
              Html.id("disable"),
              Html.type_("checkbox"),
              Html.checked(model.disabled),
              Html.on_check(value => Disable(value)),
            ],
            [],
          ),
          Html.text(" Disable the action buttons"),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "actions-title")],
        [
          Html.h2([Html.id("actions-title")], [Html.text("Actions")]),
          Html.div(
            [Html.class_("controls")],
            [
              Button.button(
                ~id="native",
                ~disabled=model.disabled,
                ~on_press=Pressed,
                ~attrs,
                [Html.text("Native action")],
              ),
              Button.button(
                ~id="custom",
                ~as_="div",
                ~disabled=model.disabled,
                ~on_press=Pressed,
                ~attrs,
                ~on_key_down=key => key == "Home" ? Some(Hook) : None,
                ~on_key_up=key => key == "End" ? Some(Hook) : None,
                [Html.text("Neutral action")],
              ),
              Button.view(
                ~id="parts",
                ~native=false,
                ~on_press=Pressed,
                ~to_view=
                  parts =>
                    Html.span(
                      parts.root @ attrs,
                      [Html.text("Parts action")],
                    ),
                (),
              ),
            ],
          ),
          Html.p(
            [],
            [
              Html.text(
                "The neutral action also reports Home (keydown) and End (keyup) through composed hooks. Tab, shortcuts and composition remain untouched.",
              ),
            ],
          ),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "toggles-title")],
        [
          Html.h2(
            [Html.id("toggles-title")],
            [Html.text("Toggle buttons")],
          ),
          Html.div(
            [Html.class_("controls")],
            [
              Button.button(
                ~id="native-toggle",
                ~on_press=Toggle_native,
                ~pressed=model.native_pressed,
                ~attrs,
                [Html.text("Mute")],
              ),
              Button.button(
                ~id="custom-toggle",
                ~as_="span",
                ~on_press=Toggle_custom,
                ~pressed=model.custom_pressed,
                ~attrs,
                [Html.text("Pin")],
              ),
            ],
          ),
          Html.p(
            [],
            [
              Html.text(
                "Labels stay constant. aria-pressed and data-pressed carry the state.",
              ),
            ],
          ),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "disabled-title")],
        [
          Html.h2(
            [Html.id("disabled-title")],
            [Html.text("Disabled, with a focus policy")],
          ),
          Html.div(
            [Html.class_("controls")],
            [
              Button.button(
                ~id="native-disabled",
                ~on_press=Pressed,
                ~disabled=true,
                ~attrs,
                [Html.text("Disabled native")],
              ),
              Button.button(
                ~id="custom-disabled",
                ~as_="div",
                ~on_press=Pressed,
                ~disabled=true,
                ~attrs,
                [Html.text("Disabled neutral")],
              ),
              Button.button(
                ~id="native-focusable",
                ~on_press=Pressed,
                ~disabled=true,
                ~focusable_when_disabled=true,
                ~attrs,
                [Html.text("Focusable disabled native")],
              ),
              Button.button(
                ~id="custom-focusable",
                ~as_="span",
                ~on_press=Pressed,
                ~disabled=true,
                ~focusable_when_disabled=true,
                ~attrs,
                [Html.text("Focusable disabled neutral")],
              ),
            ],
          ),
        ],
      ),
      Html.form(
        [Html.aria("labelledby", "forms-title"), Html.on_submit(Submitted)],
        [
          Html.h2(
            [Html.id("forms-title")],
            [Html.text("Explicit form behavior")],
          ),
          Html.div(
            [Html.class_("controls")],
            [
              Button.button(
                ~id="submit",
                ~button_type=Button.Submit,
                ~on_press=Pressed,
                ~attrs,
                [Html.text("Submit form")],
              ),
              Button.button(
                ~id="reset",
                ~button_type=Button.Reset,
                ~on_press=Reset,
                ~attrs,
                [Html.text("Reset form")],
              ),
            ],
          ),
          Html.p(
            [],
            [
              Html.text(
                "All other native buttons use type=button. Submission is handled without navigation.",
              ),
            ],
          ),
        ],
      ),
      Html.node(
        "output",
        [Html.id("activity"), Html.aria("live", "polite")],
        [
          Html.text(
            "Activations: "
            ++ string_of_int(model.count)
            ++ ". Hooks: "
            ++ string_of_int(model.hooks)
            ++ ". Submissions: "
            ++ string_of_int(model.submitted)
            ++ ". Resets: "
            ++ string_of_int(model.resets)
            ++ ".",
          ),
        ],
      ),
    ],
  );
let () = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              count: 0,
              native_pressed: false,
              custom_pressed: false,
              disabled: false,
              hooks: 0,
              submitted: 0,
              resets: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=_ => Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  ignore(app);
};
