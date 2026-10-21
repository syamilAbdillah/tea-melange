module Html = Tea_html.Html;
module D = Tea_core.Decode;
module V = Tea_vdom.Vdom;
type option_item = {
  key: string,
  label: string,
  disabled: bool,
};
type item =
  | Option(option_item)
  | Group({
      key: string,
      label: string,
      disabled: bool,
      items: array(option_item),
    });
type selection =
  | Single(option(string))
  | Multiple(array(string));
type parts('msg) = {
  label: list(Html.attr('msg)),
  control: list(Html.attr('msg)),
  description: list(Html.attr('msg)),
  error: list(Html.attr('msg)),
  options: list((string, Html.t('msg))),
};
let keys = items =>
  Array.fold_left(
    (acc, item) =>
      switch (item) {
      | Option(item) => [item.key, ...acc]
      | Group(group) =>
        Array.fold_left((acc, item) => [item.key, ...acc], acc, group.items)
      },
    [],
    items,
  );
let normalize = (~items, selection) => {
  let keys = keys(items);
  let exists = key => List.exists(String.equal(key), keys);
  switch (selection) {
  | Single(Some(key)) when exists(key) => Single(Some(key))
  | Single(_) => Single(None)
  | Multiple(values) =>
    Multiple(
      Array.of_list(
        List.rev(
          Array.fold_left(
            (acc, key) =>
              exists(key) && !List.exists(String.equal(key), acc)
                ? [key, ...acc] : acc,
            [],
            values,
          ),
        ),
      ),
    )
  };
};
let parts =
    (
      ~id,
      ~items,
      ~value,
      ~on_change,
      ~placeholder=?,
      ~has_description=false,
      ~has_error=false,
      ~invalid=false,
      ~required=false,
      ~disabled=false,
      ~name=?,
      (),
    ) => {
  let value = normalize(~items, value);
  let multiple =
    switch (value) {
    | Multiple(_) => true
    | _ => false
    };
  let selected = key =>
    switch (value) {
    | Single(Some(value)) => String.equal(key, value)
    | Single(None) => false
    | Multiple(values) => Array.exists(String.equal(key), values)
    };
  let placeholder_id = Ui_attr.part_id(id, "placeholder");
  /* selectedOptions is an HTMLCollection, not a JS array. Read its numeric
     properties only to enumerate: every selection is identified by value,
     never selectedIndex or its position in the supplied items. */
  let decode_option =
    D.map2(
      (id, value) => String.equal(id, placeholder_id) ? None : Some(value),
      D.field("id", D.string),
      D.field("value", D.string),
    );
  let rec decode_options = index =>
    index < 0
      ? D.succeed([])
      : D.map2(
          (value, rest) =>
            switch (value) {
            | None => rest
            | Some(value) => [value, ...rest]
            },
          D.field(string_of_int(index), decode_option),
          decode_options(index - 1),
        );
  let decoder =
    D.map(
      values =>
        on_change(
          multiple
            ? Multiple(Array.of_list(List.rev(values)))
            : Single(
                switch (List.rev(values)) {
                | [] => None
                | [value, ..._] => Some(value)
                },
              ),
        ),
      D.at(
        ["target", "selectedOptions"],
        D.and_then(
          length => decode_options(length - 1),
          D.field("length", D.int),
        ),
      ),
    );
  let option = (group_disabled, item: option_item) => (
    "option-" ++ item.key,
    Html.option(
      [
        Html.id(Ui_attr.item_id(id, item.key)),
        Html.value(item.key),
        Html.disabled(group_disabled || item.disabled),
        Html.selected(selected(item.key)),
      ],
      [Html.text(item.label)],
    ),
  );
  let options =
    Array.to_list(
      Array.map(
        item =>
          switch (item) {
          | Option(item) => option(false, item)
          | Group(group) => (
              "group-" ++ group.key,
              Html.keyed(
                "optgroup",
                [
                  Html.id(
                    Ui_attr.part_id(
                      id,
                      "group-" ++ Ui_attr.safe_key(group.key),
                    ),
                  ),
                  Html.attribute("label", group.label),
                  Html.disabled(group.disabled),
                ],
                Array.to_list(
                  Array.map(option(group.disabled), group.items),
                ),
              ),
            )
          },
        items,
      ),
    );
  let options =
    multiple
      ? options
      : [
        (
          "placeholder",
          Html.option(
            [
              Html.id(placeholder_id),
              Html.value(""),
              Html.selected(
                switch (value) {
                | Single(None) => true
                | _ => false
                },
              ),
              Html.disabled(true),
              Html.hidden(Option.is_none(placeholder)),
            ],
            [Html.text(Option.value(~default="", placeholder))],
          ),
        ),
        ...options,
      ];
  {
    label: [
      Html.id(Ui_attr.part_id(id, "label")),
      Html.for_(Ui_attr.part_id(id, "control")),
    ],
    control: [
      Html.id(Ui_attr.part_id(id, "control")),
      Html.multiple(multiple),
      Html.disabled(disabled),
      Html.required(required),
      switch (name) {
      | None => Html.none
      | Some(name) => Html.name(name)
      },
      invalid ? Ui_attr.aria_bool("invalid", true) : Html.none,
      Ui_attr.aria_ids(
        "describedby",
        (has_description ? [Ui_attr.part_id(id, "description")] : [])
        @ (has_error ? [Ui_attr.part_id(id, "error")] : []),
      ),
      Ui_attr.aria_ids(
        "errormessage",
        invalid && has_error ? [Ui_attr.part_id(id, "error")] : [],
      ),
      Ui_attr.data_flag("invalid", invalid),
      Ui_attr.data_flag("disabled", disabled),
      disabled ? Html.none : Html.on("change", V.Normal(decoder)),
    ],
    description: [
      Html.id(Ui_attr.part_id(id, "description")),
      Html.hidden(!has_description),
    ],
    error: [
      Html.id(Ui_attr.part_id(id, "error")),
      Html.hidden(!has_error),
    ],
    options,
  };
};
let view =
    (
      ~id,
      ~items,
      ~value,
      ~on_change,
      ~placeholder=?,
      ~has_description=false,
      ~has_error=false,
      ~invalid=false,
      ~required=false,
      ~disabled=false,
      ~name=?,
      ~to_view,
      (),
    ) =>
  to_view(
    parts(
      ~id,
      ~items,
      ~value,
      ~on_change,
      ~placeholder?,
      ~has_description,
      ~has_error,
      ~invalid,
      ~required,
      ~disabled,
      ~name?,
      (),
    ),
  );
