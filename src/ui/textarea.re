module Html = Tea_html.Html;
type parts('msg) =
  Input.parts('msg) = {
    label: list(Html.attr('msg)),
    control: list(Html.attr('msg)),
    description: list(Html.attr('msg)),
    error: list(Html.attr('msg)),
  };
/* An Input without type/autocomplete/inputmode, plus rows. */
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
      ~rows=3,
      (),
    ) => {
  let base =
    Input.parts(
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
      (),
    );
  {
    ...base,
    control:
      Ui_attr.without("type", base.control)
      @ [Html.attribute("rows", string_of_int(max(1, rows)))],
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
      ~rows=3,
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
      ~rows,
      (),
    ),
  );
