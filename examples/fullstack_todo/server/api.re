/* The JSON API, independent of the HTTP library: method, path segments and
   body text in; status and optional JSON value out. */
open Todo_shared;

let problem = (status, message) => (
  status,
  Some(Contract.Encode.problem({ message: message })),
);

let decode_body = (decoder, body) =>
  switch (Json.of_string(body)) {
  | Ok(value) => Tea_core.Decode.run(decoder, value)
  | Error(message) => Error("Malformed JSON: " ++ message)
  };

/* Validate the title only when the patch changes it. */
let validate_patch = (patch: Contract.patch) =>
  switch (patch.title) {
  | None => Ok(patch)
  | Some(title) =>
    Result.map(
      title =>
        {
          ...patch,
          title: Some(title),
        },
      Contract.validate_title(title),
    )
  };

let with_id = (raw, respond) =>
  switch (int_of_string_opt(raw)) {
  | Some(id) => respond(id)
  | None => problem(404, "No such todo")
  };

let handle = (store, ~meth: Http.Method.t, ~path, ~body) =>
  switch (meth, path) {
  | (`GET, ["api", "todos"]) => (
      200,
      Some(Contract.Encode.todos(Store.list(store))),
    )
  | (`POST, ["api", "todos"]) =>
    switch (decode_body(Contract.Decode.create, body)) {
    | Error(message) => problem(400, message)
    | Ok({ title }) =>
      switch (Contract.validate_title(title)) {
      | Error(message) => problem(422, message)
      | Ok(title) => (
          201,
          Some(Contract.Encode.todo(Store.create(store, title))),
        )
      }
    }
  | (`PATCH, ["api", "todos", raw]) =>
    with_id(raw, id =>
      switch (decode_body(Contract.Decode.patch, body)) {
      | Error(message) => problem(400, message)
      | Ok(patch) =>
        switch (validate_patch(patch)) {
        | Error(message) => problem(422, message)
        | Ok(patch) =>
          switch (Store.update(store, id, patch)) {
          | Some(todo) => (200, Some(Contract.Encode.todo(todo)))
          | None => problem(404, "No such todo")
          }
        }
      }
    )
  | (`DELETE, ["api", "todos", raw]) =>
    with_id(raw, id =>
      Store.delete(store, id) ? (204, None) : problem(404, "No such todo")
    )
  | (_, ["api", "todos"] | ["api", "todos", _]) =>
    problem(405, "Method not allowed")
  | _ => problem(404, "No such endpoint")
  };
