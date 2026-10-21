module V = Tea_vdom.Vdom;
let attribute = V.attribute;
let property = V.property;
let none = V.Attribute("", "");
let class_ = value => attribute("class", value);
let classList = entries =>
  class_(
    String.concat(
      " ",
      List.filter_map(
        ((name, enabled)) => enabled ? Some(name) : None,
        entries,
      ),
    ),
  );
let style = V.style;
let data = (key, value) => attribute("data-" ++ key, value);
let aria = (key, value) => attribute("aria-" ++ key, value);
let id = value => attribute("id", value);
let href = value => attribute("href", value);
let src = value => attribute("src", value);
let alt = value => attribute("alt", value);
let type_ = value => attribute("type", value);
let placeholder = value => attribute("placeholder", value);
let name = value => attribute("name", value);
let for_ = value => attribute("for", value);
let title = value => attribute("title", value);
let role = value => attribute("role", value);
let width = value => attribute("width", string_of_int(value));
let height = value => attribute("height", string_of_int(value));
let tabindex = value => attribute("tabindex", string_of_int(value));
let value = value => property("value", V.String(value));
let checked = value => property("checked", V.Bool(value));
let selected = value => property("selected", V.Bool(value));
let disabled = value => property("disabled", V.Bool(value));
let hidden = value => property("hidden", V.Bool(value));
let readonly = value => property("readOnly", V.Bool(value));
let required = value => property("required", V.Bool(value));
let autofocus = value => property("autoFocus", V.Bool(value));
let multiple = value => property("multiple", V.Bool(value));
