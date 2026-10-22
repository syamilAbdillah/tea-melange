open Tea;
module T = Ui.Toast;
type content = {
  key: string,
  title: string,
  description: string,
  action: bool,
};
type msg =
  | Toast(T.msg)
  | Secondary(T.msg)
  | Push(string)
  | Burst
  | Clear
  | Dismiss_info
  | Alternate
  | Undo;
type model = {
  toast: T.model,
  secondary: T.model,
  contents: list(content),
  serial: int,
  undos: int,
  events: list(string),
};
let reason_name = reason =>
  switch (reason) {
  | T.Timeout => "Timeout"
  | T.Close_button => "Close_button"
  | T.Action => "Action"
  | T.Escape => "Escape"
  | T.Programmatic => "Programmatic"
  };
let outputs = (model, outs) =>
  List.fold_left(
    (model, out) =>
      switch (out) {
      | T.Action(key) => {
          ...model,
          undos: model.undos + 1,
          events: model.events @ ["Action " ++ key],
        }
      | T.Dismissed(key, reason) => {
          ...model,
          events:
            model.events
            @ ["Dismissed " ++ key ++ " " ++ reason_name(reason)],
        }
      },
    model,
    outs,
  );
let push = (model, kind) => {
  let (key, title, description, urgency, duration, action) =
    switch (kind) {
    | "success" => (
        "success",
        "Changes saved",
        "Your draft is available in the library.",
        T.Normal,
        Some(5000),
        false,
      )
    | "error" => (
        "error",
        "Connection lost",
        "Your edits are safe. Reconnect before closing this page.",
        T.Urgent,
        None,
        false,
      )
    | "action" => (
        "action",
        "Item archived",
        "You can restore it from the archive at any time.",
        T.Normal,
        Some(5000),
        true,
      )
    | "persistent" => (
        "persistent",
        "Export ready",
        "Download it from the library whenever you are ready.",
        T.Normal,
        None,
        false,
      )
    | "update" => (
        "info",
        "Information updated",
        "This replaces the information toast in place.",
        T.Urgent,
        Some(5000),
        false,
      )
    | _ => (
        "info",
        "Background sync complete",
        "All files are up to date.",
        T.Normal,
        Some(5000),
        false,
      )
    };
  let serial = model.serial + 1;
  let content_key = "content-" ++ string_of_int(serial);
  let (toast, command) =
    T.push(
      model.toast,
      ~key,
      ~duration,
      ~urgency,
      ~has_actions=action,
      content_key,
    );
  (
    {
      ...model,
      toast,
      serial,
      contents: [
        {
          key: content_key,
          title,
          description,
          action,
        },
        ...model.contents,
      ],
    },
    Cmd.map(message => Toast(message), command),
  );
};
let update = (message, model) =>
  switch (message) {
  | Toast(message) =>
    let (toast, command, outs) = T.update(message, model.toast);
    (
      outputs(
        {
          ...model,
          toast,
        },
        outs,
      ),
      Cmd.map(message => Toast(message), command),
    );
  | Secondary(message) =>
    let (secondary, command, outs) = T.update(message, model.secondary);
    (
      outputs(
        {
          ...model,
          secondary,
        },
        outs,
      ),
      Cmd.map(message => Secondary(message), command),
    );
  | Push(kind) => push(model, kind)
  | Clear =>
    let (toast, command) = T.dismiss_all(model.toast);
    (
      {
        ...model,
        toast,
      },
      Cmd.map(message => Toast(message), command),
    );
  | Dismiss_info =>
    let (toast, command) = T.dismiss(model.toast, "info");
    (
      {
        ...model,
        toast,
      },
      Cmd.map(message => Toast(message), command),
    );
  | Undo => (
      {
        ...model,
        undos: model.undos + 1,
      },
      Cmd.none,
    )
  | Alternate =>
    let serial = model.serial + 1;
    let key = "alternate-" ++ string_of_int(serial);
    let (secondary, command) =
      T.push(model.secondary, ~key, ~duration=None, ~urgency=T.Normal, key);
    (
      {
        ...model,
        secondary,
        serial,
        contents: [
          {
            key,
            title: "Alternative " ++ string_of_int(serial),
            description: "Newest first; two visible at a time.",
            action: false,
          },
          ...model.contents,
        ],
      },
      Cmd.map(message => Secondary(message), command),
    );
  | Burst =>
    let (model, commands) =
      List.fold_left(
        ((model, commands), index) => {
          let serial = model.serial + 1;
          let key = "burst-" ++ string_of_int(serial);
          let (toast, command) =
            T.push(model.toast, ~key, ~urgency=T.Normal, key);
          (
            {
              ...model,
              toast,
              serial,
              contents: [
                {
                  key,
                  title: "Queued notification " ++ string_of_int(index),
                  description: "Waiting entries do not start their timers until they enter.",
                  action: false,
                },
                ...model.contents,
              ],
            },
            [Cmd.map(message => Toast(message), command), ...commands],
          );
        },
        (model, []),
        [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
      );
    (model, Cmd.batch(List.rev(commands)));
  };
let render = (model, toast, to_parent) =>
  T.view(
    ~model=toast,
    ~to_parent,
    ~to_view=
      parts => {
        let lane = urgency =>
          List.filter_map(
            (p: T.toast_parts(msg)) =>
              p.urgency != urgency
                ? None
                : {
                  let content =
                    Option.value(
                      ~default={
                        key: p.content_key,
                        title: "Notification",
                        description: "",
                        action: false,
                      },
                      List.find_opt(
                        (c: content) => String.equal(c.key, p.content_key),
                        model.contents,
                      ),
                    );
                  Some((
                    p.key,
                    Html.div(
                      p.root
                      @ [Html.class_("toast"), Html.data("key", p.key)],
                      [
                        Html.h3(p.title, [Html.text(content.title)]),
                        Html.p(
                          p.description,
                          [Html.text(content.description)],
                        ),
                        Html.div(
                          [Html.class_("toast-actions")],
                          [
                            content.action
                              ? Html.button(
                                  p.action_attrs(
                                    ~key="undo",
                                    ~alt_text=
                                      "Restore this item using Undo archive below, or visit the archive.",
                                    (),
                                  ),
                                  [Html.text("Undo")],
                                )
                              : Html.text(""),
                            Html.button(
                              p.close_button,
                              [Html.text("Dismiss")],
                            ),
                          ],
                        ),
                      ],
                    ),
                  ));
                },
            parts.toasts,
          );
        Html.div(
          parts.region @ [Html.class_("notifications")],
          [
            Html.Keyed.div(parts.polite, lane(T.Normal)),
            Html.Keyed.div(parts.assertive, lane(T.Urgent)),
            Html.p(
              [
                Html.class_("queue"),
                Html.data("queue", string_of_int(parts.queued)),
              ],
              [
                Html.text(
                  string_of_int(parts.queued)
                  ++ " waiting"
                  ++ (parts.paused ? " · paused" : ""),
                ),
              ],
            ),
          ],
        );
      },
    (),
  );
let button = (id, message, label) =>
  Ui.Button.button(~id, ~on_press=message, [Html.text(label)]);
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("A little less interruption.")]),
      Html.p(
        [Html.class_("intro")],
        [
          Html.text(
            "Notifications wait their turn. Hover or focus the inbox to pause every clock. Press F8 to visit, Escape to dismiss and return.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("workbench")],
        [
          Html.section(
            [Html.aria("labelledby", "send-title")],
            [
              Html.h2(
                [Html.id("send-title")],
                [Html.text("Send a notification")],
              ),
              Html.div(
                [Html.class_("controls")],
                [
                  button("info", Push("info"), "Information"),
                  button("success", Push("success"), "Success"),
                  button("error", Push("error"), "Urgent error"),
                  button("action", Push("action"), "With action"),
                  button("persistent", Push("persistent"), "Persistent"),
                  button("burst", Burst, "Queue ten"),
                ],
              ),
              Html.p(
                [],
                [
                  Html.text(
                    "Urgency interrupts speech; use it only for time-critical problems. Actions never expire and always have another route.",
                  ),
                ],
              ),
              button("undo", Undo, "Undo archive"),
              Html.p(
                [Html.id("undos")],
                [Html.text("Restored: " ++ string_of_int(model.undos))],
              ),
              Html.div(
                [Html.class_("controls")],
                [
                  button("update", Push("update"), "Update information"),
                  button("dismiss-info", Dismiss_info, "Dismiss information"),
                  button("clear", Clear, "Dismiss all"),
                ],
              ),
            ],
          ),
          Html.section(
            [Html.aria("labelledby", "inbox-title")],
            [
              Html.h2([Html.id("inbox-title")], [Html.text("Inbox")]),
              render(model, model.toast, message => Toast(message)),
            ],
          ),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "alternate-title")],
        [
          Html.h2(
            [Html.id("alternate-title")],
            [Html.text("A different order")],
          ),
          Html.p(
            [],
            [
              Html.text(
                "Two visible entries, newest first. Use Control+Shift+F9 to visit this region.",
              ),
            ],
          ),
          button("alternate", Alternate, "Add to alternative inbox"),
          render(model, model.secondary, message => Secondary(message)),
        ],
      ),
      Html.h2([], [Html.text("Lifecycle log")]),
      Html.node(
        "pre",
        [Html.id("events")],
        [Html.text(String.concat("\n", model.events))],
      ),
      button("after", Undo, "Another way to restore"),
    ],
  );
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              toast: T.init(~id="notifications", ()),
              secondary:
                T.init(
                  ~id="alternative",
                  ~label="Alternative notifications",
                  ~max_visible=2,
                  ~order=T.Newest_first,
                  ~hotkey={
                    key: "F9",
                    ctrl: true,
                    shift: true,
                    alt: false,
                    meta: false,
                  },
                  (),
                ),
              contents: [],
              serial: 0,
              undos: 0,
              events: [],
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch([
            Sub.map(
              message => Toast(message),
              T.subscriptions(model.toast),
            ),
            Sub.map(
              message => Secondary(message),
              T.subscriptions(model.secondary),
            ),
          ]),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
