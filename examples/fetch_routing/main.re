open Tea;

/* The server's SPA fallback makes these paths safe to reload directly. */
type route =
  | Home
  | Users
  | User(int)
  | NotFound;
type user = {
  id: int,
  name: string,
};
type remote =
  | Loading
  | Loaded(list(user))
  | Failed;
type model = {
  route,
  users: remote,
};
type msg =
  | UrlRequested(Navigation.url_request)
  | UrlChanged(Url.t)
  | Received(result(list(user), Http.error))
  | Retry;

let route = url => {
  Url.Parser.(
    switch (
      parse(
        one_of([
          map(_ => Home, top),
          map(_ => Users, s("users")),
          map(((_, id)) => User(id), slash(s("users"), int)),
        ]),
        url,
      )
    ) {
    | Some(route) => route
    | None => NotFound
    }
  );
};
let decoder =
  Decode.list(
    Decode.map2(
      (id, name) =>
        {
          id,
          name,
        },
      Decode.field("id", Decode.int),
      Decode.field("name", Decode.string),
    ),
  );
let fetch_users = () =>
  Http.get(
    ~url="/users.json",
    ~expect=Http.expect_json(result => Received(result), decoder),
  );
let init = ((), url) => (
  {
    route: route(url),
    users: Loading,
  },
  fetch_users(),
);
let update = (model, msg) =>
  switch (msg) {
  | UrlRequested(Navigation.Internal(url)) => (
      model,
      Navigation.push_url(Url.to_string(url)),
    )
  | UrlRequested(Navigation.External(url)) => (model, Navigation.load(url))
  | UrlChanged(url) => (
      {
        ...model,
        route: route(url),
      },
      Cmd.none,
    )
  | Received(Ok(users)) => (
      {
        ...model,
        users: Loaded(users),
      },
      Cmd.none,
    )
  | Received(Error(_)) => (
      {
        ...model,
        users: Failed,
      },
      Cmd.none,
    )
  | Retry => (
      {
        ...model,
        users: Loading,
      },
      fetch_users(),
    )
  };
let link = (path, label) =>
  Html.a([Html.href(path)], [Html.text(label)]);
let user_content = (users, selected) =>
  switch (users) {
  | Loading => Html.p([Html.id("loading")], [Html.text("Loading users…")])
  | Failed =>
    Html.div(
      [Html.id("error")],
      [
        Html.text("Could not load users. "),
        Html.button([Html.on_click(Retry)], [Html.text("Retry")]),
      ],
    )
  | Loaded(users) =>
    switch (selected) {
    | None =>
      Html.ul(
        [Html.id("users")],
        List.map(
          user =>
            Html.li(
              [],
              [link("/users/" ++ string_of_int(user.id), user.name)],
            ),
          users,
        ),
      )
    | Some(id) =>
      switch (List.find_opt(user => user.id == id, users)) {
      | Some(user) => Html.h2([Html.id("user")], [Html.text(user.name)])
      | None =>
        Html.p([Html.id("not-found")], [Html.text("User not found")])
      }
    }
  };
let view = model => {
  let (title, content) =
    switch (model.route) {
    | Home => (
        "Home",
        Html.p(
          [Html.id("home")],
          [Html.text("Welcome. Browse the users loaded from JSON.")],
        ),
      )
    | Users => ("Users", user_content(model.users, None))
    | User(id) => (
        "User " ++ string_of_int(id),
        user_content(model.users, Some(id)),
      )
    | NotFound => (
        "Not found",
        Html.h2([Html.id("not-found")], [Html.text("Page not found")]),
      )
    };
  {
    App.title: title ++ " · Tea",
    body: [
      Html.main(
        [],
        [
          Html.h1([], [Html.text("People / places")]),
          Html.nav(
            [],
            [
              link("/", "Home"),
              link("/users", "Users"),
              link("/missing", "Unknown route"),
            ],
          ),
          content,
        ],
      ),
    ],
  };
};
let app =
  App.application(
    ~init,
    ~update,
    ~view,
    ~subscriptions=_ => Sub.none,
    ~on_url_request=request => UrlRequested(request),
    ~on_url_change=url => UrlChanged(url),
    ~flags=(),
    (),
  );
