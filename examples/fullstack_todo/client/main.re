open Tea;
open Todo_shared;

/* The server owns the list; the client holds what it last heard back. */
type remote =
  | Loading
  | Failed
  | Ready(list(Contract.todo));
type model = {
  todos: remote,
  draft: string,
  saving: bool,
  error: option(string),
};
type msg =
  | Loaded(result(list(Contract.todo), Http.error))
  | Retry
  | Draft(string)
  | Submit
  | Created(result(Contract.todo, Http.error))
  | Toggle(int, bool)
  | Updated(result(Contract.todo, Http.error))
  | Delete(int)
  | Deleted(int, result(unit, Http.error));

let describe: Http.error => string =
  fun
  | Http.Bad_status(422) => "The server rejected that title."
  | Http.Bad_status(404) => "That todo no longer exists."
  | Http.Bad_status(status) =>
    "Server error (" ++ string_of_int(status) ++ ")."
  | Http.Bad_body(message) => "Unexpected response: " ++ message
  | Http.Bad_url(_)
  | Http.Timeout
  | Http.Network_error => "Could not reach the server.";

let json = value => Http.json_body(Http.Json.of_value(value));

let fetch_todos = () =>
  Http.get(
    ~url=Contract.Path.todos,
    ~expect=
      Http.expect_json(result => Loaded(result), Contract.Decode.todos),
  );
let create = title =>
  Http.post(
    ~url=Contract.Path.todos,
    ~body=json(Contract.Encode.create({ title: title })),
    ~expect=
      Http.expect_json(result => Created(result), Contract.Decode.todo),
  );
let patch = (id, patch) =>
  Http.request(
    ~method_="PATCH",
    ~headers=[],
    ~url=Contract.Path.todo(id),
    ~body=json(Contract.Encode.patch(patch)),
    ~expect=
      Http.expect_json(result => Updated(result), Contract.Decode.todo),
    (),
  );
let delete = id =>
  Http.request(
    ~method_="DELETE",
    ~headers=[],
    ~url=Contract.Path.todo(id),
    ~body=Http.empty_body,
    ~expect=Http.expect_whatever(result => Deleted(id, result)),
    (),
  );

let init = () => (
  {
    todos: Loading,
    draft: "",
    saving: false,
    error: None,
  },
  fetch_todos(),
);

let map_todos = (f, model) =>
  switch (model.todos) {
  | Ready(todos) => {
      ...model,
      todos: Ready(f(todos)),
    }
  | Loading
  | Failed => model
  };
let fail = (error, model) => (
  {
    ...model,
    saving: false,
    error: Some(describe(error)),
  },
  Cmd.none,
);

let update = (msg, model) =>
  switch (msg) {
  | Loaded(Ok(todos)) => (
      {
        ...model,
        todos: Ready(todos),
      },
      Cmd.none,
    )
  | Loaded(Error(_)) => (
      {
        ...model,
        todos: Failed,
      },
      Cmd.none,
    )
  | Retry => (
      {
        ...model,
        todos: Loading,
      },
      fetch_todos(),
    )
  | Draft(draft) => (
      {
        ...model,
        draft,
      },
      Cmd.none,
    )
  | Submit when model.saving => (model, Cmd.none)
  /* The same validator the server runs: bad input never leaves the browser. */
  | Submit =>
    switch (Contract.validate_title(model.draft)) {
    | Error(message) => (
        {
          ...model,
          error: Some(message),
        },
        Cmd.none,
      )
    | Ok(title) => (
        {
          ...model,
          saving: true,
          error: None,
        },
        create(title),
      )
    }
  | Created(Ok(todo)) => (
      {
        ...map_todos(todos => todos @ [todo], model),
        draft: "",
        saving: false,
      },
      Cmd.none,
    )
  | Toggle(id, completed) => (
      model,
      patch(
        id,
        {
          title: None,
          completed: Some(completed),
        },
      ),
    )
  | Updated(Ok(updated)) => (
      map_todos(
        List.map((todo: Contract.todo) =>
          todo.id == updated.id ? updated : todo
        ),
        model,
      ),
      Cmd.none,
    )
  | Delete(id) => (model, delete(id))
  | Deleted(id, Ok ()) => (
      map_todos(List.filter((todo: Contract.todo) => todo.id != id), model),
      Cmd.none,
    )
  | Created(Error(error))
  | Updated(Error(error))
  | Deleted(_, Error(error)) => fail(error, model)
  };

let row = (todo: Contract.todo) =>
  Html.li(
    [Html.classList([("completed", todo.completed)])],
    [
      Html.label(
        [],
        [
          Html.input(
            [
              Html.type_("checkbox"),
              Html.checked(todo.completed),
              Html.on_check(completed => Toggle(todo.id, completed)),
            ],
            [],
          ),
          Html.span([], [Html.text(todo.title)]),
        ],
      ),
      Html.button(
        [
          Html.type_("button"),
          Html.aria("label", "Delete " ++ todo.title),
          Html.on_click(Delete(todo.id)),
        ],
        [Html.text({js|×|js})],
      ),
    ],
  );

let list = model =>
  switch (model.todos) {
  | Loading => Html.p([Html.id("loading")], [Html.text({js|Loading…|js})])
  | Failed =>
    Html.p(
      [Html.id("load-error")],
      [
        Html.text("Could not load todos. "),
        Html.button([Html.on_click(Retry)], [Html.text("Retry")]),
      ],
    )
  | Ready([]) => Html.p([Html.id("empty")], [Html.text("Nothing to do.")])
  | Ready(todos) =>
    Keyed.ul(
      [Html.id("todos")],
      List.map(
        (todo: Contract.todo) =>
          (string_of_int(todo.id), Html.lazy1(row, todo)),
        todos,
      ),
    )
  };

let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Todos")]),
      Html.p(
        [],
        [Html.text("Eio + SQLite server, Tea client, one shared contract.")],
      ),
      Html.form(
        [Html.id("add-form"), Html.on_submit(Submit)],
        [
          Html.input(
            [
              Html.id("draft"),
              Html.aria("label", "New todo"),
              Html.placeholder("What needs doing?"),
              Html.value(model.draft),
              Html.on_input(text => Draft(text)),
            ],
            [],
          ),
          Html.button(
            [Html.type_("submit"), Html.disabled(model.saving)],
            [Html.text("Add")],
          ),
        ],
      ),
      switch (model.error) {
      | Some(message) =>
        Html.p(
          [Html.id("error"), Html.role("alert")],
          [Html.text(message)],
        )
      | None => Html.text("")
      },
      list(model),
    ],
  );

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
