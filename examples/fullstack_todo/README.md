# Full-stack todo

A Tea client and a native OCaml 5 server, both written in Reason, sharing one
API contract.

```
shared/   Contract: types, decoders, encoders, paths, validation (native + Melange)
server/   Eio + cohttp-eio HTTP server, SQLite store, JSON API, static files
client/   Tea element program (Melange) that talks to /api
```

## Run it

```sh
cd examples/fullstack_todo
docker compose up --build
```

Open http://127.0.0.1:8787/. Todos live in SQLite on the `todos` volume, so
they survive restarts (`docker compose down -v` wipes them). Docker is the only
requirement: the image installs OCaml, Melange, Eio and the SQLite headers.

## The shared contract

`shared/contract.rei` is the single source of truth. The library is built in
`(modes native melange)`, so the server links the native build and the client
links the Melange build of the same file:

- `Contract.todo`, `create`, `patch`, `problem`: request and response shapes.
- `Contract.Decode` / `Contract.Encode`: written once against
  `Tea_core.Decode` and its portable `Decode.Value.t`. The client feeds them to
  `Http.expect_json` and `Http.Json.of_value`. The server converts JSON text
  to `Decode.Value.t` with yojson (`server/json.re`).
- `Contract.Path`: the URLs the client requests.
- `Contract.validate_title`: the client runs it before sending and the server
  runs it again before writing.

If you rename a field or change a rule, the side that falls behind fails to
compile. `shared/contract_test.re` checks that encode and decode round-trip.

## API

| Method | Path | Body | Success | Errors |
| --- | --- | --- | --- | --- |
| GET | `/api/todos` | none | 200 `todo[]` | |
| POST | `/api/todos` | `{"title"}` | 201 `todo` | 400 malformed, 422 invalid |
| PATCH | `/api/todos/:id` | `{"title"?, "completed"?}` | 200 `todo` | 400, 404, 422 |
| DELETE | `/api/todos/:id` | none | 204 | 404 |

Errors have the body `{"message": "..."}`. Every other path serves the client,
falling back to `index.html` for extensionless routes.

## Without Docker

You need SQLite development headers (`libsqlite3-dev`, `sqlite-devel`, etc.).
Then:

```sh
opam install eio_main cohttp-eio sqlite3 yojson
dune build @examples/fullstack_todo/client/melange examples/fullstack_todo/server/main.exe
./_build/default/examples/fullstack_todo/server/main.exe \
  --static _build/default/examples/fullstack_todo/client/js --db todos.db
```

If those packages are missing, a repository-wide `dune build` still succeeds:
the server library is optional and `main.exe` becomes a stub that prints these
instructions.
