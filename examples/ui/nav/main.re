open Tea;
module N = Ui.Nav;
type model = {
  current: string,
  token: string,
  labelled: bool,
  count: string,
};
type msg =
  | Current(string)
  | Token(string)
  | Labelled(bool)
  | Count(string);
let update = (msg, model) => (
  switch (msg) {
  | Current(current) => {
      ...model,
      current,
    }
  | Token(token) => {
      ...model,
      token,
    }
  | Labelled(labelled) => {
      ...model,
      labelled,
    }
  | Count(count) => {
      ...model,
      count,
    }
  },
  Cmd.none,
);
let option_ = value =>
  Html.option([Html.attribute("value", value)], [Html.text(value)]);
let view = model => {
  let token =
    switch (model.token) {
    | "step" => N.Step
    | "location" => N.Location
    | "date" => N.Date
    | "time" => N.Time
    | "true" => N.True
    | _ => N.Page
    };
  let keys =
    switch (model.count) {
    | "Empty" => [||]
    | "Single" => [|"Overview"|]
    | _ => [|"Overview", "Resources", "Contact"|]
    };
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Know where you are")]),
      Html.p(
        [],
        [
          Html.text(
            "Ordinary links, a named landmark and one current destination. This is navigation, not an application menu.",
          ),
        ],
      ),
      Html.section(
        [],
        [
          Html.h2([], [Html.text("Navigation controls")]),
          Html.label([Html.for_("token")], [Html.text("Current token")]),
          Html.select(
            [
              Html.id("token"),
              Html.value(model.token),
              Html.on_change(value => Token(value)),
            ],
            List.map(
              option_,
              ["page", "step", "location", "date", "time", "true"],
            ),
          ),
          Html.label([Html.for_("count")], [Html.text("Destinations")]),
          Html.select(
            [
              Html.id("count"),
              Html.value(model.count),
              Html.on_change(value => Count(value)),
            ],
            List.map(option_, ["Multiple", "Single", "Empty"]),
          ),
          Html.label(
            [],
            [
              Html.input(
                [
                  Html.id("labelled"),
                  Html.type_("checkbox"),
                  Html.checked(model.labelled),
                  Html.on_check(value => Labelled(value)),
                ],
                [],
              ),
              Html.text(" Use visible landmark label"),
            ],
          ),
        ],
      ),
      N.view(
        ~id="sections",
        ~name=model.labelled ? N.Labelledby : N.Label("Sections"),
        ~keys,
        ~current=model.current,
        ~current_token=token,
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
                                Html.href("#destination"),
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
      ),
      Html.section(
        [Html.id("destination"), Html.tabindex(-1)],
        [
          Html.h2([], [Html.text(model.current)]),
          Html.p(
            [],
            [
              Html.text(
                "The active link updates this destination. Removing it does not silently choose another current link.",
              ),
            ],
          ),
        ],
      ),
    ],
  );
};
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              current: "Overview",
              token: "page",
              labelled: false,
              count: "Multiple",
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=_ => Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
