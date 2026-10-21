open Tea;
module T = Ui.Tooltip;
module D = Ui.Dialog;
module G = Ui.Hover_intent.Group;
type entry = {
  key: string,
  tip: T.model,
};
type model = {
  tips: list(entry),
  group: G.t,
  dialog: D.model,
  actions: int,
};
type msg =
  | Tip(string, T.msg)
  | Dialog(D.msg)
  | Open_dialog
  | Action
  | Disable(bool);
let create = key => {
  key,
  tip:
    switch (key) {
    | "cut"
    | "copy" =>
      T.init(~id=key, ~mode=T.Label, ~close_delay_ms=0, ~offset=0., ())
    | "animated" => T.init(~id=key, ~arrow_size=8., ~offset=12., ())
    | "modal-help" => T.init(~id=key, ~mode=T.Label, ())
    | _ => T.init(~id=key, ())
    },
};
let update = (message, model) =>
  switch (message) {
  | Tip(key, message) =>
    let (tips, commands, group) =
      List.fold_right(
        (entry, (tips, commands, group)) =>
          if (String.equal(entry.key, key)) {
            let grouped = List.mem(key, ["cut", "copy"]);
            let (tip, command, outs) =
              T.update(
                ~group=?grouped ? Some(model.group) : None,
                message,
                entry.tip,
              );
            let group =
              List.fold_left(
                (group, out) =>
                  switch (out) {
                  | T.Opened => group
                  | T.Closed(time_stamp) =>
                    grouped ? G.record_close(~time_stamp, group) : group
                  },
                group,
                outs,
              );
            (
              [
                {
                  ...entry,
                  tip,
                },
                ...tips,
              ],
              [Cmd.map(x => Tip(key, x), command), ...commands],
              group,
            );
          } else {
            ([entry, ...tips], commands, group);
          },
        model.tips,
        ([], [], model.group),
      );
    (
      {
        ...model,
        tips,
        group,
      },
      Cmd.batch(commands),
    );
  | Dialog(message) =>
    let (dialog, command, _) = D.update(message, model.dialog);
    /* A dialog's removed/hidden subtree must not retain an active tooltip. */
    let (tips, commands) =
      List.fold_right(
        (entry, (tips, commands)) =>
          if (String.equal(entry.key, "modal-help") && !D.is_open(dialog)) {
            let (tip, command) = T.close(entry.tip);
            (
              [
                {
                  ...entry,
                  tip,
                },
                ...tips,
              ],
              [Cmd.map(x => Tip(entry.key, x), command), ...commands],
            );
          } else {
            ([entry, ...tips], commands);
          },
        model.tips,
        ([], []),
      );
    (
      {
        ...model,
        dialog,
        tips,
      },
      Cmd.batch([Cmd.map(x => Dialog(x), command), ...commands]),
    );
  | Open_dialog =>
    let (dialog, command) = D.open_(model.dialog);
    (
      {
        ...model,
        dialog,
      },
      Cmd.map(x => Dialog(x), command),
    );
  | Action => (
      {
        ...model,
        actions: model.actions + 1,
      },
      Cmd.none,
    )
  | Disable(disabled) =>
    let (tips, commands) =
      List.fold_right(
        (entry, (tips, commands)) =>
          if (String.equal(entry.key, "animated")) {
            let (tip, command) = T.set_disabled(disabled, entry.tip);
            (
              [
                {
                  ...entry,
                  tip,
                },
                ...tips,
              ],
              [Cmd.map(x => Tip(entry.key, x), command), ...commands],
            );
          } else {
            ([entry, ...tips], commands);
          },
        model.tips,
        ([], []),
      );
    (
      {
        ...model,
        tips,
      },
      Cmd.batch(commands),
    );
  };
let tip = (model, key, trigger, text) => {
  let entry = List.find(entry => String.equal(entry.key, key), model.tips);
  T.view(
    ~model=entry.tip,
    ~to_parent=x => Tip(key, x),
    ~to_view=
      parts =>
        Html.div(
          [Html.class_("pair")],
          [
            trigger(parts.trigger),
            Html.div(
              parts.tooltip
              @ [
                Html.class_(
                  String.equal(key, "animated") ? "tip animated" : "tip",
                ),
              ],
              [
                Html.text(text),
                String.equal(key, "animated")
                  ? Html.span(parts.arrow @ [Html.class_("arrow")], [])
                  : Html.text(""),
              ],
            ),
          ],
        ),
    (),
  );
};
let button = (attrs, text) =>
  Html.button(
    attrs @ [Html.type_("button"), Html.on_click(Action)],
    [Html.text(text)],
  );
let icon_button = (attrs, glyph) =>
  Html.button(
    attrs @ [Html.type_("button"), Html.on_click(Action)],
    [Html.span([Html.aria("hidden", "true")], [Html.text(glyph)])],
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("A little help, right where you need it.")]),
      Html.p(
        [],
        [
          Html.text(
            "Hover for a moment, or use Tab. Escape dismisses help without moving your focus.",
          ),
        ],
      ),
      Html.button(
        [Html.id("start"), Html.type_("button")],
        [Html.text("Start keyboard tour")],
      ),
      Html.section(
        [Html.aria("labelledby", "toolbar-title")],
        [
          Html.h2(
            [Html.id("toolbar-title")],
            [Html.text("Editing tools")],
          ),
          Html.div(
            [
              Html.role("group"),
              Html.aria("label", "Document toolbar"),
              Html.class_("tools"),
            ],
            [
              tip(
                model,
                "cut",
                attrs => icon_button(attrs, {js|✂|js}),
                "Cut selection",
              ),
              tip(
                model,
                "copy",
                attrs => icon_button(attrs, {js|▣|js}),
                "Copy selection",
              ),
            ],
          ),
          Html.p(
            [],
            [
              Html.text(
                "The first hover waits 600ms. Move to the next icon within 1.8 seconds and help appears immediately. This labelled group keeps native Tab order.",
              ),
            ],
          ),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "form-title")],
        [
          Html.h2([Html.id("form-title")], [Html.text("Form field help")]),
          Html.label(
            [Html.for_("help-trigger")],
            [Html.text("Project name")],
          ),
          tip(
            model,
            "help",
            attrs => Html.input(attrs @ [Html.type_("text")], []),
            "Use a name your team will recognize.",
          ),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "motion-title")],
        [
          Html.h2(
            [Html.id("motion-title")],
            [Html.text("Hoverable help, with an arrow")],
          ),
          tip(
            model,
            "animated",
            attrs => button(attrs, "Sharing"),
            "Only invited teammates can see this project.",
          ),
          Html.label(
            [],
            [
              Html.input(
                [
                  Html.id("disable"),
                  Html.type_("checkbox"),
                  Html.on_check(x => Disable(x)),
                ],
                [],
              ),
              Html.text(" Disable sharing tooltip"),
            ],
          ),
          Html.p(
            [],
            [
              Html.text(
                "You can move onto the tooltip and read at your own pace. Motion respects reduced-motion preferences.",
              ),
            ],
          ),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "disabled-title")],
        [
          Html.h2(
            [Html.id("disabled-title")],
            [Html.text("Help for a disabled control")],
          ),
          tip(
            model,
            "disabled",
            attrs =>
              Html.span(
                attrs
                @ [
                  Html.tabindex(0),
                  Html.role("group"),
                  Html.aria("label", "Publish unavailable"),
                  Html.class_("disabled-wrapper"),
                ],
                [
                  Html.button(
                    [
                      Html.type_("button"),
                      Html.disabled(true),
                      Html.class_("disabled-control"),
                    ],
                    [Html.text("Publish")],
                  ),
                ],
              ),
            "Add a project name before publishing.",
          ),
        ],
      ),
      Html.section(
        [Html.aria("labelledby", "dialog-title")],
        [
          Html.h2(
            [Html.id("dialog-title")],
            [Html.text("Help inside a modal")],
          ),
          Html.button(
            [
              Html.id("open-dialog"),
              Html.type_("button"),
              Html.on_click(Open_dialog),
            ],
            [Html.text("Open project settings")],
          ),
          D.view(
            ~model=model.dialog,
            ~labels={ D.close: "Close settings" },
            ~to_parent=x => Dialog(x),
            ~to_view=
              parts =>
                Html.dialog(
                  parts.root,
                  [
                    Html.div(
                      [Html.class_("dialog-content")],
                      [
                        Html.h2(
                          parts.title,
                          [Html.text("Project settings")],
                        ),
                        Html.button(
                          parts.close_button,
                          [Html.text("Close settings")],
                        ),
                        tip(
                          model,
                          "modal-help",
                          attrs => icon_button(attrs, "?"),
                          "Settings help",
                        ),
                        Html.p(
                          [],
                          [
                            Html.text(
                              "Tab to the question mark. First Escape closes help; second Escape closes settings.",
                            ),
                          ],
                        ),
                      ],
                    ),
                  ],
                ),
            (),
          ),
        ],
      ),
      Html.p(
        [Html.id("activity"), Html.aria("live", "polite")],
        [Html.text("Actions: " ++ string_of_int(model.actions))],
      ),
    ],
  );
let subscriptions = model =>
  Sub.batch([
    D.subscriptions(model.dialog) |> Sub.map(x => Dialog(x)),
    Sub.batch(
      List.map(
        entry =>
          T.subscriptions(entry.tip) |> Sub.map(x => Tip(entry.key, x)),
        model.tips,
      ),
    ),
  ]);
let () =
  ignore(
    App.element(
      ~init=
        () =>
          (
            {
              tips:
                List.map(
                  create,
                  [
                    "cut",
                    "copy",
                    "help",
                    "animated",
                    "disabled",
                    "modal-help",
                  ],
                ),
              group: G.init(~skip_delay_ms=1800, ()),
              dialog: D.init(~id="settings", ()),
              actions: 0,
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
