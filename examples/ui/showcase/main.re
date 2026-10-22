open Tea;
module L = Ui.Listbox;
module C = Ui.Combobox;
module P = Ui.Date_picker;
module D = Ui.Dialog;
module M = Ui.Menu;
module T = Ui.Tooltip;
module H = Ui.Hover_intent;
module R = Ui.Radio_group;
module S = Ui.Slider;
module V = Ui.Virtual_list;
module G = Ui.Drag_and_drop;
module F = Ui.File_drop;
module N = Ui.Toast;

/* Collections belong to the application; keys survive filtering and reordering. */
let teams: array(L.item(string)) = [|
  {
    key: "research",
    label: "Research",
    value: "research",
    disabled: false,
  },
  {
    key: "operations",
    label: "Operations",
    value: "operations",
    disabled: false,
  },
  {
    key: "guests",
    label: "Guests (unavailable)",
    value: "guests",
    disabled: true,
  },
|];
let people: array(C.item) = [|
  {
    key: "ada",
    label: "Ada Chen",
    disabled: false,
  },
  {
    key: "jo",
    label: "Jo Rivera",
    disabled: false,
  },
  {
    key: "sam",
    label: "Sam Okafor",
    disabled: false,
  },
|];
let radio_items: array(R.item) = [|
  {
    key: "weekly",
    label: "Weekly digest",
    disabled: false,
  },
  {
    key: "daily",
    label: "Daily digest",
    disabled: false,
  },
|];
let tab_items: array(Ui.Tabs.item) = [|
  {
    key: "settings",
    label: "Settings",
    disabled: false,
  },
  {
    key: "activity",
    label: "Activity",
    disabled: false,
  },
|];
let thumbs: array(S.thumb) = [|
  {
    key: "limit",
    label: S.Label("Storage reminder threshold"),
    initial: 75.,
    disabled: false,
  },
|];
let regions = [|
  Ui.Select.Option({
    key: "eu",
    label: "Europe",
    disabled: false,
  }),
  Ui.Select.Option({
    key: "us",
    label: "North America",
    disabled: false,
  }),
|];
let actions: array(M.item) = [|
  {
    key: "export",
    label: "Export settings",
    disabled: false,
    kind:
      M.Submenu([|
        {
          key: "json",
          label: "JSON file",
          disabled: false,
          kind: M.Action,
        },
        {
          key: "csv",
          label: "CSV file",
          disabled: false,
          kind: M.Action,
        },
      |]),
  },
  {
    key: "copy",
    label: "Copy summary",
    disabled: false,
    kind: M.Action,
  },
|];
let audit_items =
  V.items(Array.init(10000, i => "event-" ++ string_of_int(i)));
let today =
  Option.value(
    ~default=Ui.Calendar.Date.first,
    Ui.Calendar.Date.of_iso("2026-03-05"),
  );
let initial_order: array(G.item) = [|
  {
    key: "survey",
    label: "Review field survey",
    disabled: false,
  },
  {
    key: "brief",
    label: "Publish team brief",
    disabled: false,
  },
  {
    key: "backup",
    label: "Verify backup",
    disabled: false,
  },
|];
let containers = (order): array(G.container) => [|
  {
    key: "tasks",
    label: "Launch checklist",
    disabled: false,
    items: order,
  },
|];
type msg =
  | Name(string)
  | Notes(string)
  | Check
  | Switch
  | Region(Ui.Select.selection)
  | Radio(R.msg)
  | Slider(S.msg)
  | Tabs(Ui.Tabs.msg)
  | Listbox(string, L.msg)
  | Combo(string, C.msg)
  | Picker(string, P.msg)
  | Menu(string, M.msg)
  | Tip(string, T.msg)
  | Filters(Ui.Popover.msg)
  | Dialog(D.msg)
  | Open_delete
  | Cancel_delete
  | Delete
  | Save
  | Toast(N.msg)
  | Drag(G.msg)
  | Files(F.msg)
  | Virtual(V.msg)
  | Jump
  | Faq
  | Preview
  | Noop;
type model = {
  name: string,
  notes: string,
  checked: bool,
  updates: bool,
  region: Ui.Select.selection,
  radio: R.model,
  slider: S.model,
  tabs: Ui.Tabs.model,
  lists: list((string, L.model)),
  combos: list((string, C.model)),
  pickers: list((string, P.model)),
  menus: list((string, M.model)),
  tips: list((string, T.model)),
  group: H.Group.t,
  filters: Ui.Popover.model,
  dialog: D.model,
  toast: N.model,
  contents: list((string, string)),
  serial: int,
  saved: string,
  deleted: bool,
  drag: G.model,
  order: array(G.item),
  files: F.model,
  filenames: list(string),
  virtual_: V.model,
  faq: bool,
  preview: bool,
};
let replace = (key, value, pairs) =>
  List.map(((k, v)) => (k, String.equal(k, key) ? value : v), pairs);
let suggestions = combo =>
  C.filter(~query=C.query(combo), ~label=(i: C.item) => i.label, people);
let label_for_key = key =>
  Option.map(
    (p: C.item) => p.label,
    Array.to_list(people)
    |> List.find_opt((p: C.item) => String.equal(key, p.key)),
  );
let notify = (text, model) => {
  let serial = model.serial + 1;
  let key = "notice-" ++ string_of_int(serial);
  /* Persistent success notices have a dismiss button; never interrupt speech. */
  let (toast, cmd) =
    N.push(model.toast, ~key, ~duration=None, ~urgency=N.Normal, key);
  (
    {
      ...model,
      toast,
      serial,
      contents: [(key, text), ...model.contents],
    },
    Cmd.map(m => Toast(m), cmd),
  );
};
/* Native modal ancestors close their child overlays too. Keep every model and
   subscription alive through Leaving; never override the library's hidden fact. */
let close_children = model => {
  let commands = ref([]);
  let close = (pairs, f, tag) =>
    List.map(
      ((key, value)) =>
        if (String.equal(key, "review")) {
          let (value, cmd) = f(value);
          commands := [Cmd.map(m => tag(key, m), cmd), ...commands^];
          (key, value);
        } else {
          (key, value);
        },
      pairs,
    );
  let lists = close(model.lists, m => L.close(m), (k, m) => Listbox(k, m));
  let combos = close(model.combos, C.close, (k, m) => Combo(k, m));
  let pickers = close(model.pickers, P.close, (k, m) => Picker(k, m));
  let menus = close(model.menus, M.close, (k, m) => Menu(k, m));
  let tips = close(model.tips, T.close_immediately, (k, m) => Tip(k, m));
  (
    {
      ...model,
      lists,
      combos,
      pickers,
      menus,
      tips,
      group: H.Group.forget(~key="review", model.group),
    },
    Cmd.batch(commands^),
  );
};
let close_dialog = model => {
  let (model, child_cmd) = close_children(model);
  let (dialog, cmd) = D.close(model.dialog);
  (
    {
      ...model,
      dialog,
    },
    Cmd.batch([child_cmd, Cmd.map(m => Dialog(m), cmd)]),
  );
};
let update = (message, model) =>
  switch (message) {
  | Name(name) => (
      {
        ...model,
        name,
      },
      Cmd.none,
    )
  | Notes(notes) => (
      {
        ...model,
        notes,
      },
      Cmd.none,
    )
  | Check => (
      {
        ...model,
        checked: Ui.Checkbox.next_checked(~checked=model.checked, ()),
      },
      Cmd.none,
    )
  | Switch => (
      {
        ...model,
        updates: !model.updates,
      },
      Cmd.none,
    )
  | Region(region) => (
      {
        ...model,
        region,
      },
      Cmd.none,
    )
  | Faq => (
      {
        ...model,
        faq: !model.faq,
      },
      Cmd.none,
    )
  | Preview => (
      {
        ...model,
        preview: !model.preview,
      },
      Cmd.none,
    )
  | Noop => (model, Cmd.none)
  | Radio(m) =>
    let (radio, cmd, _) = R.update(~items=radio_items, m, model.radio);
    (
      {
        ...model,
        radio,
      },
      Cmd.map(m => Radio(m), cmd),
    );
  | Slider(m) =>
    let (slider, cmd, _) = S.update(~items=thumbs, m, model.slider);
    (
      {
        ...model,
        slider,
      },
      Cmd.map(m => Slider(m), cmd),
    );
  | Tabs(m) =>
    let (tabs, cmd, _) = Ui.Tabs.update(~items=tab_items, m, model.tabs);
    (
      {
        ...model,
        tabs,
      },
      Cmd.map(m => Tabs(m), cmd),
    );
  | Listbox(key, m) =>
    let (value, cmd, _) =
      L.update(~items=teams, m, List.assoc(key, model.lists));
    (
      {
        ...model,
        lists: replace(key, value, model.lists),
      },
      Cmd.map(m => Listbox(key, m), cmd),
    );
  | Combo(key, m) =>
    let current = List.assoc(key, model.combos);
    let (value, cmd, _) =
      C.update(~items=suggestions(current), ~label_for_key, m, current);
    let value = C.refresh(~items=suggestions(value), value);
    (
      {
        ...model,
        combos: replace(key, value, model.combos),
      },
      Cmd.map(m => Combo(key, m), cmd),
    );
  | Picker(key, m) =>
    let (value, cmd, _) = P.update(m, List.assoc(key, model.pickers));
    (
      {
        ...model,
        pickers: replace(key, value, model.pickers),
      },
      Cmd.map(m => Picker(key, m), cmd),
    );
  | Menu(key, m) =>
    let (value, cmd, out) =
      M.update(~items=actions, m, List.assoc(key, model.menus));
    let model = {
      ...model,
      menus: replace(key, value, model.menus),
    };
    let selected =
      List.find_opt(
        o =>
          switch (o) {
          | M.Selected(_) => true
          | _ => false
          },
        out,
      );
    let (model, notice) =
      switch (selected) {
      | Some(M.Selected(key)) =>
        notify("Prepared " ++ key ++ " summary (local preview).", model)
      | _ => (model, Cmd.none)
      };
    (model, Cmd.batch([Cmd.map(m => Menu(key, m), cmd), notice]));
  | Tip(key, m) =>
    let (value, cmd, out) =
      T.update(~group=model.group, m, List.assoc(key, model.tips));
    let group =
      List.fold_left((g, o) => T.track(~key, o, g), model.group, out);
    let commands = ref([Cmd.map(m => Tip(key, m), cmd)]);
    let group = ref(group);
    let opened =
      List.exists(
        o =>
          switch (o) {
          | T.Opened => true
          | _ => false
          },
        out,
      );
    let tips =
      List.map(
        ((other, tip)) =>
          if (String.equal(key, other)) {
            (other, value);
          } else if (opened && T.is_visible(tip)) {
            let (tip, cmd) = T.close_immediately(tip);
            commands := [Cmd.map(m => Tip(other, m), cmd), ...commands^];
            group := H.Group.forget(~key=other, group^);
            (other, tip);
          } else {
            (other, tip);
          },
        model.tips,
      );
    (
      {
        ...model,
        tips,
        group: group^,
      },
      Cmd.batch(commands^),
    );
  | Filters(m) =>
    let (filters, cmd, _) = Ui.Popover.update(m, model.filters);
    (
      {
        ...model,
        filters,
      },
      Cmd.map(m => Filters(m), cmd),
    );
  | Open_delete =>
    let (dialog, cmd) = D.open_(model.dialog);
    (
      {
        ...model,
        dialog,
      },
      Cmd.map(m => Dialog(m), cmd),
    );
  | Cancel_delete => close_dialog(model)
  | Dialog(m) =>
    let (dialog, cmd, _) = D.update(m, model.dialog);
    let (model, children) =
      D.is_open(dialog)
        ? (
          {
            ...model,
            dialog,
          },
          Cmd.none,
        )
        : close_children({
            ...model,
            dialog,
          });
    (model, Cmd.batch([Cmd.map(m => Dialog(m), cmd), children]));
  | Delete =>
    let (model, cmd) = close_dialog(model);
    let (model, notice) =
      notify(
        "Workspace archived. Your files remain available in the archive.",
        {
          ...model,
          deleted: true,
        },
      );
    (model, Cmd.batch([cmd, notice]));
  | Save =>
    let saved =
      model.name
      ++ " / "
      ++ C.text(List.assoc("workspace", model.combos))
      ++ " / "
      ++ String.concat(
           ", ",
           L.selected_keys(List.assoc("workspace", model.lists)),
         )
      ++ " / "
      ++ Option.fold(
           ~none="No review date",
           ~some=Ui.Calendar.Date.to_iso,
           P.value(List.assoc("workspace", model.pickers)),
         );
    notify(
      "Changes saved for " ++ model.name ++ ".",
      {
        ...model,
        saved,
      },
    );
  | Toast(m) =>
    let (toast, cmd, out) = N.update(m, model.toast);
    let contents =
      List.fold_left(
        (contents, o) =>
          switch (o) {
          | N.Dismissed(key, _) =>
            List.filter(((k, _)) => !String.equal(k, key), contents)
          | N.Action(_) => contents
          },
        model.contents,
        out,
      );
    (
      {
        ...model,
        toast,
        contents,
      },
      Cmd.map(m => Toast(m), cmd),
    );
  | Drag(m) =>
    let (drag, cmd, out) =
      G.update(~containers=containers(model.order), m, model.drag);
    let order =
      List.fold_left(
        (order, o) =>
          switch (o) {
          | G.Moved(move) =>
            let item =
              Array.to_list(order)
              |> List.find_opt((i: G.item) => String.equal(i.key, move.key));
            switch (item) {
            | None => order
            | Some(item) =>
              let remaining =
                Array.to_list(order)
                |> List.filter((i: G.item) => !String.equal(i.key, move.key));
              let rec insert = (index, xs) =>
                switch (xs) {
                | [] => [item]
                | [head, ...tail] =>
                  index <= 0
                    ? [item, ...xs] : [head, ...insert(index - 1, tail)]
                };
              Array.of_list(insert(move.to_index, remaining));
            };
          | _ => order
          },
        model.order,
        out,
      );
    (
      {
        ...model,
        drag,
        order,
      },
      Cmd.map(m => Drag(m), cmd),
    );
  | Files(m) =>
    let (files, cmd, out) = F.update(m, model.files);
    let filenames =
      List.fold_left(
        (names, o) =>
          switch (o) {
          | F.Accepted(files) =>
            names @ List.map((f: F.file) => f.name, files)
          | F.Rejected(_) =>
            names @ ["File rejected: choose a text file up to 1 MB."]
          },
        model.filenames,
        out,
      );
    (
      {
        ...model,
        files,
        filenames,
      },
      Cmd.map(m => Files(m), cmd),
    );
  | Virtual(m) =>
    let (virtual_, cmd, _) = V.update(~items=audit_items, m, model.virtual_);
    (
      {
        ...model,
        virtual_,
      },
      Cmd.map(m => Virtual(m), cmd),
    );
  | Jump =>
    let (virtual_, cmd) =
      V.scroll_to_index(
        ~items=audit_items,
        ~index=9999,
        ~align=V.End,
        model.virtual_,
      );
    (
      {
        ...model,
        virtual_,
      },
      Cmd.map(m => Virtual(m), cmd),
    );
  };

let text = Html.text;
let button = (id, message, label) =>
  Ui.Button.button(~id, ~on_press=message, [text(label)]);
let box = children => Html.div([Html.class_("field")], children);
let section = (id, title, children) =>
  Html.section(
    [Html.id(id), Html.aria("labelledby", id ++ "-title")],
    [Html.h2([Html.id(id ++ "-title")], [text(title)]), ...children],
  );
let listbox = (model, key) =>
  L.view(
    ~model=List.assoc(key, model.lists),
    ~items=teams,
    ~name=key ++ "-team",
    ~to_parent=m => Listbox(key, m),
    ~to_view=
      p =>
        box([
          Html.span(
            p.label,
            [
              text(
                String.equal(key, "review") ? "Archive team" : "Default team",
              ),
            ],
          ),
          Html.button(p.button, [text(p.display_value)]),
          Html.div(
            p.listbox @ [Html.class_("floating options")],
            Array.to_list(
              Array.map(
                (item: L.item(string)) =>
                  Html.div(p.option_(item.key), [text(item.label)]),
                teams,
              ),
            ),
          ),
          ...p.form_inputs,
        ]),
    (),
  );
let combo = (model, key) => {
  let current = List.assoc(key, model.combos);
  let items = suggestions(current);
  C.view(
    ~model=current,
    ~items,
    ~name=key ++ "-owner",
    ~to_parent=m => Combo(key, m),
    ~to_view=
      p =>
        box([
          Html.label(
            p.label,
            [
              text(
                String.equal(key, "review")
                  ? "Archive owner" : "Workspace owner",
              ),
            ],
          ),
          Html.div(
            p.wrapper,
            [
              Html.input(p.control, []),
              Html.button(p.toggle, [text("Show suggestions")]),
              Html.div(
                p.popup @ [Html.class_("floating options")],
                Array.to_list(
                  Array.map(
                    (item: C.item) =>
                      Html.div(p.option_(item.key), [text(item.label)]),
                    items,
                  ),
                ),
              ),
            ],
          ),
          Html.p(p.empty, [text("No matching people")]),
          Html.p(
            p.live @ [Html.class_("sr-only")],
            [text(p.announcement)],
          ),
        ]),
    (),
  );
};
let picker = (model, key) =>
  P.view(
    ~model=List.assoc(key, model.pickers),
    ~name=key ++ "-date",
    ~today,
    ~to_parent=m => Picker(key, m),
    ~to_view=
      p =>
        box([
          Html.label(
            p.label,
            [
              text(
                String.equal(key, "review")
                  ? "Archive date" : "Next review date",
              ),
            ],
          ),
          Html.div(
            p.group,
            [
              Html.input(p.input, []),
              Html.button(p.trigger, [text(P.english.choose)]),
              Html.div(
                p.dialog @ [Html.class_("floating calendar")],
                [
                  Html.span(p.start_boundary, []),
                  Html.div(
                    p.calendar.root,
                    [
                      Html.h3(
                        p.calendar.heading,
                        [text(p.calendar.heading_text)],
                      ),
                      Html.div(
                        [Html.class_("row")],
                        [
                          Html.button(
                            p.calendar.previous_month,
                            [text("Previous month")],
                          ),
                          Html.button(
                            p.calendar.next_month,
                            [text("Next month")],
                          ),
                        ],
                      ),
                      Html.table(
                        p.calendar.grid,
                        [
                          Html.thead(
                            [],
                            [
                              Html.tr(
                                [],
                                Array.to_list(
                                  Array.map(
                                    (day: Ui.Calendar.weekday(msg)) =>
                                      Html.th(day.attrs, [text(day.text)]),
                                    p.calendar.weekdays,
                                  ),
                                ),
                              ),
                            ],
                          ),
                          Html.tbody(
                            [],
                            Array.to_list(
                              Array.map(
                                week =>
                                  Html.tr(
                                    [],
                                    Array.to_list(
                                      Array.map(
                                        (cell: Ui.Calendar.cell(msg)) =>
                                          Html.td(
                                            cell.attrs,
                                            [text(cell.text)],
                                          ),
                                        week,
                                      ),
                                    ),
                                  ),
                                p.calendar.weeks,
                              ),
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                  Html.button(p.cancel_button, [text(P.english.cancel)]),
                  Html.span(p.end_boundary, []),
                ],
              ),
            ],
          ),
          Html.input(p.hidden_input, []),
          Html.p(p.error, [text(p.error_text)]),
          Html.p(
            p.instructions @ [Html.class_("sr-only")],
            [text(p.instructions_text)],
          ),
        ]),
    (),
  );
let menu = (model, key) =>
  M.view(
    ~model=List.assoc(key, model.menus),
    ~items=actions,
    ~to_parent=m => Menu(key, m),
    ~to_view=
      p => {
        let rec entry = (i: M.item_parts(msg)) =>
          Html.div(
            i.attrs,
            switch (i.item.kind) {
            | M.Group(_) => [
                Html.span(i.label, [text(i.item.label)]),
                ...Array.to_list(Array.map(entry, i.children)),
              ]
            | _ => [
                text(
                  i.item.label
                  ++ (
                    switch (i.item.kind) {
                    | M.Submenu(_) => " >"
                    | _ => ""
                    }
                  ),
                ),
              ]
            },
          );
        Html.div(
          [],
          [
            Html.button(
              p.trigger,
              [
                text(
                  String.equal(key, "review") ? "Review actions" : "Actions",
                ),
              ],
            ),
            ...Array.to_list(
                 Array.map(
                   (panel: M.panel_parts(msg)) =>
                     Html.div(
                       panel.attrs @ [Html.class_("floating options")],
                       Array.to_list(Array.map(entry, panel.items)),
                     ),
                   p.panels,
                 ),
               ),
          ],
        );
      },
    (),
  );
let tip = (model, key, label, help, message) =>
  T.view(
    ~model=List.assoc(key, model.tips),
    ~to_parent=m => Tip(key, m),
    ~to_view=
      p =>
        Html.div(
          [],
          [
            /* Tooltip owns keydown; native button supplies activation, not Button.parts. */
            Html.button(
              p.trigger @ [Html.type_("button"), Html.on_click(message)],
              [text(label)],
            ),
            Html.div(
              p.tooltip @ [Html.class_("floating tooltip")],
              [text(help)],
            ),
          ],
        ),
    (),
  );
let notifications = model =>
  N.view(
    ~model=model.toast,
    ~to_parent=m => Toast(m),
    ~to_view=
      p => {
        let lane = urgency =>
          List.filter_map(
            (t: N.toast_parts(msg)) =>
              t.urgency != urgency
                ? None
                : Some((
                    t.key,
                    Html.div(
                      t.root @ [Html.class_("toast")],
                      [
                        Html.h3(t.title, [text("Fieldwork")]),
                        Html.p(
                          t.description,
                          [
                            text(
                              Option.value(
                                ~default="",
                                List.assoc_opt(t.content_key, model.contents),
                              ),
                            ),
                          ],
                        ),
                        Html.button(t.close_button, [text("Dismiss")]),
                      ],
                    ),
                  )),
            p.toasts,
          );
        Html.div(
          p.region @ [Html.class_("notifications")],
          [
            Html.keyed("div", p.polite, lane(N.Normal)),
            Html.keyed("div", p.assertive, lane(N.Urgent)),
          ],
        );
      },
    (),
  );
let form = model =>
  Html.form(
    [Html.on_submit(Save), Html.aria("label", "Workspace settings")],
    [
      section(
        "identity",
        "Workspace details",
        [
          Html.p(
            [],
            [
              text(
                "Set the defaults your field team starts with. Changes stay in this local demo.",
              ),
            ],
          ),
          Ui.Fieldset.view(
            ~id="details",
            ~to_view=
              p =>
                Html.fieldset(
                  p.root,
                  [
                    Html.legend(p.legend, [text("Identity and ownership")]),
                    Ui.Input.view(
                      ~id="workspace-name",
                      ~name="workspace",
                      ~value=model.name,
                      ~required=true,
                      ~on_input=s => Name(s),
                      ~to_view=
                        p =>
                          box([
                            Html.label(p.label, [text("Workspace name")]),
                            Html.input(p.control, []),
                          ]),
                      (),
                    ),
                    Ui.Textarea.view(
                      ~id="workspace-notes",
                      ~name="notes",
                      ~value=model.notes,
                      ~on_input=s => Notes(s),
                      ~to_view=
                        p =>
                          box([
                            Html.label(p.label, [text("Team brief")]),
                            Html.textarea(p.control, []),
                          ]),
                      (),
                    ),
                    Html.div(
                      [Html.class_("columns")],
                      [
                        Ui.Select.view(
                          ~id="region",
                          ~name="region",
                          ~items=regions,
                          ~value=model.region,
                          ~on_change=s => Region(s),
                          ~to_view=
                            p =>
                              box([
                                Html.label(p.label, [text("Data region")]),
                                Html.keyed("select", p.control, p.options),
                              ]),
                          (),
                        ),
                        combo(model, "workspace"),
                        listbox(model, "workspace"),
                        picker(model, "workspace"),
                      ],
                    ),
                  ],
                ),
            (),
          ),
        ],
      ),
      section(
        "preferences",
        "Notifications and storage",
        [
          Ui.Checkbox.view(
            ~id="digest",
            ~name="digest",
            ~checked=model.checked,
            ~on_change=Check,
            ~to_view=
              p =>
                Html.div(
                  [Html.class_("check")],
                  [
                    Html.input(p.control, []),
                    Html.label(p.label, [text("Email me a team digest")]),
                  ],
                ),
            (),
          ),
          Ui.Switch_.view(
            ~id="updates",
            ~name="updates",
            ~checked=model.updates,
            ~on_change=Switch,
            ~to_view=
              p =>
                Html.div(
                  [Html.class_("check")],
                  [
                    Html.label(p.label, [text("Live project updates")]),
                    Html.button(
                      p.root @ [Html.class_("switch")],
                      [
                        Html.span(
                          [Html.aria("hidden", "true")],
                          [text(model.updates ? "On" : "Off")],
                        ),
                      ],
                    ),
                    p.hidden_input,
                  ],
                ),
            (),
          ),
          R.view(
            ~model=model.radio,
            ~items=radio_items,
            ~name="frequency",
            ~to_parent=m => Radio(m),
            ~to_view=
              p =>
                Html.div(
                  p.root,
                  [
                    Html.p(p.label, [text("Digest frequency")]),
                    Html.div(
                      [Html.class_("row choices")],
                      Array.to_list(
                        Array.map(
                          (i: R.item_parts(msg)) =>
                            Html.div(i.root, [text(i.item.label)]),
                          p.items,
                        ),
                      ),
                    ),
                    ...Option.to_list(p.hidden_input),
                  ],
                ),
            (),
          ),
          Html.p(
            [],
            [text("Remind me when storage reaches this percentage:")],
          ),
          S.view(
            ~model=model.slider,
            ~items=thumbs,
            ~name="threshold",
            ~to_parent=m => Slider(m),
            ~format_value=
              (_, value) =>
                string_of_int(int_of_float(value)) ++ " percent",
            ~to_view=
              p =>
                Html.div(
                  p.root,
                  [
                    Html.div(
                      p.track @ [Html.class_("track")],
                      [
                        Html.div(p.range @ [Html.class_("range")], []),
                        ...Array.to_list(
                             Array.map(
                               (thumb: S.thumb_parts(msg)) =>
                                 Html.div(
                                   thumb.root @ [Html.class_("thumb")],
                                   [
                                     text(
                                       string_of_int(
                                         int_of_float(thumb.value),
                                       ),
                                     ),
                                   ],
                                 ),
                               p.thumbs,
                             ),
                           ),
                      ],
                    ),
                    ...Array.to_list(
                         Array.map(
                           (thumb: S.thumb_parts(msg)) =>
                             Html.input(thumb.input, []),
                           p.thumbs,
                         ),
                       ),
                  ],
                ),
            (),
          ),
          Ui.Meter.view(
            ~id="storage",
            ~value=0.42,
            ~format=_ => "42 percent used",
            ~to_view=
              p =>
                Html.div(
                  [Html.class_("measure")],
                  [
                    Html.span(p.label, [text("Storage used: 42 of 100 GB")]),
                    Html.div(p.root, [Html.span([], [])]),
                  ],
                ),
            (),
          ),
          Ui.Progress.view(
            ~id="backup-progress",
            ~value=100.,
            ~to_view=
              p =>
                Html.div(
                  [Html.class_("measure")],
                  [
                    Html.span(p.label, [text("Latest backup complete")]),
                    Html.div(p.root, [Html.span([], [])]),
                  ],
                ),
            (),
          ),
        ],
      ),
      Html.div(
        [Html.class_("save-row")],
        [
          Ui.Button.button(
            ~id="save",
            ~button_type=Ui.Button.Submit,
            ~on_press=Noop,
            ~attrs=[Html.class_("primary")],
            [text("Save changes")],
          ),
          Html.p(
            [Html.id("saved-summary")],
            [text(model.preview ? model.saved : "")],
          ),
        ],
      ),
    ],
  );
let checklist = model =>
  section(
    "checklist",
    "Before the next field visit",
    [
      G.view(
        ~model=model.drag,
        ~containers=containers(model.order),
        ~to_parent=m => Drag(m),
        ~to_view=
          p =>
            Html.div(
              [],
              [
                Html.p(p.instructions, [text(p.instructions_text)]),
                Html.keyed(
                  "ul",
                  p.container("tasks") @ [Html.class_("tasks")],
                  Array.to_list(
                    Array.map(
                      (item: G.item) =>
                        (
                          item.key,
                          Html.li(
                            p.item(item.key),
                            [
                              Html.button(
                                p.handle(item.key),
                                [
                                  text("Move"),
                                  Html.span(
                                    [Html.class_("sr-only")],
                                    [text(" " ++ item.label)],
                                  ),
                                ],
                              ),
                              Html.span([], [text(item.label)]),
                            ],
                          ),
                        ),
                      model.order,
                    ),
                  ),
                ),
                Html.p(
                  p.live @ [Html.class_("sr-only")],
                  [text(p.live_text)],
                ),
              ],
            ),
        (),
      ),
      F.view(
        ~model=model.files,
        ~to_parent=m => Files(m),
        ~to_view=
          p =>
            Html.div(
              p.root @ [Html.class_("dropzone")],
              [
                Html.h3([], [text("Attach a field brief")]),
                Html.p(
                  [],
                  [
                    text(
                      "Drop text files here, or use the file picker. Up to 1 MB per file; nothing is uploaded.",
                    ),
                  ],
                ),
                Html.button(p.button, [text("Choose text files")]),
                Html.input(p.input, []),
                Html.ul(
                  [],
                  List.map(
                    name => Html.li([], [text(name)]),
                    model.filenames,
                  ),
                ),
              ],
            ),
        (),
      ),
    ],
  );
let activity = model =>
  section(
    "audit-section",
    "Workspace activity",
    [
      Html.p(
        [],
        [
          text(
            "10,000 local audit entries. Only the visible window is mounted; use pagination instead if full-page search or printing is essential.",
          ),
        ],
      ),
      button("jump", Jump, "Jump to latest entry"),
      V.view(
        ~model=model.virtual_,
        ~items=audit_items,
        ~to_parent=m => Virtual(m),
        ~to_view=
          p =>
            Html.div(
              p.container
              @ [
                Html.class_("audit-window"),
                Html.role("list"),
                Html.aria("label", "Audit entries"),
                Html.tabindex(0),
              ],
              [
                Html.keyed(
                  "div",
                  p.content @ [Html.role("presentation")],
                  List.map(
                    (row: V.row(msg)) =>
                      (
                        row.key,
                        Html.div(
                          row.attrs
                          @ [
                            Html.role("listitem"),
                            Html.class_("audit-row"),
                          ],
                          [
                            text(
                              "Entry "
                              ++ string_of_int(row.index + 1)
                              ++ ": field records synchronized",
                            ),
                          ],
                        ),
                      ),
                    p.rows,
                  ),
                ),
              ],
            ),
        (),
      ),
    ],
  );
let confirmation = model =>
  D.view(
    ~model=model.dialog,
    ~has_description=true,
    ~to_parent=m => Dialog(m),
    ~to_view=
      p =>
        Html.dialog(
          p.root,
          [
            Html.div(
              [Html.class_("dialog-body")],
              [
                Html.h2(p.title, [text("Archive this workspace?")]),
                Html.p(
                  p.description,
                  [
                    text(
                      "The workspace becomes read-only. Choose who will own its archive before continuing. This demo never deletes your files.",
                    ),
                  ],
                ),
                Html.div(
                  [Html.class_("row")],
                  [
                    button("cancel-archive", Cancel_delete, "Keep workspace"),
                    button("confirm-archive", Delete, "Archive workspace"),
                  ],
                ),
                Html.div(
                  [Html.class_("columns")],
                  [
                    listbox(model, "review"),
                    combo(model, "review"),
                    picker(model, "review"),
                  ],
                ),
                Html.div(
                  [Html.class_("row")],
                  [
                    menu(model, "review"),
                    tip(
                      model,
                      "review",
                      "Archive help",
                      "Archived workspaces can be restored by an administrator.",
                      Noop,
                    ),
                  ],
                ),
              ],
            ),
          ],
        ),
    (),
  );
let view = model =>
  Html.div(
    [],
    [
      Html.a(
        [Html.href("#workspace"), Html.class_("skip")],
        [text("Skip to workspace")],
      ),
      Html.header(
        [],
        [
          Html.div(
            [Html.class_("brand")],
            [text("Fieldwork"), Html.span([], [text("Team workspace")])],
          ),
          Ui.Nav.view(
            ~id="navigation",
            ~name=Ui.Nav.Label("Workspace"),
            ~keys=[|"workspace", "checklist", "help"|],
            ~current="workspace",
            ~current_token=Ui.Nav.Location,
            ~to_view=
              p =>
                Html.ul(
                  p.list,
                  Array.to_list(
                    Array.map(
                      (i: Ui.Nav.item(msg)) =>
                        Html.li(
                          [],
                          [
                            Html.a(
                              i.link @ [Html.href("#" ++ i.key)],
                              [
                                text(
                                  switch (i.key) {
                                  | "workspace" => "Workspace"
                                  | "checklist" => "Checklist"
                                  | _ => "Help"
                                  },
                                ),
                              ],
                            ),
                          ],
                        ),
                      p.items,
                    ),
                  ),
                ),
            (),
          ),
        ],
      ),
      Html.main(
        [Html.id("workspace"), Html.tabindex(-1)],
        [
          Html.div(
            [Html.class_("workspace-header")],
            [
              Html.div(
                [],
                [
                  Html.h1(
                    [],
                    [
                      text(
                        model.deleted
                          ? "Field station / Archived" : "Field station",
                      ),
                    ],
                  ),
                  Html.p(
                    [],
                    [
                      text(
                        "A shared home for the people and records behind each visit.",
                      ),
                    ],
                  ),
                ],
              ),
              Html.div(
                [
                  Html.class_("row toolbar"),
                  Html.role("group"),
                  Html.aria("label", "Workspace tools"),
                ],
                [
                  menu(model, "workspace"),
                  Ui.Popover.view(
                    ~model=model.filters,
                    ~to_parent=m => Filters(m),
                    ~to_view=
                      p =>
                        Html.div(
                          [],
                          [
                            Html.button(p.trigger, [text("View options")]),
                            Html.div(
                              p.panel @ [Html.class_("floating")],
                              [
                                Html.h2([], [text("View options")]),
                                Html.p(
                                  [],
                                  [
                                    text(
                                      "Choose whether to show the saved settings summary.",
                                    ),
                                  ],
                                ),
                                Html.label(
                                  [],
                                  [
                                    Html.input(
                                      [
                                        Html.type_("checkbox"),
                                        Html.checked(model.preview),
                                        Html.on_check(_ => Preview),
                                      ],
                                      [],
                                    ),
                                    text(" Show summary"),
                                  ],
                                ),
                                Html.button(p.close_button, [text("Done")]),
                              ],
                            ),
                          ],
                        ),
                    (),
                  ),
                  tip(
                    model,
                    "help",
                    "Keyboard help",
                    "Use Tab to move, arrows inside composite controls, and Escape to close the nearest popup.",
                    Faq,
                  ),
                  tip(
                    model,
                    "preview",
                    "Show summary",
                    "The saved settings summary appears below Save changes.",
                    Preview,
                  ),
                ],
              ),
            ],
          ),
          Ui.Tabs.view(
            ~model=model.tabs,
            ~items=tab_items,
            ~aria_label="Workspace sections",
            ~panel_focusable=_ => false,
            ~to_parent=m => Tabs(m),
            ~to_view=
              p =>
                Html.div(
                  [],
                  [
                    Html.div(
                      p.root @ [Html.class_("tabs")],
                      Array.to_list(
                        Array.map(
                          (i: Ui.Tabs.item_parts(msg)) =>
                            Html.div(i.tab, [text(i.item.label)]),
                          p.items,
                        ),
                      ),
                    ),
                    ...Array.to_list(
                         Array.map(
                           (i: Ui.Tabs.item_parts(msg)) =>
                             Html.div(
                               i.panel,
                               String.equal(i.item.key, "settings")
                                 ? [form(model), checklist(model)]
                                 : [activity(model)],
                             ),
                           p.items,
                         ),
                       ),
                  ],
                ),
            (),
          ),
          section(
            "help",
            "Workspace help",
            [
              Ui.Disclosure.view(
                ~id="faq",
                ~open_=model.faq,
                ~on_toggle=Faq,
                ~to_view=
                  p =>
                    Html.div(
                      [],
                      [
                        Html.h3(
                          [],
                          [
                            Html.button(
                              p.button,
                              [text("Who can change these defaults?")],
                            ),
                          ],
                        ),
                        p.mounted
                          ? Html.div(
                              p.panel,
                              [
                                Html.p(
                                  [],
                                  [
                                    text(
                                      "Workspace administrators can edit defaults. Members keep their personal notification preferences. All changes in this showcase are local and reset when you reload.",
                                    ),
                                  ],
                                ),
                              ],
                            )
                          : text(""),
                      ],
                    ),
                (),
              ),
            ],
          ),
          section(
            "archive-section",
            "Archive workspace",
            [
              Html.p(
                [],
                [
                  text(
                    "Close this station when the field season is over. You can review ownership before confirming.",
                  ),
                ],
              ),
              button("open-archive", Open_delete, "Review archive"),
            ],
          ),
          confirmation(model),
          notifications(model),
          Html.footer(
            [],
            [
              text(
                "Built with tea.ui. Headless behavior, application-owned design. No data leaves this page.",
              ),
            ],
          ),
        ],
      ),
    ],
  );
let subscriptions = model =>
  Sub.batch([
    Sub.map(m => Radio(m), R.subscriptions(model.radio)),
    Sub.map(m => Slider(m), S.subscriptions(model.slider)),
    Sub.map(m => Tabs(m), Ui.Tabs.subscriptions(model.tabs)),
    Sub.batch(
      List.map(
        ((key, value)) =>
          Sub.map(m => Listbox(key, m), L.subscriptions(value)),
        model.lists,
      ),
    ),
    Sub.batch(
      List.map(
        ((key, value)) =>
          Sub.map(m => Combo(key, m), C.subscriptions(value)),
        model.combos,
      ),
    ),
    Sub.batch(
      List.map(
        ((key, value)) =>
          Sub.map(m => Picker(key, m), P.subscriptions(value)),
        model.pickers,
      ),
    ),
    Sub.batch(
      List.map(
        ((key, value)) =>
          Sub.map(m => Menu(key, m), M.subscriptions(value)),
        model.menus,
      ),
    ),
    Sub.batch(
      List.map(
        ((key, value)) =>
          Sub.map(m => Tip(key, m), T.subscriptions(value)),
        model.tips,
      ),
    ),
    Sub.map(m => Filters(m), Ui.Popover.subscriptions(model.filters)),
    Sub.map(m => Dialog(m), D.subscriptions(model.dialog)),
    Sub.map(m => Toast(m), N.subscriptions(model.toast)),
    Sub.map(m => Drag(m), G.subscriptions(model.drag)),
    Sub.map(m => Files(m), F.subscriptions(model.files)),
    Sub.map(m => Virtual(m), V.subscriptions(model.virtual_)),
  ]);
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              name: "Field station",
              notes: "Record observations, share findings, leave a clear trail.",
              checked: true,
              updates: false,
              region: Ui.Select.Single(Some("eu")),
              radio:
                R.init(
                  ~id="frequency",
                  ~items=radio_items,
                  ~checked="weekly",
                  (),
                ),
              slider: S.init(~id="threshold", ~items=thumbs, ()),
              tabs: Ui.Tabs.init(~id="workspace-tabs", ~items=tab_items, ()),
              lists:
                List.map(
                  key =>
                    (
                      key,
                      L.init(~id=key ++ "-team", ~selected=["research"], ()),
                    ),
                  ["workspace", "review"],
                ),
              combos:
                List.map(
                  key =>
                    (
                      key,
                      C.refresh(
                        ~items=people,
                        C.init(~id=key ++ "-owner", ()),
                      ),
                    ),
                  ["workspace", "review"],
                ),
              pickers:
                List.map(
                  key => (key, P.init(~id=key ++ "-date", ~focus=today, ())),
                  ["workspace", "review"],
                ),
              menus:
                List.map(
                  key => (key, M.init(~id=key ++ "-actions", ())),
                  ["workspace", "review"],
                ),
              tips:
                List.map(
                  key => (key, T.init(~id=key ++ "-tip", ())),
                  ["help", "preview", "review"],
                ),
              group: H.Group.init(~skip_delay_ms=500, ()),
              filters:
                Ui.Popover.init(
                  ~id="filters",
                  ~role=Ui.Popover.Dialog("View options"),
                  ~focus_on_open=Ui.Popover.First_tabbable,
                  (),
                ),
              dialog:
                D.init(
                  ~id="archive",
                  ~kind=D.Alertdialog("cancel-archive"),
                  (),
                ),
              toast: N.init(~id="notifications", ()),
              contents: [],
              serial: 0,
              saved: "",
              deleted: false,
              drag: G.init(~id="checklist", ~drag_from_item=false, ()),
              order: initial_order,
              files:
                F.init(
                  ~id="brief",
                  ~multiple=true,
                  ~accept=[".txt", "text/plain"],
                  ~max_size=1000000.,
                  (),
                ),
              filenames: [],
              virtual_:
                V.init(
                  ~id="audit",
                  ~items=audit_items,
                  ~row_size=V.Fixed(44.),
                  ~viewport=264.,
                  (),
                ),
              faq: false,
              preview: true,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions,
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
