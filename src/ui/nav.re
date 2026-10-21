module Html = Tea_html.Html;
type name =
  | Label(string)
  | Labelledby;
type current_token =
  | Page
  | Step
  | Location
  | Date
  | Time
  | True;
type item('msg) = {
  key: string,
  link: list(Html.attr('msg)),
};
type parts('msg) = {
  root: list(Html.attr('msg)),
  label: list(Html.attr('msg)),
  list: list(Html.attr('msg)),
  items: array(item('msg)),
};
let token = token =>
  switch (token) {
  | Page => "page"
  | Step => "step"
  | Location => "location"
  | Date => "date"
  | Time => "time"
  | True => "true"
  };
let parts = (~id, ~name, ~keys, ~current=?, ~current_token=Page, ()) => {
  let seen = ref([]);
  let items =
    Array.map(
      key => {
        if (List.exists(String.equal(key), seen^)) {
          invalid_arg("Nav: duplicate key");
        };
        seen := [key, ...seen^];
        let selected =
          switch (current) {
          | Some(current) => String.equal(current, key)
          | None => false
          };
        {
          key,
          link: [
            Html.id(Ui_attr.item_id(id, key)),
            selected
              ? Html.aria("current", token(current_token)) : Html.none,
            Ui_attr.data_flag("current", selected),
          ],
        };
      },
      keys,
    );
  let naming =
    switch (name) {
    | Label(label) =>
      if (String.length(String.trim(label)) == 0) {
        invalid_arg("Nav: accessible name must not be empty");
      };
      Html.aria("label", label);
    | Labelledby =>
      Ui_attr.aria_ids("labelledby", [Ui_attr.part_id(id, "label")])
    };
  {
    root: [Html.id(id), naming],
    label: [Html.id(Ui_attr.part_id(id, "label"))],
    list: [Html.id(Ui_attr.part_id(id, "list"))],
    items,
  };
};
let view = (~id, ~name, ~keys, ~current=?, ~current_token=Page, ~to_view, ()) => {
  let parts = parts(~id, ~name, ~keys, ~current?, ~current_token, ());
  Html.nav(parts.root, [to_view(parts)]);
};
