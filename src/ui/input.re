module Html = Tea_html.Html;
type parts('msg) = {
  label: list(Html.attr('msg)),
  control: list(Html.attr('msg)),
  description: list(Html.attr('msg)),
  error: list(Html.attr('msg)),
};
let optional = (name, value) =>
  switch (value) {
  | None => Html.none
  | Some(value) => Html.attribute(name, value)
  };
let parts =
    (
      ~id,
      ~value,
      ~on_input,
      ~on_change=?,
      ~on_blur=?,
      ~has_description=false,
      ~has_error=false,
      ~invalid=false,
      ~required=false,
      ~disabled=false,
      ~readonly=false,
      ~name=?,
      ~type_="text",
      ~autocomplete=?,
      ~inputmode=?,
      (),
    ) => {
  let control_id = Ui_attr.part_id(id, "control");
  let description_id = Ui_attr.part_id(id, "description");
  let error_id = Ui_attr.part_id(id, "error");
  {
    label: [Html.id(Ui_attr.part_id(id, "label")), Html.for_(control_id)],
    control:
      [
        Html.id(control_id),
        Html.value(value),
        Html.disabled(disabled),
        Html.readonly(readonly),
        Html.required(required),
        optional("name", name),
        Html.type_(type_),
        optional("autocomplete", autocomplete),
        optional("inputmode", inputmode),
        Ui_attr.data_flag("invalid", invalid),
        Ui_attr.data_flag("disabled", disabled),
        Ui_attr.data_flag("readonly", readonly),
        disabled || readonly ? Html.none : Html.on_input(on_input),
        switch (on_change) {
        | Some(f) when !disabled && !readonly => Html.on_change(f)
        | _ => Html.none
        },
        switch (on_blur) {
        | Some(msg) => Html.on_blur(msg)
        | None => Html.none
        },
      ]
      @ Ui_attr.field_aria(~id, ~has_description, ~has_error, ~invalid),
    description: [Html.id(description_id), Html.hidden(!has_description)],
    error: [Html.id(error_id), Html.hidden(!has_error)],
  };
};
let view =
    (
      ~id,
      ~value,
      ~on_input,
      ~on_change=?,
      ~on_blur=?,
      ~has_description=false,
      ~has_error=false,
      ~invalid=false,
      ~required=false,
      ~disabled=false,
      ~readonly=false,
      ~name=?,
      ~type_="text",
      ~autocomplete=?,
      ~inputmode=?,
      ~to_view,
      (),
    ) =>
  to_view(
    parts(
      ~id,
      ~value,
      ~on_input,
      ~on_change?,
      ~on_blur?,
      ~has_description,
      ~has_error,
      ~invalid,
      ~required,
      ~disabled,
      ~readonly,
      ~name?,
      ~type_,
      ~autocomplete?,
      ~inputmode?,
      (),
    ),
  );
