module Html = Tea_html.Html;
type parts('msg) = {
  root: list(Html.attr('msg)),
  legend: list(Html.attr('msg)),
  description: list(Html.attr('msg)),
  disabled: bool,
};
let parts = (~id, ~disabled=false, ~invalid=false, ~has_description=false, ()) => {
  root: [
    Html.id(Ui_attr.part_id(id, "root")),
    Html.disabled(disabled),
    Ui_attr.data_flag("disabled", disabled),
    Ui_attr.data_flag("invalid", invalid),
    Ui_attr.aria_ids(
      "describedby",
      has_description ? [Ui_attr.part_id(id, "description")] : [],
    ),
  ],
  legend: [Html.id(Ui_attr.part_id(id, "legend"))],
  description: [
    Html.id(Ui_attr.part_id(id, "description")),
    Html.hidden(!has_description),
  ],
  disabled,
};
let view =
    (
      ~id,
      ~disabled=false,
      ~invalid=false,
      ~has_description=false,
      ~to_view,
      (),
    ) =>
  to_view(parts(~id, ~disabled, ~invalid, ~has_description, ()));
