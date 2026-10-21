open Tea;
open Test_support;
module F = Ui.Select;
[@mel.new] [@mel.scope "window"]
external form_data: Dom.node => Js.t({..}) = "FormData";
[@mel.send]
external values: (Js.t({..}), string) => array(string) = "getAll";
type msg =
  | Selected(F.selection)
  | Items(array(F.item))
  | Value(F.selection)
  | Flags(bool)
  | Presence(bool, bool);
type model = {
  value: F.selection,
  items: array(F.item),
  flags: bool,
  description: bool,
  error: bool,
  count: int,
};
let initial_items = [|
  F.Option({
    key: "a",
    label: "Alpha",
    disabled: false,
  }),
  F.Option({
    key: "off",
    label: "Disabled",
    disabled: true,
  }),
  F.Group({
    key: "group/1",
    label: "Group",
    disabled: false,
    items: [|
      {
        key: "b /☕",
        label: "Beta",
        disabled: false,
      },
      {
        key: "",
        label: "Empty key",
        disabled: false,
      },
    |],
  }),
|];
let update = (msg, m) => (
  switch (msg) {
  | Selected(value) => {
      ...m,
      value: F.normalize(~items=m.items, value),
      count: m.count + 1,
    }
  | Items(items) => {
      ...m,
      items,
      value: F.normalize(~items, m.value),
    }
  | Value(value) => {
      ...m,
      value,
    }
  | Flags(flags) => {
      ...m,
      flags,
    }
  | Presence(description, error) => {
      ...m,
      description,
      error,
    }
  },
  Cmd.none,
);
let view = m =>
  Html.form(
    [Html.id("form")],
    [
      F.view(
        ~id="select",
        ~items=m.items,
        ~value=m.value,
        ~on_change=value => Selected(value),
        ~placeholder="Choose",
        ~invalid=m.flags,
        ~disabled=m.flags,
        ~required=m.flags,
        ~has_description=m.description,
        ~has_error=m.error,
        ~name="choice",
        ~to_view=
          p =>
            Html.div(
              [],
              [
                Html.label(p.label, [Html.text("Choice")]),
                Html.keyed("select", p.control, p.options),
                m.description
                  ? Html.p(p.description, [Html.text("Help")])
                  : Html.text(""),
                m.error
                  ? Html.p(p.error, [Html.text("Error")]) : Html.text(""),
              ],
            ),
        (),
      ),
    ],
  );
let set_selected = keys => {
  Array.iter(
    option =>
      put(
        option,
        "selected",
        List.exists(key => String.equal(key, get(option, "value")), keys),
      ),
    query_all(document, "#select-control option"),
  );
  ignore(emit("#select-control", "change"));
};
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              value: F.Single(None),
              items: initial_items,
              flags: false,
              description: false,
              error: false,
              count: 0,
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
  check(
    "native select",
    get(node("#select-control"), "tagName") == "SELECT",
  );
  check(
    "label links",
    attr("#select-label", "for") == Some("select-control"),
  );
  check(
    "initial placeholder selected",
    get(node("#select-control"), "value") == ""
    && get(node("#select-placeholder"), "selected") == true,
  );
  check(
    "placeholder disabled",
    get(node("#select-placeholder"), "disabled") == true,
  );
  check(
    "no phantom refs",
    attr("#select-control", "aria-describedby") == None,
  );
  check(
    "optgroup rendered",
    Array.length(query_all(document, "optgroup[label=Group]")) == 1,
  );
  set_selected(["b /☕"]);
  check("keys not indices", app.model().value == F.Single(Some("b /☕")));
  check(
    "raw key is native form value",
    values(form_data(node("#form")), "choice") == [|"b /☕"|],
  );
  app.dispatch(
    Items([|initial_items[2], initial_items[1], initial_items[0]|]),
  );
  frame();
  check(
    "reorder preserves selection by key",
    get(node("#select-control"), "value") == "b /☕"
    && app.model().value == F.Single(Some("b /☕")),
  );
  app.dispatch(Value(F.Single(Some(""))));
  frame();
  ignore(emit("#select-control", "change"));
  check(
    "empty key distinguished from placeholder",
    app.model().value == F.Single(Some("")),
  );
  app.dispatch(Value(F.Single(None)));
  frame();
  ignore(emit("#select-control", "change"));
  check("placeholder decodes None", app.model().value == F.Single(None));
  app.dispatch(Value(F.Multiple([|"a", "b /☕", "a", "absent"|])));
  frame();
  check(
    "multiple native mode",
    get(node("#select-control"), "multiple") == true,
  );
  check(
    "no placeholder in multiple",
    Array.length(query_all(document, "#select-placeholder")) == 0,
  );
  check(
    "normalize removes duplicates/missing",
    F.normalize(
      ~items=initial_items,
      F.Multiple([|"a", "b /☕", "a", "absent"|]),
    )
    == F.Multiple([|"a", "b /☕"|]),
  );
  set_selected(["a", "b /☕"]);
  check(
    "multiple actual key selection DOM order",
    app.model().value == F.Multiple([|"b /☕", "a"|]),
  );
  check(
    "multiple form values",
    values(form_data(node("#form")), "choice") == [|"b /☕", "a"|],
  );
  set_selected([]);
  check("multiple empty selection", app.model().value == F.Multiple([||]));
  set_selected(["a"]);
  check("single multiple item", app.model().value == F.Multiple([|"a"|]));
  app.dispatch(Items([||]));
  frame();
  check(
    "empty collection repairs",
    app.model().value == F.Multiple([||])
    && Array.length(query_all(document, "option")) == 0,
  );
  app.dispatch(
    Items([|
      F.Group({
        key: "off",
        label: "Off",
        disabled: true,
        items: [|
          {
            key: "a",
            label: "A",
            disabled: false,
          },
        |],
      }),
    |]),
  );
  frame();
  check(
    "disabled group and options",
    get(node("optgroup"), "disabled") == true
    && get(node("option"), "disabled") == true,
  );
  app.dispatch(Value(F.Single(Some("gone"))));
  frame();
  check(
    "missing single renders placeholder",
    get(node("#select-placeholder"), "selected") == true,
  );
  app.dispatch(
    Items([|
      F.Option({
        key: "only",
        label: "Only",
        disabled: false,
      }),
    |]),
  );
  app.dispatch(Value(F.Single(Some("only"))));
  frame();
  check(
    "single item selection",
    get(node("#select-control"), "value") == "only",
  );
  focus(node("#select-control"));
  check("native focus", has_focus("#select-control"));
  List.iter(
    key_ =>
      check(
        "native keys untouched",
        key("#select-control", "keydown", key_),
      ),
    [
      "Tab",
      "ArrowDown",
      "ArrowUp",
      "Home",
      "End",
      " ",
      "Enter",
      "Escape",
      "o",
    ],
  );
  check(
    "modifier untouched",
    key(~alt=true, "#select-control", "keydown", "ArrowDown"),
  );
  app.dispatch(Flags(true));
  app.dispatch(Presence(true, true));
  frame();
  check(
    "disabled/required native",
    get(node("#select-control"), "disabled") == true
    && get(node("#select-control"), "required") == true,
  );
  check(
    "validation",
    attr("#select-control", "aria-invalid") == Some("true")
    && attr("#select-control", "aria-errormessage") == Some("select-error")
    && attr("#select-control", "aria-describedby")
    == Some("select-description select-error"),
  );
  check(
    "data hooks",
    attr("#select-control", "data-disabled") == Some("")
    && attr("#select-control", "data-invalid") == Some(""),
  );
  let before = app.model().count;
  ignore(emit("#select-control", "change"));
  check("disabled no event", app.model().count == before);
  check(
    "disabled omits form data",
    Array.length(values(form_data(node("#form")), "choice")) == 0,
  );
  app.dispatch(Presence(false, false));
  frame();
  check(
    "error absent cannot dangle",
    attr("#select-control", "aria-errormessage") == None
    && attr("#select-control", "aria-describedby") == None,
  );
  app.dispatch(Flags(false));
  frame();
  check(
    "states cleared",
    attr("#select-control", "aria-invalid") == None
    && attr("#select-control", "data-invalid") == None
    && get(node("#select-control"), "required") == false,
  );
  ignore(
    dispatch(
      node("#select-control"),
      event("change", { "bubbles": true }),
    ),
  );
  app.dispatch(Items([||]));
  ignore(
    dispatch(
      node("#select-control"),
      event("change", { "bubbles": true }),
    ),
  );
  frame();
  check(
    "items changed between messages normalize current collection",
    app.model().value == F.Single(None) && app.model().count == before + 2,
  );
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("select");
  Js.Promise.resolve();
};
