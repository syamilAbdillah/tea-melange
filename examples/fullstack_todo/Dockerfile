# Build context is the repository root (the example builds tea from source):
#   docker build -f examples/fullstack_todo/Dockerfile -t tea-fullstack-todo .
# or, from this directory: docker compose up --build

FROM ocaml/opam:debian-12-ocaml-5.4 AS build
USER root
RUN apt-get update \
 && apt-get install -y --no-install-recommends libsqlite3-dev pkg-config \
 && rm -rf /var/lib/apt/lists/*
USER opam
WORKDIR /home/opam/app

# Dependencies first so source edits reuse this layer.
RUN opam update -q
COPY --chown=opam tea.opam ./
RUN opam install -y --deps-only ./tea.opam \
 && opam install -y reason eio_main cohttp-eio sqlite3 yojson

COPY --chown=opam dune-project ./
COPY --chown=opam src src
COPY --chown=opam examples/fullstack_todo examples/fullstack_todo
RUN opam exec -- dune build --root . \
      @examples/fullstack_todo/shared/runtest \
      @examples/fullstack_todo/client/melange \
      examples/fullstack_todo/server/main.exe \
 && mkdir -p /home/opam/out \
 && cp _build/default/examples/fullstack_todo/server/main.exe /home/opam/out/todo-server \
 && cp -rL _build/default/examples/fullstack_todo/client/js /home/opam/out/public

FROM debian:12-slim
RUN apt-get update \
 && apt-get install -y --no-install-recommends libsqlite3-0 \
 && rm -rf /var/lib/apt/lists/* \
 && mkdir /data
COPY --from=build /home/opam/out /app
# Docker's default seccomp profile blocks io_uring; use Eio's portable backend.
ENV EIO_BACKEND=posix
VOLUME /data
EXPOSE 8080
CMD ["/app/todo-server", "--port", "8080", "--db", "/data/todos.db", "--static", "/app/public"]
