open Tea;
module M = Ui.Menu;
let item = (~disabled=false, key, label, kind): M.item => {
  key,
  label,
  disabled,
  kind,
};
let files = [|
  item("new", "New file", M.Action),
  item("copy", "Copy", M.Action),
  item(~disabled=true, "delete", "Delete", M.Action),
  item("divider", "", M.Separator),
  item(
    "share",
    "Share",
    M.Submenu([|
      item("link", "Copy link", M.Action),
      item(
        "team",
        "Team",
        M.Submenu([|
          item("design", "Design team", M.Action),
          item("engineering", "Engineering team", M.Action),
        |]),
      ),
    |]),
  ),
  item("export", "Export", M.Action),
|];
let settings = [|
  item("status", "Status bar", M.Checkbox),
  item("grid", "Grid", M.Checkbox),
  item(
    "density",
    "Density",
    M.Group([|
      item("comfortable", "Comfortable", M.Radio("density")),
      item("compact", "Compact", M.Radio("density")),
    |]),
  ),
|];
let items = id => String.equal(id, "view") ? settings : files;
let specimens = [
  ("file", "File actions", M.Ltr),
  ("view", "View", M.Ltr),
  ("rtl", "RTL actions", M.Rtl),
  ("animated", "Animated actions", M.Ltr),
];
type msg =
  | Menu(string, M.msg);
type model = {
  menus: list((string, M.model)),
  activity: string,
};
let describe = out =>
  switch (out) {
  | M.Opened => "Opened"
  | M.Closed(_) => "Closed"
  | M.Selected(key) => "Selected " ++ key
  | M.Checked(key, value) =>
    "Checked " ++ key ++ " " ++ string_of_bool(value)
  | M.Radio_selected(group, key) => "Radio " ++ group ++ " " ++ key
  };
let update = (Menu(id, message), model) => {
  let current = List.assoc(id, model.menus);
  let (menu, cmd, outs) = M.update(~items=items(id), message, current);
  (
    {
      menus:
        List.map(
          ((key, old)) => (key, String.equal(id, key) ? menu : old),
          model.menus,
        ),
      activity:
        List.fold_left(
          (text, out) => text ++ "; " ++ describe(out),
          model.activity,
          outs,
        ),
    },
    Cmd.map(message => Menu(id, message), cmd),
  );
};
let rec entry = (p: M.item_parts(msg)) => {
  let content =
    switch (p.item.kind) {
    | M.Group(_) => [
        Html.div(
          p.label @ [Html.class_("group-label")],
          [Html.text(p.item.label)],
        ),
        Html.div(
          [Html.role("none")],
          Array.to_list(Array.map(entry, p.children)),
        ),
      ]
    | M.Separator => []
    | kind => [
        Html.span(
          [Html.aria("hidden", "true"), Html.class_("icon")],
          [
            Html.text(
              switch (kind) {
              | M.Checkbox
              | M.Radio(_) => p.checked ? "✓" : ""
              | M.Submenu(_) => "▸"
              | _ => String.equal(p.item.key, "new") ? "+" : "◇"
              },
            ),
          ],
        ),
        Html.span([], [Html.text(p.item.label)]),
        Html.span(
          [Html.aria("hidden", "true"), Html.class_("shortcut")],
          [Html.text(String.equal(p.item.key, "copy") ? "Ctrl C" : "")],
        ),
      ]
    };
  Html.div(p.attrs, content);
};
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Actions at hand")]),
      Html.p(
        [],
        [
          Html.text(
            "An actions menu is a temporary workspace, not site navigation. Open with Enter, Space or an arrow; explore the nested Share menu.",
          ),
        ],
      ),
      Html.button([Html.id("before")], [Html.text("Before menus")]),
      Html.div(
        [Html.class_("specimens")],
        List.map(
          ((id, title, dir)) =>
            Html.section(
              [Html.attribute("dir", dir == M.Rtl ? "rtl" : "ltr")],
              [
                Html.h2([], [Html.text(title)]),
                M.view(
                  ~model=List.assoc(id, model.menus),
                  ~items=items(id),
                  ~to_parent=message => Menu(id, message),
                  ~to_view=
                    parts =>
                      Html.div(
                        [],
                        [
                          Html.button(parts.trigger, [Html.text(title)]),
                          ...Array.to_list(
                               Array.map(
                                 (p: M.panel_parts(msg)) =>
                                   Html.div(
                                     p.attrs
                                     @ [
                                       Html.class_(
                                         String.equal(id, "animated")
                                           ? "panel animated" : "panel",
                                       ),
                                     ],
                                     Array.to_list(
                                       Array.map(entry, p.items),
                                     ),
                                   ),
                                 parts.panels,
                               ),
                             ),
                        ],
                      ),
                  (),
                ),
              ],
            ),
          specimens,
        ),
      ),
      Html.button([Html.id("after")], [Html.text("After menus")]),
      Html.p(
        [Html.id("activity"), Html.role("status")],
        [Html.text(model.activity)],
      ),
    ],
  );
let () = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              menus:
                List.map(
                  ((id, _, dir)) =>
                    (
                      id,
                      M.init(
                        ~id,
                        ~dir,
                        ~checked=["status"],
                        ~radio=[("density", "comfortable")],
                        (),
                      ),
                    ),
                  specimens,
                ),
              activity: "Ready",
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch(
            List.map(
              ((id, menu)) =>
                Sub.map(
                  message => Menu(id, message),
                  M.subscriptions(menu),
                ),
              model.menus,
            ),
          ),
      ~node="#app",
      ~flags=(),
      (),
    );
  ignore(app);
};
