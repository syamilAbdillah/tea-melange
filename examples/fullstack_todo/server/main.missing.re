let () = {
  prerr_endline(
    "todo server was built without its dependencies. Install them with\n"
    ++ "  opam install eio_main cohttp-eio sqlite3 yojson\n"
    ++ "or run the example with docker compose (see README.md).",
  );
  exit(1);
};
