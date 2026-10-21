open Tea;

type todo = {
  id: int,
  title: string,
  completed: bool,
};
type filter =
  | All
  | Active
  | Completed;
type model = {
  todos: list(todo),
  draft: string,
  filter,
  next_id: int,
};
type msg =
  | Draft(string)
  | Add
  | Edit(int, string)
  | Toggle(int, bool)
  | Delete(int)
  | Filter(filter)
  | Seed;

let init = () => (
  {
    todos: [],
    draft: "",
    filter: All,
    next_id: 0,
  },
  Cmd.none,
);

let update = (model, msg) => {
  let model =
    switch (msg) {
    | Seed => {
        todos:
          List.init(1000, id =>
            {
              id,
              title: "Task " ++ string_of_int(id + 1),
              completed: false,
            }
          ),
        draft: "",
        filter: All,
        next_id: 1000,
      }
    | Draft(draft) => {
        ...model,
        draft,
      }
    | Add =>
      let title = String.trim(model.draft);
      if (title == "") {
        model;
      } else {
        {
          ...model,
          todos:
            model.todos
            @ [
              {
                id: model.next_id,
                title,
                completed: false,
              },
            ],
          draft: "",
          next_id: model.next_id + 1,
        };
      };
    | Edit(id, title) => {
        ...model,
        todos:
          List.map(
            todo =>
              todo.id == id
                ? {
                  ...todo,
                  title,
                }
                : todo,
            model.todos,
          ),
      }
    | Toggle(id, completed) => {
        ...model,
        todos:
          List.map(
            todo =>
              todo.id == id
                ? {
                  ...todo,
                  completed,
                }
                : todo,
            model.todos,
          ),
      }
    | Delete(id) => {
        ...model,
        todos: List.filter(todo => todo.id != id, model.todos),
      }
    | Filter(filter) => {
        ...model,
        filter,
      }
    };
  (model, Cmd.none);
};

/* Stable top-level view + unchanged record identities let lazy rows bail out. */
let row = todo =>
  Html.li(
    [
      Html.data("id", string_of_int(todo.id)),
      Html.classList([("completed", todo.completed)]),
    ],
    [
      Html.input(
        [
          Html.type_("checkbox"),
          Html.aria("label", "Complete " ++ todo.title),
          Html.checked(todo.completed),
          Html.on_check(checked => Toggle(todo.id, checked)),
        ],
        [],
      ),
      Html.input(
        [
          Html.type_("text"),
          Html.aria("label", "Edit task"),
          Html.value(todo.title),
          Html.on_input(title => Edit(todo.id, title)),
        ],
        [],
      ),
      Html.button(
        [Html.type_("button"), Html.on_click(Delete(todo.id))],
        [Html.text("Delete")],
      ),
    ],
  );

let view = model => {
  let visible =
    List.filter(
      todo =>
        switch (model.filter) {
        | All => true
        | Active => !todo.completed
        | Completed => todo.completed
        },
      model.todos,
    );
  Html.main(
    [],
    [
      Html.h1([], [Html.text("A thousand small things")]),
      Html.p(
        [],
        [
          Html.text(
            string_of_int(List.length(model.todos))
            ++ " tasks · keyed identity · lazy rows",
          ),
        ],
      ),
      Html.button(
        [Html.id("seed"), Html.on_click(Seed)],
        [Html.text("Seed 1,000 items")],
      ),
      Html.form(
        [Html.id("add-form"), Html.on_submit(Add)],
        [
          Html.input(
            [
              Html.id("draft"),
              Html.aria("label", "New task"),
              Html.value(model.draft),
              Html.placeholder("What needs doing?"),
              Html.on_input(text => Draft(text)),
            ],
            [],
          ),
          Html.button([Html.type_("submit")], [Html.text("Add")]),
        ],
      ),
      Html.nav(
        [],
        List.map(
          ((filter, label)) =>
            Html.button(
              [
                Html.id("filter-" ++ label),
                Html.on_click(Filter(filter)),
                Html.aria(
                  "pressed",
                  model.filter == filter ? "true" : "false",
                ),
              ],
              [Html.text(label)],
            ),
          [(All, "All"), (Active, "Active"), (Completed, "Completed")],
        ),
      ),
      Keyed.ul(
        [Html.id("todos")],
        List.map(
          todo => (string_of_int(todo.id), Html.lazy1(row, todo)),
          visible,
        ),
      ),
    ],
  );
};

let app =
  App.element(
    ~init,
    ~update,
    ~view,
    ~subscriptions=_ => Sub.none,
    ~node="#app",
    ~flags=(),
    (),
  );
