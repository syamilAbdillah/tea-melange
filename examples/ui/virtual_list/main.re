open Tea;
module V = Ui.Virtual_list;
let fixed = V.items(Array.init(100000, i => "fixed-" ++ string_of_int(i)));
let variable =
  V.items(Array.init(10000, i => "variable-" ++ string_of_int(i)));
let options = V.items(Array.init(1000, i => "option-" ++ string_of_int(i)));
/* A reproducible integer seed controls paragraph lengths, never Math.random. */
let paragraphs =
  Array.init(
    10000,
    i => {
      let seed = (i * 1664525 + 1013904223) land 0x7fffffff;
      String.concat(
        " ",
        List.init(1 + seed mod 15, _ =>
          "Measured rows keep their place as the window moves."
        ),
      );
    },
  );
type msg =
  | Fixed(V.msg)
  | Variable(V.msg)
  | Options(V.msg)
  | Jump_input(string)
  | Alignment(string)
  | Jump_fixed
  | Jump_variable
  | Navigate(string)
  | Choose(int)
  | Focus_options
  | Table(bool)
  | Grow;
type model = {
  fixed: V.model,
  variable: V.model,
  options: V.model,
  jump: string,
  align: string,
  active: int,
  expanded: bool,
  table: bool,
};
let move = (index, model) => {
  let active = max(0, min(999, index));
  let (options, cmd) =
    V.scroll_to_index(~items=options, ~index=active, model.options);
  (
    {
      ...model,
      options,
      active,
    },
    Cmd.map(msg => Options(msg), cmd),
  );
};
let alignment = key =>
  switch (key) {
  | "center" => V.Center
  | "end" => V.End
  | "auto" => V.Auto
  | _ => V.Start
  };
let update = (msg, model) =>
  switch (msg) {
  | Fixed(msg) =>
    let (fixed_, cmd, _) = V.update(~items=fixed, msg, model.fixed);
    (
      {
        ...model,
        fixed: fixed_,
      },
      Cmd.map(msg => Fixed(msg), cmd),
    );
  | Variable(msg) =>
    let (variable_, cmd, _) = V.update(~items=variable, msg, model.variable);
    (
      {
        ...model,
        variable: variable_,
      },
      Cmd.map(msg => Variable(msg), cmd),
    );
  | Options(msg) =>
    let (options_, cmd, _) = V.update(~items=options, msg, model.options);
    (
      {
        ...model,
        options: options_,
      },
      Cmd.map(msg => Options(msg), cmd),
    );
  | Jump_input(jump) => (
      {
        ...model,
        jump,
      },
      Cmd.none,
    )
  | Alignment(align) => (
      {
        ...model,
        align,
      },
      Cmd.none,
    )
  | Jump_fixed =>
    let index = Option.value(~default=0, int_of_string_opt(model.jump));
    let (fixed_, cmd) =
      V.scroll_to_index(
        ~items=fixed,
        ~index,
        ~align=alignment(model.align),
        model.fixed,
      );
    (
      {
        ...model,
        fixed: fixed_,
      },
      Cmd.map(msg => Fixed(msg), cmd),
    );
  | Jump_variable =>
    let index = Option.value(~default=0, int_of_string_opt(model.jump));
    let (variable_, cmd) =
      V.scroll_to_index(
        ~items=variable,
        ~index,
        ~align=alignment(model.align),
        model.variable,
      );
    (
      {
        ...model,
        variable: variable_,
      },
      Cmd.map(msg => Variable(msg), cmd),
    );
  | Choose(index) =>
    let (model, cmd) = move(index, model);
    (model, Cmd.batch([cmd, Browser_dom.focus_cmd("options-viewport")]));
  | Focus_options => move(model.active, model)
  | Table(table) => (
      {
        ...model,
        table,
      },
      Cmd.none,
    )
  | Navigate(key) =>
    move(
      switch (key) {
      | "ArrowDown" => model.active + 1
      | "ArrowUp" => model.active - 1
      | "Home" => 0
      | "End" => 999
      | "PageDown" => model.active + 7
      | "PageUp" => model.active - 7
      | _ => model.active
      },
      model,
    )
  | Grow => (
      {
        ...model,
        expanded: !model.expanded,
      },
      Cmd.none,
    )
  };
let navigation = {
  open Decode;
  let flag = name => one_of([field(name, bool), succeed(false)]);
  map3(
    (key, modified, composing) => (key, modified, composing),
    field("key", string),
    map3(
      (ctrl, alt, meta) => ctrl || alt || meta,
      flag("ctrlKey"),
      flag("altKey"),
      flag("metaKey"),
    ),
    flag("isComposing"),
  )
  |> and_then(((key, modified, composing)) =>
       !modified
       && !composing
       && List.mem(
            key,
            ["ArrowDown", "ArrowUp", "Home", "End", "PageDown", "PageUp"],
          )
         ? succeed((Navigate(key), true)) : fail("unhandled")
     );
};
let list =
    (
      ~model,
      ~items,
      ~to_parent,
      ~label,
      ~variable_=false,
      ~expanded=false,
      ~table=false,
      (),
    ) =>
  V.view(
    ~model,
    ~items,
    ~to_parent,
    ~semantics=table ? V.Grid : V.Set,
    ~to_view=
      parts =>
        Html.div(
          parts.container
          @ [
            Html.role(table ? "table" : "list"),
            Html.aria("label", label),
            Html.tabindex(0),
            Html.style("height", "240px"),
            Html.class_("window"),
          ],
          [
            Html.keyed(
              "div",
              parts.content @ [Html.role("presentation")],
              List.map(
                (row: V.row(msg)) =>
                  (
                    row.key,
                    Html.div(
                      row.attrs
                      @ [
                        Html.role(table ? "row" : "listitem"),
                        Html.class_("row"),
                      ],
                      [
                        (
                          text =>
                            table
                              ? Html.div([Html.role("cell")], [text])
                              : text
                        )(
                          Html.text(
                            variable_
                              ? "Row "
                                ++ string_of_int(row.index)
                                ++ ". "
                                ++ paragraphs[row.index]
                                ++ (
                                  expanded
                                    ? " Extra height. Extra height. Extra height. Extra height. Extra height. Extra height. Extra height. Extra height."
                                    : ""
                                )
                              : "Row " ++ string_of_int(row.index),
                          ),
                        ),
                      ],
                    ),
                  ),
                parts.rows,
              ),
            ),
          ],
        ),
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("A window into a long list")]),
      Html.p(
        [],
        [
          Html.text(
            "Only the rows near the viewport are mounted. Scroll, jump, or use the keyboard to explore stable keyed rows.",
          ),
        ],
      ),
      Html.label(
        [Html.for_("jump")],
        [Html.text("Jump to index (zero-based)")],
      ),
      Html.input(
        [
          Html.id("jump"),
          Html.type_("number"),
          Html.value(model.jump),
          Html.on_input(value => Jump_input(value)),
        ],
        [],
      ),
      Html.label([Html.for_("align")], [Html.text("Jump alignment")]),
      Html.select(
        [
          Html.id("align"),
          Html.value(model.align),
          Html.on_change(value => Alignment(value)),
        ],
        List.map(
          key => Html.option([Html.value(key)], [Html.text(key)]),
          ["start", "center", "end", "auto"],
        ),
      ),
      Html.button(
        [Html.id("jump-fixed"), Html.on_click(Jump_fixed)],
        [Html.text("Jump fixed")],
      ),
      Html.button(
        [Html.id("jump-variable"), Html.on_click(Jump_variable)],
        [Html.text("Jump measured")],
      ),
      Html.h2([], [Html.text("100,000 fixed rows")]),
      Html.label(
        [],
        [
          Html.input(
            [
              Html.id("table"),
              Html.type_("checkbox"),
              Html.checked(model.table),
              Html.on_check(value => Table(value)),
            ],
            [],
          ),
          Html.text(" Use table row indices instead of set positions"),
        ],
      ),
      list(
        ~model=model.fixed,
        ~items=fixed,
        ~to_parent=msg => Fixed(msg),
        ~label="Fixed rows",
        ~table=model.table,
        (),
      ),
      Html.h2([], [Html.text("10,000 measured rows")]),
      Html.button(
        [Html.id("grow"), Html.on_click(Grow)],
        [Html.text("Change paragraph lengths")],
      ),
      list(
        ~model=model.variable,
        ~items=variable,
        ~to_parent=msg => Variable(msg),
        ~label="Measured rows",
        ~variable_=true,
        ~expanded=model.expanded,
        (),
      ),
      Html.h2([], [Html.text("Keep an active row in the window")]),
      Html.p(
        [Html.id("keys")],
        [
          Html.text(
            "Arrow keys move; Home and End jump to the ends. Page Up and Page Down move seven rows. Selection follows focus; Tab leaves the list.",
          ),
        ],
      ),
      V.view(
        ~model=model.options,
        ~items=options,
        ~pinned=[model.active],
        ~to_parent=msg => Options(msg),
        ~to_view=
          parts =>
            Html.div(
              parts.container
              @ [
                Html.role("listbox"),
                Html.on_focus(Focus_options),
                Html.aria("label", "Keyboard rows"),
                Html.aria("describedby", "keys"),
                Html.aria(
                  "activedescendant",
                  V.row_id(
                    ~key="option-" ++ string_of_int(model.active),
                    model.options,
                  ),
                ),
                Html.tabindex(0),
                Html.prevent_default_on("keydown", navigation),
                Html.style("height", "224px"),
                Html.class_("window"),
              ],
              [
                Html.keyed(
                  "div",
                  parts.content @ [Html.role("presentation")],
                  List.map(
                    (row: V.row(msg)) =>
                      (
                        row.key,
                        Html.div(
                          row.attrs
                          @ [
                            Html.role("option"),
                            Html.aria(
                              "selected",
                              row.index == model.active ? "true" : "false",
                            ),
                            Html.on_click(Choose(row.index)),
                            Html.class_("row"),
                          ],
                          [Html.text("Row " ++ string_of_int(row.index))],
                        ),
                      ),
                    parts.rows,
                  ),
                ),
              ],
            ),
        (),
      ),
      Html.button([Html.id("after")], [Html.text("After the list")]),
    ],
  );
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              fixed:
                V.init(
                  ~id="fixed",
                  ~items=fixed,
                  ~row_size=V.Fixed(32.),
                  ~viewport=240.,
                  (),
                ),
              variable:
                V.init(
                  ~id="variable",
                  ~items=variable,
                  ~row_size=V.Estimated(80.),
                  ~viewport=240.,
                  ~overscan=4,
                  (),
                ),
              options:
                V.init(
                  ~id="options",
                  ~items=options,
                  ~row_size=V.Fixed(32.),
                  ~viewport=224.,
                  (),
                ),
              jump: "5000",
              align: "start",
              active: 0,
              expanded: false,
              table: false,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch([
            Sub.map(msg => Fixed(msg), V.subscriptions(model.fixed)),
            Sub.map(msg => Variable(msg), V.subscriptions(model.variable)),
            Sub.map(msg => Options(msg), V.subscriptions(model.options)),
          ]),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
