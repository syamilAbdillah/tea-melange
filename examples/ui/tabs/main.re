open Tea;
module T = Ui.Tabs;
let items: array(T.item) = [|
  {
    key: "profile",
    label: "Profile",
    disabled: false,
  },
  {
    key: "billing",
    label: "Billing (unavailable)",
    disabled: true,
  },
  {
    key: "privacy",
    label: "Privacy",
    disabled: false,
  },
  {
    key: "alerts",
    label: "Alerts",
    disabled: false,
  },
|];
type model = {
  groups: array(T.model),
  items: array(T.item),
  changes: int,
};
type msg =
  | Group(int, T.msg)
  | Remove
  | Restore
  | Empty
  | Select;
let update = (msg, model) =>
  switch (msg) {
  | Group(i, msg) =>
    let (group, cmd, out) =
      T.update(~items=i == 0 ? model.items : items, msg, model.groups[i]);
    let groups = Array.copy(model.groups);
    groups[i] = group;
    (
      {
        ...model,
        groups,
        changes: model.changes + List.length(out),
      },
      Cmd.map(msg => Group(i, msg), cmd),
    );
  | Remove
  | Restore
  | Empty
  | Select =>
    let next_items =
      switch (msg) {
      | Remove =>
        Array.of_list(
          List.filter(
            (item: T.item) => Some(item.key) != T.selected(model.groups[0]),
            Array.to_list(model.items),
          ),
        )
      | Restore => items
      | Empty => [||]
      | _ => model.items
      };
    let (group, cmd) =
      switch (msg) {
      | Select => T.set_selected(~items=next_items, "alerts", model.groups[0])
      | _ => T.reconcile(~items=next_items, model.groups[0])
      };
    let groups = Array.copy(model.groups);
    groups[0] = group;
    (
      {
        ...model,
        groups,
        items: next_items,
      },
      Cmd.map(msg => Group(0, msg), cmd),
    );
  };
let render = (model, i, title) => {
  let aria_label = i == 4 ? Some(title) : None;
  T.view(
    ~model=model.groups[i],
    ~items=i == 0 ? model.items : items,
    ~to_parent=msg => Group(i, msg),
    ~aria_label?,
    ~panel_rendering=i == 1 ? T.Selected_only : T.Keep_mounted,
    ~panel_focusable=key => !(i == 0 && String.equal(key, "privacy")),
    ~to_view=
      parts =>
        Html.section(
          [],
          [
            Html.h2(parts.label, [Html.text(title)]),
            Html.keyed(
              "div",
              parts.root,
              Array.to_list(
                Array.map(
                  (part: T.item_parts(msg)) =>
                    (
                      part.item.key,
                      Html.div(part.tab, [Html.text(part.item.label)]),
                    ),
                  parts.items,
                ),
              ),
            ),
            Html.keyed(
              "div",
              [],
              Array.to_list(
                Array.map(
                  (part: T.item_parts(msg)) =>
                    (
                      part.item.key,
                      Html.div(
                        part.panel,
                        part.render_panel
                          ? [
                            i == 0 && String.equal(part.item.key, "privacy")
                              ? Html.a(
                                  [
                                    Html.href("#privacy-help"),
                                    Html.id("privacy-link"),
                                  ],
                                  [Html.text("Read the privacy guide")],
                                )
                              : Html.p(
                                  [],
                                  [
                                    Html.text(
                                      part.item.label
                                      ++ " settings are ready to edit.",
                                    ),
                                  ],
                                ),
                          ]
                          : [],
                      ),
                    ),
                  parts.items,
                ),
              ),
            ),
          ],
        ),
    (),
  );
};
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Workspace settings")]),
      Html.p(
        [],
        [
          Html.text(
            "Automatic tabs follow focus. Manual tabs wait for Enter or Space before showing a panel.",
          ),
        ],
      ),
      render(model, 0, "Horizontal automatic"),
      Html.div(
        [],
        [
          Html.button(
            [
              Html.id("remove"),
              Html.type_("button"),
              Html.on_click(Remove),
            ],
            [Html.text("Remove selected tab")],
          ),
          Html.button(
            [
              Html.id("restore"),
              Html.type_("button"),
              Html.on_click(Restore),
            ],
            [Html.text("Restore tabs")],
          ),
          Html.button(
            [Html.id("empty"), Html.type_("button"), Html.on_click(Empty)],
            [Html.text("Clear available options")],
          ),
          Html.button(
            [
              Html.id("select"),
              Html.type_("button"),
              Html.on_click(Select),
            ],
            [Html.text("Select alerts")],
          ),
        ],
      ),
      render(model, 1, "Horizontal manual"),
      render(model, 2, "Vertical automatic"),
      render(model, 3, "Vertical manual"),
      render(model, 4, "Right-to-left automatic"),
      Html.p(
        [Html.id("privacy-help")],
        [
          Html.text(
            "Privacy guide: choose which workspace information to share.",
          ),
        ],
      ),
      Html.node(
        "output",
        [Html.id("activity"), Html.aria("live", "polite")],
        [Html.text("Changes: " ++ string_of_int(model.changes))],
      ),
      Html.button(
        [Html.id("after"), Html.type_("button")],
        [Html.text("Continue")],
      ),
    ],
  );
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              groups: [|
                T.init(~id="ha", ~items, ()),
                T.init(~id="hm", ~items, ~activation=T.Manual, ()),
                T.init(~id="va", ~items, ~orientation=T.Vertical, ()),
                T.init(
                  ~id="vm",
                  ~items,
                  ~orientation=T.Vertical,
                  ~activation=T.Manual,
                  (),
                ),
                T.init(~id="rtl", ~items, ~dir=T.Rtl, ()),
              |],
              items,
              changes: 0,
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
                (i, group) =>
                  Sub.map(msg => Group(i, msg), T.subscriptions(group)),
                model.groups,
              ),
            ),
          ),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
