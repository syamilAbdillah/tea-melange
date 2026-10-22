open Tea;
module P = Ui.Popover;
type msg =
  | Popover(string, P.msg)
  | Open(string)
  | Close(string)
  | Unmount
  | Mount;
type model = {
  popovers: list((string, P.model)),
  activity: string,
  mounted: bool,
};
let names = [
  "filter",
  "plants",
  "tools",
  "animated",
  "clipped",
  "first",
  "element",
  "persistent",
];
let make = name => {
  let group = String.equal(name, "plants") || String.equal(name, "tools");
  let focus_on_open =
    switch (name) {
    | "first" => P.First_tabbable
    | "element" => P.Element("element-target")
    | _ => P.None
    };
  P.init(
    ~id=name,
    ~role=
      String.equal(name, "filter")
        ? P.Dialog("Filter specimens") : group ? P.None : P.Region,
    ~focus_on_open,
    ~close_on_focus_out=!String.equal(name, "persistent"),
    ~group_triggers=group ? ["plants-trigger", "tools-trigger"] : [],
    ~arrow_size=12.,
    (),
  );
};
let initial = () => List.map(name => (name, make(name)), names);
let reason = reason =>
  switch (reason) {
  | P.Escape => "Escape"
  | P.Outside => "Outside"
  | P.Focus_out => "Focus_out"
  | P.Trigger => "Trigger"
  | P.Programmatic => "Programmatic"
  };
let map_cmd = (name, command) =>
  Cmd.map(message => Popover(name, message), command);
let update = (message, model) =>
  switch (message) {
  | Unmount => (
      {
        ...model,
        mounted: false,
      },
      Cmd.none,
    )
  | Mount => (
      {
        ...model,
        mounted: true,
        popovers: initial(),
      },
      Cmd.none,
    )
  | Popover(name, message) =>
    let commands = ref([]);
    let opened = ref(false);
    let activity = ref(model.activity);
    let popovers =
      List.map(
        ((key, popover)) =>
          if (String.equal(name, key)) {
            let (popover, command, outs) = P.update(message, popover);
            commands := [map_cmd(key, command), ...commands^];
            List.iter(
              out =>
                switch (out) {
                | P.Opened =>
                  opened := true;
                  activity := key ++ " Opened";
                | P.Closed(cause) =>
                  activity := key ++ " Closed(" ++ reason(cause) ++ ")"
                },
              outs,
            );
            (key, popover);
          } else {
            (key, popover);
          },
        model.popovers,
      );
    /* Group exclusivity is parent-owned: close peers in the same update,
       before rendering or running any commands. No synthetic user outs. */
    let grouped =
      String.equal(name, "plants") || String.equal(name, "tools");
    let popovers =
      ! opened^ || !grouped
        ? popovers
        : List.map(
            ((key, popover)) =>
              if (!String.equal(name, key)
                  && (
                    String.equal(key, "plants") || String.equal(key, "tools")
                  )) {
                let (popover, command) = P.close(popover);
                commands := [map_cmd(key, command), ...commands^];
                (key, popover);
              } else {
                (key, popover);
              },
            popovers,
          );
    (
      {
        ...model,
        popovers,
        activity: activity^,
      },
      Cmd.batch(commands^),
    );
  | Open(name)
  | Close(name) =>
    let commands = ref([]);
    let popovers =
      List.map(
        ((key, popover)) =>
          if (String.equal(name, key)) {
            let (popover, command) =
              switch (message) {
              | Open(_) => P.open_(popover)
              | _ => P.close(popover)
              };
            commands := [map_cmd(key, command), ...commands^];
            (key, popover);
          } else {
            (key, popover);
          },
        model.popovers,
      );
    (
      {
        ...model,
        popovers,
      },
      Cmd.batch(commands^),
    );
  };
let label = name =>
  switch (name) {
  | "filter" => "Filter specimens"
  | "plants" => "Plants"
  | "tools" => "Tools"
  | "animated" => "Growing notes"
  | "clipped" => "Inside a clipped container"
  | "first" => "Focus the first field"
  | "element" => "Focus a specific field"
  | _ => "Stay open on focus out"
  };
let render = (model, name) => {
  let popover = List.assoc(name, model.popovers);
  P.view(
    ~model=popover,
    ~to_parent=message => Popover(name, message),
    ~to_view=
      parts =>
        Html.div(
          [Html.class_("popover")],
          [
            Html.button(parts.trigger, [Html.text(label(name))]),
            /* Keep mounted to prove hidden and aria-controls removal independently. */
            Html.div(
              parts.panel
              @ [
                Html.class_(
                  String.equal(name, "animated") ? "panel animated" : "panel",
                ),
              ],
              [
                Html.span(
                  [
                    Html.class_("arrow"),
                    Html.aria("hidden", "true"),
                    Html.style(
                      "left",
                      switch (
                        Option.bind(parts.position, position =>
                          position.arrow_offset
                        )
                      ) {
                      | None => "0px"
                      | Some(offset) => string_of_float(offset) ++ "px"
                      },
                    ),
                  ],
                  [],
                ),
                String.equal(name, "filter")
                  ? Html.form(
                      [
                        Html.aria("label", "Specimen filters"),
                        Html.on_submit(Close(name)),
                      ],
                      [
                        Html.label(
                          [Html.for_("filter-search")],
                          [Html.text("Common name")],
                        ),
                        Html.input(
                          [Html.id("filter-search"), Html.type_("search")],
                          [],
                        ),
                        Html.label(
                          [Html.for_("filter-kind")],
                          [Html.text("Growing conditions")],
                        ),
                        Html.select(
                          [Html.id("filter-kind")],
                          [
                            Html.option([], [Html.text("All conditions")]),
                            Html.option([], [Html.text("Shade")]),
                          ],
                        ),
                        Html.label(
                          [],
                          [
                            Html.input(
                              [
                                Html.id("filter-stock"),
                                Html.type_("checkbox"),
                              ],
                              [],
                            ),
                            Html.text(" In stock only"),
                          ],
                        ),
                      ],
                    )
                  : (
                    if (String.equal(name, "plants")
                        || String.equal(name, "tools")) {
                      Html.nav(
                        [Html.aria("label", label(name) ++ " links")],
                        [
                          Html.a(
                            [
                              Html.id(name ++ "-link"),
                              Html.href("#collection"),
                            ],
                            [
                              Html.text(
                                String.equal(name, "plants")
                                  ? "Ferns" : "Hand tools",
                              ),
                            ],
                          ),
                          Html.a(
                            [Html.href("#collection")],
                            [Html.text("Browse the collection")],
                          ),
                        ],
                      );
                    } else {
                      Html.div(
                        [],
                        [
                          Html.p(
                            [],
                            [
                              Html.text(
                                "Notes stay in the document's natural reading and Tab order.",
                              ),
                            ],
                          ),
                          Html.label(
                            [Html.for_(name ++ "-target")],
                            [Html.text("Field notes")],
                          ),
                          Html.input([Html.id(name ++ "-target")], []),
                        ],
                      );
                    }
                  ),
                Html.button(parts.close_button, [Html.text("Done")]),
              ],
            ),
          ],
        ),
    (),
  );
};
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("The specimen notebook")]),
      Html.p(
        [],
        [
          Html.text(
            "Non-modal popovers: explore a collection without leaving the page. Tab moves through each open panel; Escape returns to its button.",
          ),
        ],
      ),
      model.mounted
        ? Html.div(
            [],
            [
              Html.section(
                [Html.aria("label", "Filters")],
                [
                  render(model, "filter"),
                  Ui.Button.button(
                    ~id="after-filter",
                    ~on_press=Close("filter"),
                    [Html.text("Continue browsing")],
                  ),
                ],
              ),
              Html.section(
                [Html.aria("label", "Collections")],
                [
                  Html.h2([], [Html.text("Browse collections")]),
                  Html.div(
                    [
                      Html.role("group"),
                      Html.aria("label", "Collection navigation"),
                      Html.class_("group"),
                    ],
                    [render(model, "plants"), render(model, "tools")],
                  ),
                ],
              ),
              Html.section(
                [Html.aria("label", "Motion")],
                [render(model, "animated")],
              ),
              Html.section(
                [
                  Html.aria("label", "Top-layer escape"),
                  Html.class_("clip"),
                ],
                [render(model, "clipped")],
              ),
              Html.section(
                [Html.aria("label", "Focus policies")],
                [
                  render(model, "first"),
                  render(model, "element"),
                  render(model, "persistent"),
                ],
              ),
            ],
          )
        : Html.p([], [Html.text("Examples unmounted.")]),
      Html.div(
        [Html.class_("controls")],
        [
          Ui.Button.button(
            ~id="open-filter",
            ~on_press=Open("filter"),
            [Html.text("Open filters programmatically")],
          ),
          Ui.Button.button(
            ~id="close-filter",
            ~on_press=Close("filter"),
            [Html.text("Close filters programmatically")],
          ),
          Ui.Button.button(
            ~id="unmount",
            ~on_press=Unmount,
            [Html.text("Unmount examples")],
          ),
          Ui.Button.button(
            ~id="mount",
            ~on_press=Mount,
            [Html.text("Reset examples")],
          ),
        ],
      ),
      Html.p(
        [Html.id("collection")],
        [
          Html.text(
            "Collection: native form controls, ordinary navigation links. No menu roles and no focus trap.",
          ),
        ],
      ),
      Html.node(
        "output",
        [Html.id("activity"), Html.aria("live", "polite")],
        [Html.text(model.activity)],
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
              popovers: initial(),
              activity: "Ready",
              mounted: true,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          !model.mounted
            ? Sub.none
            : Sub.batch(
                List.map(
                  ((name, popover)) =>
                    Sub.map(
                      message => Popover(name, message),
                      P.subscriptions(popover),
                    ),
                  model.popovers,
                ),
              ),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
