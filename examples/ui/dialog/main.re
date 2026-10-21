open Tea;
module D = Ui.Dialog;
type entry = {
  key: string,
  dialog: D.model,
};
type msg =
  | Open(string)
  | Close(string)
  | Dialog(string, D.msg)
  | Help(string)
  | Confirm
  | Remove_opener
  | Guard(bool);
type model = {
  dialogs: list(entry),
  log: string,
  deleted: int,
  opener: bool,
  guard: bool,
};
let keys = [
  "basic",
  "alert",
  "alert-backdrop",
  "nested",
  "long",
  "animated",
  "empty",
  "missing",
  "label",
];
let create = key => {
  let dialog =
    switch (key) {
    | "alert" =>
      D.init(
        ~id=key,
        ~kind=D.Alertdialog("alert-cancel"),
        ~dismissible=false,
        (),
      )
    | "alert-backdrop" =>
      D.init(
        ~id=key,
        ~kind=D.Alertdialog("alert-backdrop-close"),
        ~close_on_backdrop=true,
        (),
      )
    | "long" => D.init(~id=key, ~initial_focus="long-title", ())
    | "missing" =>
      D.init(
        ~id=key,
        ~initial_focus="not-rendered",
        ~close_on_backdrop=false,
        (),
      )
    | _ => D.init(~id=key, ())
    };
  {
    key,
    dialog,
  };
};
let reason = reason =>
  switch (reason) {
  | D.Escape => "Escape"
  | D.Backdrop => "Backdrop"
  | D.Close_button => "Close button"
  | D.Native => "Native"
  };
let update = (message, model) =>
  switch (message) {
  | Help(key) => (
      {
        ...model,
        log: key ++ " keyboard help: Tab stays inside; Escape requests close",
      },
      Cmd.none,
    )
  | Guard(guard) => (
      {
        ...model,
        guard,
      },
      Cmd.none,
    )
  | Remove_opener => (
      {
        ...model,
        opener: false,
      },
      Cmd.none,
    )
  | Confirm =>
    let commands = ref([]);
    let dialogs =
      List.map(
        entry =>
          if (String.equal(entry.key, "alert")) {
            let (dialog, command) = D.close(entry.dialog);
            commands :=
              [Cmd.map(message => Dialog(entry.key, message), command)];
            {
              ...entry,
              dialog,
            };
          } else {
            entry;
          },
        model.dialogs,
      );
    (
      {
        ...model,
        dialogs,
        deleted: model.deleted + 1,
      },
      Cmd.batch(commands^),
    );
  | Open(key)
  | Close(key)
  | Dialog(key, _) =>
    let commands = ref([]);
    let log = ref(model.log);
    let dialogs =
      List.map(
        entry =>
          if (String.equal(entry.key, key)) {
            let (dialog, command, outs) =
              switch (message) {
              | Open(_) =>
                let (dialog, command) = D.open_(entry.dialog);
                (dialog, command, []);
              | Close(_) =>
                let (dialog, command) = D.close(entry.dialog);
                (dialog, command, []);
              | Dialog(_, message) => D.update(message, entry.dialog)
              | _ => (entry.dialog, Cmd.none, [])
              };
            let dialog =
              List.fold_left(
                (dialog, out) =>
                  switch (out) {
                  | D.Opened =>
                    log := key ++ " opened";
                    dialog;
                  | D.Closed =>
                    log := key ++ " closed; value=" ++ D.return_value(dialog);
                    dialog;
                  | D.Close_requested(reason_) =>
                    log := key ++ " requested " ++ reason(reason_);
                    if (String.equal(key, "alert")
                        && !model.guard
                        && reason_ == D.Close_button) {
                      let (dialog, command) = D.close(dialog);
                      commands :=
                        [
                          Cmd.map(message => Dialog(key, message), command),
                          ...commands^,
                        ];
                      dialog;
                    } else {
                      dialog;
                    };
                  },
                dialog,
                outs,
              );
            commands :=
              [
                Cmd.map(message => Dialog(key, message), command),
                ...commands^,
              ];
            {
              ...entry,
              dialog,
            };
          } else {
            entry;
          },
        model.dialogs,
      );
    (
      {
        ...model,
        dialogs,
        log: log^,
      },
      Cmd.batch(commands^),
    );
  };
let button = (id, label, message) =>
  Ui.Button.button(~id, ~on_press=message, [Html.text(label)]);
let rec panel = (model, key) => {
  let entry =
    List.find(entry => String.equal(entry.key, key), model.dialogs);
  let title =
    switch (key) {
    | "basic" => "Edit collection"
    | "alert" => "Delete collection?"
    | "alert-backdrop" => "Optional backdrop dismissal"
    | "nested" => "Choose a collection"
    | "long" => "Collection terms"
    | "animated" => "A measured entrance"
    | "empty" => "No actions"
    | "missing" => "Focus fallback"
    | _ => "Label without a title"
    };
  D.view(
    ~model=entry.dialog,
    ~name=String.equal(key, "label") ? D.Label(title) : D.Title,
    ~has_description=String.equal(key, "alert"),
    ~labels={ D.close: String.equal(key, "alert") ? "Cancel" : "Close" },
    ~on_key_down=
      pressed => String.equal(pressed, "F2") ? Some(Help(key)) : None,
    ~to_parent=message => Dialog(key, message),
    ~to_view=
      parts =>
        Html.node(
          "dialog",
          parts.root
          @ [Html.class_(String.equal(key, "animated") ? "animated" : "")],
          [
            Html.div(
              [Html.class_("content")],
              [
                String.equal(key, "label")
                  ? Html.p([], [Html.text(title)])
                  : Html.h2(
                      parts.title
                      @ (
                        String.equal(key, "long") ? [Html.tabindex(-1)] : []
                      ),
                      [Html.text(title)],
                    ),
                switch (key) {
                | "basic" =>
                  Html.div(
                    [],
                    [
                      Html.form(
                        [Html.attribute("method", "dialog")],
                        [
                          Html.label(
                            [Html.for_("collection-name")],
                            [Html.text("Collection name")],
                          ),
                          Html.input(
                            [
                              Html.id("collection-name"),
                              Html.value("Field notes"),
                            ],
                            [],
                          ),
                          Html.button(
                            [
                              Html.id("save"),
                              Html.type_("submit"),
                              Html.attribute("value", "saved"),
                            ],
                            [Html.text("Save")],
                          ),
                        ],
                      ),
                      button(
                        "open-nested",
                        "Choose collection",
                        Open("nested"),
                      ),
                      button("remove-opener", "Remove opener", Remove_opener),
                      panel(model, "nested"),
                    ],
                  )
                | "alert" =>
                  Html.div(
                    [],
                    [
                      Html.p(
                        parts.description,
                        [
                          Html.text(
                            "This permanently deletes the collection. Cancel keeps it safe.",
                          ),
                        ],
                      ),
                      button("confirm", "Delete collection", Confirm),
                      Html.button(
                        parts.close_button @ [Html.id("alert-cancel")],
                        [Html.text("Cancel")],
                      ),
                    ],
                  )
                | "alert-backdrop" =>
                  Html.p(
                    [],
                    [
                      Html.text(
                        "This informational alert explicitly opts into backdrop dismissal. Close is its only action.",
                      ),
                    ],
                  )
                | "missing" =>
                  Html.p(
                    [],
                    [
                      Html.text(
                        "The requested initial target is absent, so focus falls back to Close. Backdrop dismissal is explicitly disabled.",
                      ),
                    ],
                  )
                | "long" =>
                  Html.div(
                    [],
                    List.init(25, index =>
                      Html.p(
                        [],
                        [
                          Html.text(
                            "Term "
                            ++ string_of_int(index + 1)
                            ++ ": Keep a local copy of your collection before making changes.",
                          ),
                        ],
                      )
                    ),
                  )
                | "animated" =>
                  Html.div(
                    [],
                    [
                      Html.p(
                        [],
                        [
                          Html.text(
                            "The dialog and its backdrop follow the same enter and leave flags.",
                          ),
                        ],
                      ),
                      button("leave", "Start leave", Close(key)),
                      button("reverse", "Reopen", Open(key)),
                    ],
                  )
                | "empty" =>
                  Html.p(
                    [],
                    [
                      Html.text(
                        "Escape or the backdrop dismisses this information-only dialog.",
                      ),
                    ],
                  )
                | _ =>
                  Html.p(
                    [],
                    [
                      Html.text("Tab stays within this native modal dialog."),
                    ],
                  )
                },
                String.equal(key, "alert") || String.equal(key, "empty")
                  ? Html.text("")
                  : Html.button(parts.close_button, [Html.text("Close")]),
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
      Html.h1([], [Html.text("A place to pause.")]),
      Html.p(
        [],
        [
          Html.text(
            "Native modal dialogs. Try Tab, Shift+Tab and Escape; F2 uses the composed keyboard hook. The page behind each dialog stays still.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("controls")],
        List.filter_map(
          key =>
            String.equal(key, "nested")
            || String.equal(key, "basic")
            && !model.opener
              ? None
              : Some(button("open-" ++ key, "Open " ++ key, Open(key))),
          keys,
        ),
      ),
      Html.label(
        [],
        [
          Html.input(
            [
              Html.id("guard"),
              Html.type_("checkbox"),
              Html.checked(model.guard),
              Html.on_check(value => Guard(value)),
            ],
            [],
          ),
          Html.text(" Refuse even Cancel in the alertdialog (parent guard)"),
        ],
      ),
      Html.node(
        "output",
        [Html.id("activity"), Html.aria("live", "polite")],
        [
          Html.text(
            model.log ++ "; deleted=" ++ string_of_int(model.deleted),
          ),
        ],
      ),
      Html.div(
        [],
        List.filter_map(
          key =>
            String.equal(key, "nested") ? None : Some(panel(model, key)),
          keys,
        ),
      ),
      Html.div(
        [Html.class_("page-length")],
        [
          Html.p(
            [],
            [
              Html.text(
                "Long page for scroll-lock and scrollbar-compensation checks.",
              ),
            ],
          ),
          button("page-end", "Back to collection", Open("basic")),
        ],
      ),
    ],
  );
let subscriptions = model =>
  Sub.batch(
    List.map(
      entry =>
        Sub.map(
          message => Dialog(entry.key, message),
          D.subscriptions(entry.dialog),
        ),
      model.dialogs,
    ),
  );
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              dialogs: List.map(create, keys),
              log: "Ready",
              deleted: 0,
              opener: true,
              guard: false,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions,
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
