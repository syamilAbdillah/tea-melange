open Tea;
module F = Ui.File_drop;
type msg =
  | Files(F.msg)
  | Camera(F.msg)
  | Rear(F.msg)
  | Disable(bool);
type model = {
  files: F.model,
  camera: F.model,
  rear: F.model,
  outcomes: list(F.out_msg),
};
let reason = r =>
  switch (r) {
  | F.Not_accepted => "type not accepted"
  | F.Too_large => "file too large"
  | F.Too_many => "too many files"
  };
let update = (msg, model) =>
  switch (msg) {
  | Files(msg) =>
    let (files, cmd, outcomes) = F.update(msg, model.files);
    (
      {
        ...model,
        files,
        outcomes: model.outcomes @ outcomes,
      },
      Cmd.map(x => Files(x), cmd),
    );
  | Camera(msg) =>
    let (camera, cmd, outcomes) = F.update(msg, model.camera);
    (
      {
        ...model,
        camera,
        outcomes: model.outcomes @ outcomes,
      },
      Cmd.map(x => Camera(x), cmd),
    );
  | Rear(msg) =>
    let (rear, cmd, outcomes) = F.update(msg, model.rear);
    (
      {
        ...model,
        rear,
        outcomes: model.outcomes @ outcomes,
      },
      Cmd.map(x => Rear(x), cmd),
    );
  | Disable(value) =>
    let (files, cmd) = F.set_disabled(value, model.files);
    (
      {
        ...model,
        files,
      },
      Cmd.map(x => Files(x), cmd),
    );
  };
let target = (~id, model, to_parent, label, instructions) =>
  F.view(
    ~model,
    ~to_parent,
    ~to_view=
      parts =>
        Html.div(
          parts.root @ [Html.class_("drop-target")],
          [
            Html.p(
              [Html.id(id ++ "-instructions")],
              [Html.text(instructions)],
            ),
            Html.div(
              [Html.class_("drop-detail")],
              [Html.text("Drop files here, or use the button.")],
            ),
            Html.button(
              parts.button
              @ [Html.aria("describedby", id ++ "-instructions")],
              [Html.text(label)],
            ),
            Html.input(parts.input, []),
          ],
        ),
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("File intake")]),
      Html.p(
        [],
        [
          Html.text(
            "Drop from your desktop or choose with the keyboard. Files stay on this device; this demo does not upload them.",
          ),
        ],
      ),
      Html.label(
        [],
        [
          Html.input(
            [
              Html.id("disable"),
              Html.type_("checkbox"),
              Html.checked(F.disabled(model.files)),
              Html.on_check(x => Disable(x)),
            ],
            [],
          ),
          Html.text(" Disable file intake"),
        ],
      ),
      target(
        ~id="files",
        model.files,
        x => Files(x),
        "Choose files",
        "Images or .txt files. Up to 2 files per batch, 1,024 bytes each.",
      ),
      Html.h2([], [Html.text("Camera preferences")]),
      target(
        ~id="camera",
        model.camera,
        x => Camera(x),
        "Take a selfie",
        "One image; prefers the front camera when supported.",
      ),
      target(
        ~id="rear",
        model.rear,
        x => Rear(x),
        "Use rear camera",
        "Disabled example; prefers the rear camera when supported.",
      ),
      Html.section(
        [Html.aria("labelledby", "results-title")],
        [
          Html.h2(
            [Html.id("results-title")],
            [Html.text("Batch results")],
          ),
          Html.div(
            [
              Html.id("results"),
              Html.role("status"),
              Html.aria("live", "polite"),
            ],
            model.outcomes == []
              ? [Html.p([], [Html.text("No files selected.")])]
              : List.concat_map(
                  outcome =>
                    switch (outcome) {
                    | F.Accepted(files) =>
                      List.map(
                        file =>
                          Html.p(
                            [Html.class_("accepted")],
                            [
                              Html.text(
                                "Accepted: "
                                ++ file.F.name
                                ++ " ("
                                ++ Js.Float.toString(file.size)
                                ++ " bytes, "
                                ++ file.mime
                                ++ ")",
                              ),
                            ],
                          ),
                        files,
                      )
                    | F.Rejected(files) =>
                      List.map(
                        ((file, why)) =>
                          Html.p(
                            [Html.class_("rejected")],
                            [
                              Html.text(
                                "Rejected: "
                                ++ file.F.name
                                ++ ": "
                                ++ reason(why),
                              ),
                            ],
                          ),
                        files,
                      )
                    },
                  model.outcomes,
                ),
          ),
        ],
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
              files:
                F.init(
                  ~id="files",
                  ~multiple=true,
                  ~accept=["image/*", ".txt"],
                  ~max_count=2,
                  ~max_size=1024.,
                  (),
                ),
              camera:
                F.init(
                  ~id="camera",
                  ~accept=["image/*"],
                  ~capture=F.User,
                  (),
                ),
              rear:
                F.init(
                  ~id="rear",
                  ~accept=["image/*"],
                  ~capture=F.Environment,
                  ~disabled=true,
                  (),
                ),
              outcomes: [],
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch([
            Sub.map(x => Files(x), F.subscriptions(model.files)),
            Sub.map(x => Camera(x), F.subscriptions(model.camera)),
            Sub.map(x => Rear(x), F.subscriptions(model.rear)),
          ]),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
