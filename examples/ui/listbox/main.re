open Tea;
module L = Ui.Listbox;
let fruit: array(L.item(string)) = [|
  {
    key: "apple",
    label: "Apple",
    disabled: false,
    value: "apple",
  },
  {
    key: "apricot",
    label: "Apricot",
    disabled: true,
    value: "apricot",
  },
  {
    key: "banana",
    label: "Banana",
    disabled: false,
    value: "banana",
  },
  {
    key: "black",
    label: "Black currant",
    disabled: false,
    value: "black",
  },
  {
    key: "blue",
    label: "Blueberry",
    disabled: false,
    value: "blue",
  },
  {
    key: "cherry",
    label: "Cherry",
    disabled: false,
    value: "cherry",
  },
  {
    key: "date",
    label: "Date",
    disabled: false,
    value: "date",
  },
  {
    key: "elder",
    label: "Elderberry",
    disabled: false,
    value: "elder",
  },
  {
    key: "fig",
    label: "Fig",
    disabled: false,
    value: "fig",
  },
  {
    key: "grape",
    label: "Grape",
    disabled: false,
    value: "grape",
  },
  {
    key: "kiwi",
    label: "Kiwi",
    disabled: false,
    value: "kiwi",
  },
  {
    key: "lemon",
    label: "Lemon",
    disabled: false,
    value: "lemon",
  },
  {
    key: "mango",
    label: "Mango",
    disabled: false,
    value: "mango",
  },
  {
    key: "orange",
    label: "Orange",
    disabled: false,
    value: "orange",
  },
  {
    key: "pear",
    label: "Pear",
    disabled: false,
    value: "pear",
  },
|];
let long_items =
  Array.init(
    200,
    i => {
      let n = string_of_int(i);
      let n = String.make(3 - String.length(n), '0') ++ n;
      {
        L.key: "i" ++ n,
        label: "Item " ++ n,
        disabled: i == 4,
        value: n,
      };
    },
  );
let ids = [|
  "single",
  "multi",
  "readonly",
  "form",
  "long",
  "rtl",
  "empty",
  "disabled",
  "invalid",
  "wrap",
  "dynamic",
|];
let labels = [|
  "Fruit",
  "Tags",
  "Read only",
  "Form fruit",
  "Archive",
  "Fruit in RTL",
  "Empty",
  "Disabled",
  "Invalid",
  "Wrapping",
  "Changing items",
|];
type model = {
  boxes: array(L.model),
  dynamic: array(L.item(string)),
  changes: int,
  submitted: string,
};
type msg =
  | Box(int, L.msg)
  | Submitted
  | Remove_active
  | Empty_dynamic
  | Restore_dynamic
  | Remove_tag(string);
let items = (model, i) =>
  i == 4 ? long_items : i == 6 ? [||] : i == 10 ? model.dynamic : fruit;
let update = (message, model) =>
  switch (message) {
  | Box(i, message) =>
    let (box, command, outs) =
      L.update(~items=items(model, i), message, model.boxes[i]);
    let boxes = Array.copy(model.boxes);
    boxes[i] = box;
    let changes =
      List.fold_left(
        (n, out) =>
          switch (out) {
          | L.Changed(_) => n + 1
          | _ => n
          },
        model.changes,
        outs,
      );
    (
      {
        ...model,
        boxes,
        changes,
      },
      Cmd.map(message => Box(i, message), command),
    );
  | Submitted => (
      {
        ...model,
        submitted: String.concat(", ", L.selected_keys(model.boxes[3])),
      },
      Cmd.none,
    )
  | Remove_tag(key) =>
    let boxes = Array.copy(model.boxes);
    boxes[1] =
      L.set_selected(
        List.filter(k => !String.equal(k, key), L.selected_keys(boxes[1])),
        boxes[1],
      );
    (
      {
        ...model,
        boxes,
      },
      Cmd.none,
    );
  | Remove_active
  | Empty_dynamic
  | Restore_dynamic =>
    let dynamic =
      switch (message) {
      | Empty_dynamic => [||]
      | Restore_dynamic => fruit
      | _ =>
        Array.of_list(
          List.filter(
            item => Some(item.L.key) != L.active_key(model.boxes[10]),
            Array.to_list(model.dynamic),
          ),
        )
      };
    let (box, command) = L.reconcile(~items=dynamic, model.boxes[10]);
    let boxes = Array.copy(model.boxes);
    boxes[10] = box;
    (
      {
        ...model,
        boxes,
        dynamic,
      },
      Cmd.map(message => Box(10, message), command),
    );
  };
let render = (model, i) =>
  L.view(
    ~model=model.boxes[i],
    ~items=items(model, i),
    ~to_parent=message => Box(i, message),
    ~placeholder="Choose fruit",
    ~name=?i == 3 ? Some("fruits") : None,
    ~to_view=
      p => {
        let options = collection =>
          List.map(
            (item: L.item(string)) =>
              Html.div(p.option_(item.key), [Html.text(item.label)]),
            collection,
          );
        let children =
          i == 0
            ? [
              Html.div(
                p.group("fresh"),
                [Html.div(p.group_label("fresh"), [Html.text("Orchard")])]
                @ options(Array.to_list(Array.sub(fruit, 0, 3))),
              ),
              Html.div(
                p.group("other"),
                [
                  Html.div(
                    p.group_label("other"),
                    [Html.text("More fruit")],
                  ),
                ]
                @ options(
                    Array.to_list(
                      Array.sub(fruit, 3, Array.length(fruit) - 3),
                    ),
                  ),
              ),
            ]
            : options(Array.to_list(items(model, i)));
        Html.div(
          p.root
          @ [
            Html.class_("field"),
            Html.attribute("dir", i == 5 ? "rtl" : "ltr"),
          ],
          [
            Html.span(p.label, [Html.text(labels[i])]),
            Html.button(p.button, [Html.text(p.display_value)]),
            Html.div(
              p.listbox @ [Html.class_("popup")],
              children
              @ [Html.div(p.empty, [Html.text("No options available")])],
            ),
            i == 1
              ? Html.div(
                  [Html.class_("tags")],
                  Array.to_list(
                    Array.map(
                      (item: L.item(string)) =>
                        Ui.Button.button(
                          ~id="tag-" ++ item.key,
                          ~on_press=Remove_tag(item.key),
                          [Html.text("Remove " ++ item.label)],
                        ),
                      p.selected,
                    ),
                  ),
                )
              : Html.text(""),
          ]
          @ p.form_inputs,
        );
      },
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Pick your fruit.")]),
      Html.p(
        [],
        [
          Html.text(
            "A button, a list, and a clear choice. Browse with arrows, type to find, then Enter to select. Escape or Tab leaves the value unchanged.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("fields")],
        List.init(3, i => render(model, i)),
      ),
      Html.form(
        [Html.id("fruit-form"), Html.on_submit(Submitted)],
        [
          render(model, 3),
          Ui.Button.button(
            ~id="submit",
            ~button_type=Ui.Button.Submit,
            ~on_press=Submitted,
            [Html.text("Submit selection")],
          ),
          Html.node(
            "output",
            [Html.id("submission")],
            [Html.text("Submitted: " ++ model.submitted)],
          ),
        ],
      ),
      Html.div(
        [Html.class_("fields")],
        List.init(7, i => render(model, i + 4)),
      ),
      Html.div(
        [Html.class_("actions")],
        [
          Ui.Button.button(
            ~id="remove-active",
            ~on_press=Remove_active,
            [Html.text("Remove active option")],
          ),
          Ui.Button.button(
            ~id="empty-dynamic",
            ~on_press=Empty_dynamic,
            [Html.text("Clear changing items")],
          ),
          Ui.Button.button(
            ~id="restore-dynamic",
            ~on_press=Restore_dynamic,
            [Html.text("Restore changing items")],
          ),
          Html.button(
            [Html.id("outside"), Html.type_("button")],
            [Html.text("Outside target")],
          ),
        ],
      ),
      Html.p(
        [Html.id("plain-outside")],
        [Html.text("This nonfocusable text is also outside every listbox.")],
      ),
      Html.node(
        "output",
        [Html.id("activity"), Html.aria("live", "polite")],
        [Html.text("Changes: " ++ string_of_int(model.changes))],
      ),
    ],
  );
let app =
  App.element(
    ~init=
      () =>
        (
          {
            boxes:
              Array.mapi(
                (i, id) =>
                  L.init(
                    ~id,
                    ~multiple=i == 1 || i == 3,
                    ~selected=
                      i == 2
                        ? ["banana"] : i == 3 ? ["banana", "cherry"] : [],
                    ~readonly=i == 2,
                    ~disabled=i == 7,
                    ~invalid=i == 8,
                    ~wrap=i == 9,
                    ~dir=i == 5 ? Ui.Anchor.Rtl : Ui.Anchor.Ltr,
                    (),
                  ),
                ids,
              ),
            dynamic: fruit,
            changes: 0,
            submitted: "",
          },
          Cmd.none,
        ),
    ~update,
    ~view,
    ~subscriptions=
      model =>
        Sub.batch(
          Array.to_list(
            Array.mapi(
              (i, box) =>
                Sub.map(message => Box(i, message), L.subscriptions(box)),
              model.boxes,
            ),
          ),
        ),
    ~node="#app",
    ~flags=(),
    (),
  );
