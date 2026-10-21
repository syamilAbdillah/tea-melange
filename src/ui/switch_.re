module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
type parts('msg) = {
  root: list(Html.attr('msg)),
  label: list(Html.attr('msg)),
  description: list(Html.attr('msg)),
  hidden_input: Html.t('msg),
};
let parts =
    (
      ~id,
      ~checked,
      ~on_change,
      ~disabled=false,
      ~has_description=false,
      ~name=?,
      ~value="on",
      (),
    ) => {
  let control_id = Ui_attr.part_id(id, "control");
  {
    root: [
      Html.id(control_id),
      Html.type_("button"),
      Html.role("switch"),
      Ui_attr.aria_bool("checked", checked),
      Html.disabled(disabled),
      Ui_attr.aria_ids("labelledby", [Ui_attr.part_id(id, "label")]),
      Ui_attr.aria_ids(
        "describedby",
        has_description ? [Ui_attr.part_id(id, "description")] : [],
      ),
      Ui_attr.data_flag("checked", checked),
      Ui_attr.data_flag("disabled", disabled),
      Html.on(
        "click",
        V.Custom_optional(
          D.succeed({
            V.message: disabled ? None : Some(on_change),
            stop_propagation: false,
            prevent_default: disabled,
          }),
        ),
      ),
    ],
    label: [Html.id(Ui_attr.part_id(id, "label")), Html.for_(control_id)],
    description: [
      Html.id(Ui_attr.part_id(id, "description")),
      Html.hidden(!has_description),
    ],
    hidden_input:
      switch (name) {
      | Some(name) =>
        Html.input(
          [
            Html.type_("hidden"),
            Html.name(name),
            Html.value(value),
            Html.disabled(disabled || !checked),
          ],
          [],
        )
      | None => Html.text("")
      },
  };
};
let view =
    (
      ~id,
      ~checked,
      ~on_change,
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
      ~disabled,
      ~has_description,
      ~name?,
      ~value,
      (),
    ),
  );
