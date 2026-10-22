/** The API contract shared by the server and the browser client.

    Both sides compile this module, so a field rename or a rule change is a
    type error in whichever side falls behind, not a runtime surprise. */

type todo = {
  id: int,
  title: string,
  completed: bool,
};

/** POST /api/todos body. */
type create = {title: string};

/** PATCH /api/todos/:id body. Absent fields are left unchanged. */
type patch = {
  title: option(string),
  completed: option(bool),
};

/** Error body for every non-2xx API response. */
type problem = {message: string};

/** URL builders, used by the client for requests and by the server for docs. */
module Path: {
  let todos: string;
  let todo: int => string;
};

/** Trim, then require a non-empty title. The browser checks before sending and
    the server checks again; both run this exact function. (No length cap:
    String.length counts bytes natively but UTF-16 units under Melange.) */
let validate_title: string => result(string, string);

module Decode: {
  let todo: Tea_core.Decode.t(todo);
  let todos: Tea_core.Decode.t(list(todo));
  let create: Tea_core.Decode.t(create);
  let patch: Tea_core.Decode.t(patch);
  let problem: Tea_core.Decode.t(problem);
};

module Encode: {
  let todo: todo => Tea_core.Decode.Value.t;
  let todos: list(todo) => Tea_core.Decode.Value.t;
  let create: create => Tea_core.Decode.Value.t;
  let patch: patch => Tea_core.Decode.Value.t;
  let problem: problem => Tea_core.Decode.Value.t;
};
