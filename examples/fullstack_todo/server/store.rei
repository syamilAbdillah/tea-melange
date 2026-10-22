/** SQLite persistence. Calls are synchronous; the server runs one domain, so
    each call completes without interleaving other fibers. */

type t;

/** Open (or create) the database file and apply the schema. */
let open_: string => t;
let list: t => list(Todo_shared.Contract.todo);
let create: (t, string) => Todo_shared.Contract.todo;
/** [None] when no todo has that id. */
let update:
  (t, int, Todo_shared.Contract.patch) => option(Todo_shared.Contract.todo);
/** [false] when no todo has that id. */
let delete: (t, int) => bool;
