open Tea;
module R = Ui.Radio_group;
let items: array(R.item) = [|
  {
    key: "daily",
    label: "Daily",
    disabled: false,
  },
  {
    key: "instant",
    label: "Instant (unavailable)",
    disabled: true,
  },
  {
    key: "weekly",
    label: "Weekly",
    disabled: false,
  },
  {
    key: "never",
    label: "Never",
    disabled: false,
  },
|];
type model = {
  groups: array(R.model),
  items: array(R.item),
  invalid: bool,
  changes: int,
};
type msg =
  | Group(int, R.msg)
  | Clear
  | Remove
  | Restore
  | Empty
  | Invalid(bool);
let update = (msg, model) =>
  switch (msg) {
  | Group(i, msg) =>
    let (group, cmd, out) =
      R.update(~items=i == 0 ? model.items : items, msg, model.groups[i]);
    let groups = Array.copy(model.groups);
    groups[i] = group;
    (
      {
        ...model,
        groups,
        changes: model.changes + List.length(out),
      },
      Cmd.map(msg => Group(i, msg), cmd),
    );
  | Invalid(invalid) => (
      {
        ...model,
        invalid,
      },
      Cmd.none,
    )
  | Clear
  | Remove
  | Restore
  | Empty =>
    let next_items =
      switch (msg) {
      | Remove =>
        Array.of_list(
          List.filter(
            (item: R.item) => Some(item.key) != R.checked(model.groups[0]),
            Array.to_list(model.items),
          ),
        )
      | Restore => items
      | Empty => [||]
      | _ => model.items
      };
    let (group, cmd) =
      switch (msg) {
      | Clear => R.set_checked(~items=next_items, None, model.groups[0])
      | _ => R.reconcile(~items=next_items, model.groups[0])
      };
    let groups = Array.copy(model.groups);
    groups[0] = group;
    (
      {
        ...model,
        groups,
        items: next_items,
      },
      Cmd.map(msg => Group(0, msg), cmd),
    );
  };
let render = (model, i, title) =>
  R.view(
    ~model=model.groups[i],
    ~items=i == 0 ? model.items : items,
    ~to_parent=msg => Group(i, msg),
    ~description=i == 0,
    ~required=i == 0,
    ~invalid=i == 0 && model.invalid,
    ~name="schedule-" ++ string_of_int(i),
    ~to_view=
      parts =>
        Html.section(
          [],
          [
            Html.h2(parts.label, [Html.text(title)]),
            i == 0
              ? Html.p(
                  parts.description,
                  [
                    Html.text(
                      "Choose when your project digest arrives. Instant delivery is unavailable.",
                    ),
                  ],
                )
              : Html.text(""),
            Html.keyed(
              "div",
              parts.root,
              Array.to_list(
                Array.map(
                  (part: R.item_parts(msg)) =>
                    (
                      part.item.key,
                      Html.div(part.root, [Html.text(part.item.label)]),
                    ),
                  parts.items,
                ),
              ),
            ),
            Option.value(~default=Html.text(""), parts.hidden_input),
          ],
        ),
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Delivery preferences")]),
      Html.p(
        [],
        [
          Html.text(
            "Use the arrow keys to compare schedules. Tab continues to the next setting.",
          ),
        ],
      ),
      Html.form(
        [Html.aria("label", "Delivery settings")],
        [
          render(model, 0, "Delivery schedule"),
          Html.label(
            [],
            [
              Html.input(
                [
                  Html.id("invalid"),
                  Html.type_("checkbox"),
                  Html.checked(model.invalid),
                  Html.on_check(value => Invalid(value)),
                ],
                [],
              ),
              Html.text(" Mark schedule invalid"),
            ],
          ),
          Html.div(
            [],
            [
              Html.button(
                [
                  Html.id("clear"),
                  Html.type_("button"),
                  Html.on_click(Clear),
                ],
                [Html.text("Clear schedule")],
              ),
              Html.button(
                [
                  Html.id("remove"),
                  Html.type_("button"),
                  Html.on_click(Remove),
                ],
                [Html.text("Remove selected schedule")],
              ),
              Html.button(
                [
                  Html.id("restore"),
                  Html.type_("button"),
                  Html.on_click(Restore),
                ],
                [Html.text("Restore schedules")],
              ),
              Html.button(
                [
                  Html.id("empty"),
                  Html.type_("button"),
                  Html.on_click(Empty),
                ],
                [Html.text("Clear available options")],
              ),
            ],
          ),
          render(model, 1, "Managed schedule (read-only)"),
          render(model, 2, "Right-to-left schedule"),
          render(model, 3, "Paused schedule (disabled)"),
        ],
      ),
      Html.node(
        "output",
        [Html.id("activity"), Html.aria("live", "polite")],
        [Html.text("Changes: " ++ string_of_int(model.changes))],
      ),
      Html.button(
        [Html.id("after"), Html.type_("button")],
        [Html.text("Continue")],
      ),
    ],
  );
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              groups: [|
                R.init(~id="schedule", ~items, ~orientation=R.Horizontal, ()),
                R.init(
                  ~id="managed",
                  ~items,
                  ~checked="weekly",
                  ~readonly=true,
                  (),
                ),
                R.init(
                  ~id="rtl",
                  ~items,
                  ~checked="daily",
                  ~dir=R.Rtl,
                  ~orientation=R.Horizontal,
                  (),
                ),
                R.init(
                  ~id="paused",
                  ~items,
                  ~checked="daily",
                  ~disabled=true,
                  (),
                ),
              |],
              items,
              invalid: true,
              changes: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch(
            Array.to_list(
              Array.mapi(
                (i, group) =>
                  Sub.map(msg => Group(i, msg), R.subscriptions(group)),
                model.groups,
              ),
            ),
          ),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
