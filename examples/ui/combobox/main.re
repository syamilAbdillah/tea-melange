open Tea;
module C = Ui.Combobox;
module Modal = Ui.Dialog;
external delay: (unit => unit, int) => int = "setTimeout";
external cancel_delay: int => unit = "clearTimeout";
type widget = {
  id: string,
  title: string,
  combo: C.model,
  catalog: array(C.item),
  results: array(C.item),
  remote: bool,
  waiting: bool,
};
type model = {
  widgets: array(widget),
  modal: Modal.model,
  activity: string,
  submissions: int,
};
type msg =
  | Combo(string, C.msg)
  | Loaded(string, string)
  | Dialog(Modal.msg)
  | Show_modal
  | Submit;
let label = (items, key) =>
  Option.map(
    (item: C.item) => item.label,
    Array.find_opt((item: C.item) => item.key == key, items),
  );
let filtered = (catalog, combo) =>
  C.filter(
    ~query=C.query(combo),
    ~label=(item: C.item) => item.label,
    ~mode=C.Starts_with,
    catalog,
  );
let make =
    (
      ~id,
      ~title,
      ~mode=C.Restricted(C.Revert),
      ~multiple=false,
      ~autocomplete=C.List,
      ~dir=C.Ltr,
      ~disabled=false,
      ~readonly=false,
      ~remote=false,
      (),
    ) => {
  let catalog = Countries.items;
  let combo =
    C.init(
      ~id,
      ~mode,
      ~multiple,
      ~autocomplete,
      ~dir,
      ~disabled,
      ~readonly,
      (),
    );
  {
    id,
    title,
    combo,
    catalog,
    results: remote ? [||] : catalog,
    remote,
    waiting: false,
  };
};
let initial = () => {
  widgets: [|
    make(~id="country", ~title="Country or territory", ()),
    make(
      ~id="tags",
      ~title="Travel tags",
      ~multiple=true,
      ~mode=C.Free_form,
      (),
    ),
    make(~id="remote", ~title="Search destinations", ~remote=true, ()),
    make(~id="inline", ~title="Inline completion", ~autocomplete=C.Both, ()),
    make(~id="rtl", ~title="Destination (RTL)", ~dir=C.Rtl, ()),
    make(
      ~id="none",
      ~title="No autocomplete",
      ~autocomplete=C.None_,
      ~mode=C.Free_form,
      (),
    ),
    make(
      ~id="clear",
      ~title="Clear unmatched value",
      ~mode=C.Restricted(C.Clear),
      (),
    ),
    make(~id="readonly", ~title="Read-only country", ~readonly=true, ()),
    make(~id="disabled", ~title="Disabled country", ~disabled=true, ()),
    make(~id="invalid", ~title="Required country", ()),
    make(~id="nested", ~title="Country inside modal", ()),
  |],
  modal: Modal.init(~id="destination", ~initial_focus="nested-control", ()),
  activity: "Choose a destination or add your own travel tag.",
  submissions: 0,
};
let update = (message, model) =>
  switch (message) {
  | Show_modal =>
    let (modal, cmd) = Modal.open_(model.modal);
    (
      {
        ...model,
        modal,
      },
      Cmd.map(m => Dialog(m), cmd),
    );
  | Dialog(msg) =>
    let (modal, cmd, _) = Modal.update(msg, model.modal);
    (
      {
        ...model,
        modal,
      },
      Cmd.map(m => Dialog(m), cmd),
    );
  | Submit => (
      {
        ...model,
        submissions: model.submissions + 1,
      },
      Cmd.none,
    )
  | Loaded(id, query) => (
      {
        ...model,
        widgets:
          Array.map(
            w =>
              if (w.id != id || !w.waiting || C.query(w.combo) != query) {
                w;
              } else {
                let results = filtered(w.catalog, w.combo);
                {
                  ...w,
                  results,
                  waiting: false,
                  combo: C.refresh(~items=results, w.combo),
                };
              },
            model.widgets,
          ),
      },
      Cmd.none,
    )
  | Combo(id, msg) =>
    let commands = ref([]);
    let activity = ref(model.activity);
    let widgets =
      Array.map(
        w =>
          if (w.id != id) {
            w;
          } else {
            let (combo, cmd, outs) =
              C.update(
                ~items=w.results,
                ~label_for_key=key => label(w.catalog, key),
                msg,
                w.combo,
              );
            commands := [Cmd.map(m => Combo(id, m), cmd), ...commands^];
            let w =
              List.fold_left(
                (w, out) =>
                  switch (out) {
                  | C.Query_changed(query) =>
                    activity := "Query: " ++ query;
                    {
                      ...w,
                      waiting: w.remote,
                    };
                  | C.Selected(key) =>
                    activity :=
                      "Selected: "
                      ++ Option.value(~default=key, label(w.catalog, key));
                    w;
                  | C.Removed(key) =>
                    activity := "Removed: " ++ key;
                    w;
                  | C.Submitted(value) when id == "tags" && value != "" =>
                    let key = "custom:" ++ value;
                    let catalog =
                      Option.is_some(label(w.catalog, key))
                        ? w.catalog
                        : Array.append(
                            w.catalog,
                            [|
                              {
                                C.key,
                                label: value,
                                disabled: false,
                              },
                            |],
                          );
                    let combo =
                      C.set_text(
                        "",
                        C.set_selected(
                          C.selected(w.combo) @ [key],
                          w.combo,
                        ),
                      );
                    let (combo, cmd) = C.open_(combo);
                    commands :=
                      [Cmd.map(m => Combo(id, m), cmd), ...commands^];
                    activity := "Added tag: " ++ value;
                    {
                      ...w,
                      catalog,
                      combo,
                    };
                  | C.Submitted(value) =>
                    activity := "Submitted: " ++ value;
                    w;
                  | _ => w
                  },
                {
                  ...w,
                  combo,
                },
                outs,
              );
            let results =
              w.remote
                ? w.waiting ? [||] : w.results : filtered(w.catalog, w.combo);
            {
              ...w,
              results,
              combo: C.refresh(~items=results, ~loading=w.waiting, w.combo),
            };
          },
        model.widgets,
      );
    (
      {
        ...model,
        widgets,
        activity: activity^,
      },
      Cmd.batch(List.rev(commands^)),
    );
  };
let widget = w =>
  C.view(
    ~model=w.combo,
    ~items=w.results,
    ~to_parent=m => Combo(w.id, m),
    ~has_description=true,
    ~has_error=w.id == "invalid",
    ~invalid=w.id == "invalid",
    ~required=w.id == "invalid",
    ~to_view=
      p =>
        Html.section(
          [Html.attribute("dir", w.id == "rtl" ? "rtl" : "ltr")],
          [
            Html.label(p.label, [Html.text(w.title)]),
            Html.div(
              p.wrapper @ [Html.class_("wrapper")],
              [
                Html.input(p.control, []),
                Html.button(p.toggle, [Html.text("Show")]),
              ],
            ),
            Html.div(
              [Html.class_("chips")],
              List.map(
                key => {
                  let name =
                    Option.value(~default=key, label(w.catalog, key));
                  Html.span(
                    p.chip(key),
                    [
                      Html.text(name),
                      Html.button(
                        p.remove(~key, ~label=name),
                        [Html.text("Remove")],
                      ),
                    ],
                  );
                },
                p.selected,
              ),
            ),
            Html.p(
              p.description,
              [
                Html.text(
                  switch (w.id) {
                  | "country" => "250 countries and territories. Type a prefix, then use Down and Enter. Unmatched text reverts on blur."
                  | "tags" => "Pick several suggestions or type a new tag and press Enter. Backspace on an empty input removes the last tag."
                  | "remote" => "Simulated remote results arrive after 180 ms; stale requests are cancelled."
                  | "inline" => "A matching suffix is selected inline. Backspace removes the suffix before deleting your text."
                  | "clear" => "Unlike the first picker, unmatched text clears both text and selection on blur."
                  | "none" => "The parent still filters, but aria-autocomplete is none and text is never completed inline."
                  | _ => "DOM focus stays in the input. Home and End move the caret."
                  },
                ),
              ],
            ),
            w.id == "invalid"
              ? Html.p(
                  p.error,
                  [Html.text("Select a country before continuing.")],
                )
              : Html.text(""),
            /* Kept mounted to demonstrate native hidden and idref cleanup. */
            Html.div(
              p.popup,
              Array.to_list(
                Array.map(
                  (item: C.item) =>
                    Html.div(p.option_(item.key), [Html.text(item.label)]),
                  w.results,
                ),
              ),
            ),
            Html.div(p.loading, [Html.text("Loading destinations")]),
            Html.div(
              p.empty,
              [Html.text("No results. Try another prefix.")],
            ),
            Html.div(p.live, [Html.text(p.announcement)]),
          ],
        ),
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Find a place. Keep your focus.")]),
      Html.p(
        [],
        [
          Html.text(
            "Editable suggestions with a single keyboard destination. These examples share the same headless combobox; filtering belongs to the page.",
          ),
        ],
      ),
      Html.form(
        [Html.on_submit(Submit)],
        Array.to_list(Array.map(widget, Array.sub(model.widgets, 0, 10))),
      ),
      Html.button(
        [
          Html.id("show-modal"),
          Html.type_("button"),
          Html.on_click(Show_modal),
        ],
        [Html.text("Choose inside a modal")],
      ),
      Modal.view(
        ~model=model.modal,
        ~labels={ close: "Close settings" },
        ~to_parent=m => Dialog(m),
        ~to_view=
          p =>
            Html.node(
              "dialog",
              p.root,
              [
                Html.h2(p.title, [Html.text("Destination settings")]),
                widget(model.widgets[10]),
                Html.button(p.close_button, [Html.text("Close settings")]),
              ],
            ),
        (),
      ),
      Html.node(
        "output",
        [Html.id("activity")],
        [
          Html.text(
            model.activity
            ++ ". Form submissions: "
            ++ string_of_int(model.submissions),
          ),
        ],
      ),
    ],
  );
let subscriptions = model =>
  Sub.batch([
    Sub.map(m => Dialog(m), Modal.subscriptions(model.modal)),
    Sub.batch(
      Array.to_list(
        Array.map(
          w =>
            Sub.batch([
              Sub.map(m => Combo(w.id, m), C.subscriptions(w.combo)),
              w.waiting
                ? Sub.make(
                    ~home="Combobox.example.remote",
                    ~key=w.id ++ ":" ++ C.query(w.combo),
                    ~tagger=query => Loaded(w.id, query),
                    ~start=
                      dispatch => {
                        let timer =
                          delay(() => dispatch(C.query(w.combo)), 180);
                        () => cancel_delay(timer);
                      },
                  )
                : Sub.none,
            ]),
          model.widgets,
        ),
      ),
    ),
  ]);
let () =
  ignore(
    App.element(
      ~init=() => (initial(), Cmd.none),
      ~update,
      ~view,
      ~subscriptions,
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
