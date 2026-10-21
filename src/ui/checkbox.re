module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
type parts('msg) = {
  control: list(Html.attr('msg)),
  label: list(Html.attr('msg)),
  description: list(Html.attr('msg)),
  hidden_input: Html.t('msg),
};
let next_checked = (~checked, ~indeterminate=false, ()) =>
  indeterminate || !checked;
let parts =
    (
      ~id,
      ~checked,
      ~on_change,
      ~indeterminate=false,
      ~native=true,
      ~disabled=false,
      ~has_description=false,
      ~name=?,
      ~value="on",
      (),
    ) => {
  let control_id = Ui_attr.part_id(id, "control");
  let click =
    Html.on(
      "click",
      V.Custom_optional(
        D.succeed({
          V.message: disabled ? None : Some(on_change),
          stop_propagation: false,
          prevent_default: true,
        }),
      ),
    );
  let key = down =>
    Ui_keyboard.custom(event =>
      if (Ui_keyboard.plain(event) && String.equal(event.key, " ")) {
        Some((!down && !disabled ? Some(on_change) : None, true));
      } else {
        None;
      }
    );
  {
    control: [
      Html.id(control_id),
      native ? Html.type_("checkbox") : Html.role("checkbox"),
      native ? Html.checked(checked) : Html.tabindex(disabled ? (-1) : 0),
      native
        ? Html.property("indeterminate", V.Bool(indeterminate))
        : Html.aria(
            "checked",
            indeterminate ? "mixed" : checked ? "true" : "false",
          ),
      native
        ? Html.disabled(disabled)
        : disabled ? Ui_attr.aria_bool("disabled", true) : Html.none,
      native
        ? switch (name) {
          | None => Html.none
          | Some(name) => Html.name(name)
          }
        : Html.none,
      native ? Html.value(value) : Html.none,
      Ui_attr.aria_ids("labelledby", [Ui_attr.part_id(id, "label")]),
      Ui_attr.aria_ids(
        "describedby",
        has_description ? [Ui_attr.part_id(id, "description")] : [],
      ),
      Ui_attr.data_flag("checked", checked),
      Ui_attr.data_flag("indeterminate", indeterminate),
      Ui_attr.data_flag("disabled", disabled),
      click,
      native ? Html.none : Html.on("keydown", V.Custom_optional(key(true))),
      native ? Html.none : Html.on("keyup", V.Custom_optional(key(false))),
    ],
    label: [
      Html.id(Ui_attr.part_id(id, "label")),
      native ? Html.for_(control_id) : Html.none,
    ],
    description: [
      Html.id(Ui_attr.part_id(id, "description")),
      Html.hidden(!has_description),
    ],
    hidden_input:
      switch (name) {
      | Some(name) when !native =>
        Html.input(
          [
            Html.type_("hidden"),
            Html.name(name),
            Html.value(value),
            Html.disabled(disabled || !checked),
          ],
          [],
        )
      | _ => Html.text("")
      },
  };
};
let view =
    (
      ~id,
      ~checked,
      ~on_change,
      ~indeterminate=false,
      ~native=true,
      ~disabled=false,
      ~has_description=false,
      ~name=?,
      ~value="on",
      ~to_view,
      (),
    ) =>
  to_view(
    parts(
      ~id,
      ~checked,
      ~on_change,
      ~indeterminate,
      ~native,
      ~disabled,
      ~has_description,
      ~name?,
      ~value,
      (),
    ),
  );
