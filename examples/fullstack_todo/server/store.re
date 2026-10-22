open Todo_shared;

type t = Sqlite3.db;

let fail = db => failwith("sqlite: " ++ Sqlite3.errmsg(db));

let exec = (db, sql) =>
  if (!Sqlite3.Rc.is_success(Sqlite3.exec(db, sql))) {
    fail(db);
  };

let open_ = path => {
  let db = Sqlite3.db_open(path);
  exec(
    db,
    "CREATE TABLE IF NOT EXISTS todos (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       title TEXT NOT NULL,
       completed INTEGER NOT NULL DEFAULT 0
     )",
  );
  db;
};

let row = (stmt): Contract.todo => {
  id: Sqlite3.column_int(stmt, 0),
  title: Sqlite3.column_text(stmt, 1),
  completed: Sqlite3.column_int(stmt, 2) != 0,
};

/* Every statement selects (id, title, completed), via SELECT or RETURNING. */
let query = (db, sql, params) => {
  let stmt = Sqlite3.prepare(db, sql);
  let rec rows = acc =>
    switch (Sqlite3.step(stmt)) {
    | Sqlite3.Rc.ROW => rows([row(stmt), ...acc])
    | Sqlite3.Rc.DONE => List.rev(acc)
    | _ => fail(db)
    };
  Fun.protect(
    ~finally=() => ignore(Sqlite3.finalize(stmt)),
    () => {
      List.iteri(
        (index, param) =>
          if (!Sqlite3.Rc.is_success(Sqlite3.bind(stmt, index + 1, param))) {
            fail(db);
          },
        params,
      );
      rows([]);
    },
  );
};

let id = id => Sqlite3.Data.INT(Int64.of_int(id));
let optional = (encode, value) =>
  Option.fold(~none=Sqlite3.Data.NULL, ~some=encode, value);

let list = db =>
  query(db, "SELECT id, title, completed FROM todos ORDER BY id", []);

let create = (db, title) =>
  switch (
    query(
      db,
      "INSERT INTO todos (title) VALUES (?) RETURNING id, title, completed",
      [Sqlite3.Data.TEXT(title)],
    )
  ) {
  | [todo] => todo
  | _ => fail(db)
  };

let update = (db, todo_id, patch: Contract.patch) =>
  switch (
    query(
      db,
      "UPDATE todos
       SET title = coalesce(?, title), completed = coalesce(?, completed)
       WHERE id = ?
       RETURNING id, title, completed",
      [
        optional(title => Sqlite3.Data.TEXT(title), patch.title),
        optional(
          completed => Sqlite3.Data.INT(completed ? 1L : 0L),
          patch.completed,
        ),
        id(todo_id),
      ],
    )
  ) {
  | [todo] => Some(todo)
  | _ => None
  };

let delete = (db, todo_id) =>
  query(
    db,
    "DELETE FROM todos WHERE id = ? RETURNING id, title, completed",
    [id(todo_id)],
  )
  != [];
