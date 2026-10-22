/* One Eio process serves the JSON API under /api and the Melange client
   everywhere else, so the browser talks to a single origin. */

let content_type = file =>
  switch (Filename.extension(file)) {
  | ".html" => "text/html; charset=utf-8"
  | ".js"
  | ".mjs" => "text/javascript"
  | ".json" => "application/json; charset=utf-8"
  | ".css" => "text/css"
  | _ => "application/octet-stream"
  };

let respond = (~status, ~content_type, body) =>
  Cohttp_eio.Server.respond_string(
    ~headers=
      Http.Header.of_list([
        ("content-type", content_type),
        ("cache-control", "no-store"),
      ]),
    ~status=`Code(status),
    ~body,
    (),
  );

let not_found = () =>
  respond(~status=404, ~content_type="text/plain", "Not found");

/* Extensionless paths are client routes and get index.html; missing modules
   and assets stay 404 rather than being masked by HTML. */
let serve_static = (root, segments) =>
  if (List.exists(
        segment =>
          segment == ".."
          || String.contains(segment, '/')
          || String.contains(segment, '\\')
          || String.contains(segment, '\000'),
        segments,
      )) {
    not_found();
  } else {
    let relative =
      segments == [] ? "index.html" : String.concat("/", segments);
    let load = file =>
      switch (Eio.Path.load(Eio.Path.(root / file))) {
      | data => Some((file, data))
      | exception (Eio.Io(_)) => None
      };
    let found =
      switch (load(relative)) {
      | Some(_) as hit => hit
      | None when Filename.extension(relative) == "" => load("index.html")
      | None => None
      };
    switch (found) {
    | Some((file, data)) =>
      respond(~status=200, ~content_type=content_type(file), data)
    | None => not_found()
    };
  };

let serve_api = (store, ~meth, ~path, ~body) => {
  let (status, json) =
    switch (Api.handle(store, ~meth, ~path, ~body)) {
    | response => response
    | exception error =>
      prerr_endline("api error: " ++ Printexc.to_string(error));
      Api.problem(500, "Internal error");
    };
  respond(
    ~status,
    ~content_type="application/json",
    Option.fold(~none="", ~some=Json.to_string, json),
  );
};

let main = () => {
  let port = ref(8080);
  let database = ref("todos.db");
  let static = ref("public");
  Arg.parse(
    [
      ("--port", Arg.Set_int(port), "PORT  listen port (default 8080)"),
      (
        "--db",
        Arg.Set_string(database),
        "FILE  SQLite file (default todos.db)",
      ),
      (
        "--static",
        Arg.Set_string(static),
        "DIR  emitted client directory (default public)",
      ),
    ],
    _ => raise(Arg.Bad("unexpected argument")),
    "todo_server [--port PORT] [--db FILE] [--static DIR]",
  );
  let store = Store.open_(database^);
  Eio_main.run(env =>
    Eio.Switch.run(sw => {
      /* open_dir is a capability: paths resolved under it cannot escape the
         directory, whatever the URL decodes to. */
      let root =
        Eio.Path.open_dir(~sw, Eio.Path.(Eio.Stdenv.fs(env) / static^));
      let callback = (_conn, request, body) => {
        let path =
          Http.Request.resource(request)
          |> Uri.of_string
          |> Uri.path
          |> String.split_on_char('/')
          |> List.filter(segment => segment != "")
          |> List.map(Uri.pct_decode);
        switch (path) {
        | ["api", ..._] =>
          serve_api(
            store,
            ~meth=Http.Request.meth(request),
            ~path,
            /* Without a declared length, reading to EOF would wait forever on
               a keep-alive connection (e.g. a bodiless DELETE). */
            ~body=
              switch (Http.Request.has_body(request)) {
              | `Yes => Eio.Flow.read_all(body)
              | `No
              | `Unknown => ""
              },
          )
        | _ => serve_static(root, path)
        };
      };
      let socket =
        Eio.Net.listen(
          Eio.Stdenv.net(env),
          ~sw,
          ~backlog=128,
          ~reuse_addr=true,
          `Tcp((Eio.Net.Ipaddr.V4.any, port^)),
        );
      Printf.printf(
        "todo server on http://127.0.0.1:%d/ (db %s)\n%!",
        port^,
        database^,
      );
      Cohttp_eio.Server.run(
        socket, Cohttp_eio.Server.make(~callback, ()), ~on_error=error =>
        prerr_endline("http error: " ++ Printexc.to_string(error))
      );
    })
  );
};
