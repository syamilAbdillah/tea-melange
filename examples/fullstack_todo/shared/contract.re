module D = Tea_core.Decode;
module V = Tea_core.Decode.Value;

type todo = {
  id: int,
  title: string,
  completed: bool,
};
type create = {title: string};
type patch = {
  title: option(string),
  completed: option(bool),
};
type problem = {message: string};

module Path = {
  let todos = "/api/todos";
  let todo = id => todos ++ "/" ++ string_of_int(id);
};

let validate_title = title => {
  let title = String.trim(title);
  title == "" ? Error("Title is required") : Ok(title);
};

module Decode = {
  /* Absent or null means None; a present value of the wrong type is an error,
     unlike Decode.optional, which would silently drop it. */
  let optional_field = (name, decoder) =>
    D.and_then(
      present =>
        present
          ? D.field(name, D.map(value => Some(value), decoder))
          : D.succeed(None),
      D.map(
        fun
        | None
        | Some(V.Null) => false
        | Some(_) => true,
        D.optional(D.field(name, D.value)),
      ),
    );

  let todo =
    D.map3(
      (id, title, completed) =>
        {
          id,
          title,
          completed,
        },
      D.field("id", D.int),
      D.field("title", D.string),
      D.field("completed", D.bool),
    );
  let todos = D.list(todo);
  let create = D.map(title => { title: title }, D.field("title", D.string));
  let patch =
    D.map2(
      (title, completed) =>
        {
          title,
          completed,
        },
      optional_field("title", D.string),
      optional_field("completed", D.bool),
    );
  let problem =
    D.map(message => { message: message }, D.field("message", D.string));
};

module Encode = {
  let todo = (todo: todo) =>
    V.Object([
      ("id", V.Number(float_of_int(todo.id))),
      ("title", V.String(todo.title)),
      ("completed", V.Bool(todo.completed)),
    ]);
  let todos = todos => V.Array(Array.of_list(List.map(todo, todos)));
  let create = (create: create) =>
    V.Object([("title", V.String(create.title))]);
  let patch = (patch: patch) =>
    V.Object(
      List.filter_map(
        Fun.id,
        [
          Option.map(title => ("title", V.String(title)), patch.title),
          Option.map(
            completed => ("completed", V.Bool(completed)),
            patch.completed,
          ),
        ],
      ),
    );
  let problem = problem =>
    V.Object([("message", V.String(problem.message))]);
};
