module Html = Tea_html.Html;
type parts('msg) = {
  button: list(Html.attr('msg)),
  panel: list(Html.attr('msg)),
  mounted: bool,
};
let parts = (~id, ~open_, ~on_toggle, ~keep_mounted=true, ()) => {
  let mounted = open_ || keep_mounted;
  let state = [
    Ui_attr.data_flag("open", open_),
    Ui_attr.data_flag("closed", !open_),
  ];
  {
    button:
      Button.parts(
        ~id=Ui_attr.part_id(id, "button"),
        ~on_press=on_toggle,
        (),
      ).
        root
      @ [
        Ui_attr.aria_bool("expanded", open_),
        mounted
          ? Ui_attr.aria_ids("controls", [Ui_attr.part_id(id, "panel")])
          : Html.none,
      ]
      @ state,
    panel:
      [Html.id(Ui_attr.part_id(id, "panel")), Html.hidden(!open_)] @ state,
    mounted,
  };
};
let view = (~id, ~open_, ~on_toggle, ~keep_mounted=true, ~to_view, ()) =>
  to_view(parts(~id, ~open_, ~on_toggle, ~keep_mounted, ()));
