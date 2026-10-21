open Tea;
open Test_support;
module N = Ui.Nav;
type model = {
  keys: array(string),
  current: option(string),
  token: N.current_token,
  labelled: bool,
};
type msg =
  | Keys(array(string))
  | Current(string)
  | Token(N.current_token)
  | Labelled;
let update = (msg, model) => (
  switch (msg) {
  | Keys(keys) => {
      ...model,
      keys,
    }
  | Current(current) => {
      ...model,
      current: Some(current),
    }
  | Token(token) => {
      ...model,
      token,
    }
  | Labelled => {
      ...model,
      labelled: !model.labelled,
    }
  },
  Cmd.none,
);
let view = model => {
  let current = model.current;
  N.view(
    ~id="nav",
    ~name=model.labelled ? N.Labelledby : N.Label("Sections"),
    ~keys=model.keys,
    ~current?,
    ~current_token=model.token,
    ~to_view=
      p =>
        Html.div(
          [],
          [
            Html.h2(p.label, [Html.text("Sections")]),
            Html.ul(
              p.list,
              Array.to_list(
                Array.map(
                  (item: N.item(msg)) =>
                    Html.li(
                      [],
                      [
                        Html.a(
                          item.link
                          @ [
                            Html.href("#" ++ item.key),
                            Html.on_click(Current(item.key)),
                          ],
                          [Html.text(item.key)],
                        ),
                      ],
                    ),
                  p.items,
                ),
              ),
            ),
          ],
        ),
    (),
  );
};
let rejected = f =>
  try(
    {
      f();
      false;
    }
  ) {
  | Invalid_argument(_) => true
  };
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              keys: [|"one", "two"|],
              current: Some("one"),
              token: N.Page,
              labelled: false,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=_ => Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  check("native nav", get(node("#nav"), "tagName") == "NAV");
  check(
    "label",
    attr("#nav", "aria-label") == Some("Sections")
    && attr("#nav", "aria-labelledby") == None,
  );
  check(
    "list structure",
    Array.length(query_all(document, "#nav > div > ul > li > a")) == 2,
  );
  check(
    "only one current",
    Array.length(query_all(document, "[aria-current]")) == 1
    && attr("#nav-item-one", "data-current") == Some("")
    && attr("#nav-item-two", "data-current") == None,
  );
  click("#nav-item-two");
  check(
    "current changes",
    attr("#nav-item-one", "aria-current") == None
    && attr("#nav-item-one", "data-current") == None
    && attr("#nav-item-two", "aria-current") == Some("page"),
  );
  List.iter(
    ((token, value)) => {
      app.dispatch(Token(token));
      frame();
      check(
        "current token",
        attr("#nav-item-two", "aria-current") == Some(value),
      );
    },
    [
      (N.Page, "page"),
      (N.Step, "step"),
      (N.Location, "location"),
      (N.Date, "date"),
      (N.Time, "time"),
      (N.True, "true"),
    ],
  );
  app.dispatch(Labelled);
  frame();
  check(
    "labelledby resolves",
    attr("#nav", "aria-label") == None
    && attr("#nav", "aria-labelledby") == Some("nav-label")
    && get(node("#nav-label"), "textContent") == "Sections",
  );
  focus(node("#nav-item-two"));
  List.iter(
    key_ =>
      check(
        "native keyboard unchanged",
        key("#nav-item-two", "keydown", key_),
      ),
    ["Tab", "Enter", " ", "ArrowDown", "Home", "End"],
  );
  check("no focus effects", has_focus("#nav-item-two"));
  app.dispatch(Current("one"));
  app.dispatch(Current("two"));
  frame();
  check(
    "batched current uses latest",
    attr("#nav-item-two", "aria-current") == Some("true")
    && Array.length(query_all(document, "[aria-current]")) == 1,
  );
  app.dispatch(Keys([|"one"|]));
  frame();
  check(
    "removed current not reassigned",
    Array.length(query_all(document, "[aria-current]")) == 0,
  );
  app.dispatch(Keys([||]));
  frame();
  check("empty", Array.length(query_all(document, "#nav a")) == 0);
  app.dispatch(Keys([|"two"|]));
  frame();
  check(
    "single current reappears",
    Array.length(query_all(document, "[aria-current]")) == 1,
  );
  app.dispatch(Keys([|"a b", "a_0020b", "", "é", "a/b"|]));
  frame();
  let links = query_all(document, "#nav a");
  let ids = Array.map(link => get(link, "id"), links);
  check(
    "safe unique ids",
    Array.length(ids) == 5
    && List.length(List.sort_uniq(String.compare, Array.to_list(ids))) == 5,
  );
  Array.iter(
    id =>
      check(
        "encoded id alphabet",
        Js.Re.test(~str=id, Js.Re.fromString("^[A-Za-z0-9_-]+$")),
      ),
    ids,
  );
  check(
    "duplicates rejected",
    rejected(() =>
      ignore(
        N.parts(
          ~id="pure",
          ~name=N.Label("Nav"),
          ~keys=[|"x", "x"|],
          ~current="x",
          (),
        ),
      )
    ),
  );
  check(
    "empty label rejected",
    rejected(() =>
      ignore(N.parts(~id="pure", ~name=N.Label(" \t"), ~keys=[||], ()))
    ),
  );
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no globals", listener_count() == 0);
  report("nav");
  Js.Promise.resolve();
};
