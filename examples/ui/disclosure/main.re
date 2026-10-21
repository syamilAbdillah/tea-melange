open Tea;
module D = Ui.Disclosure;
type model = {
  open_: bool,
  keep: bool,
  active: option(string),
};
type msg =
  | Toggle
  | Keep(bool)
  | Accordion(string);
let update = (msg, model) => (
  switch (msg) {
  | Toggle => {
      ...model,
      open_: !model.open_,
    }
  | Keep(keep) => {
      ...model,
      keep,
    }
  | Accordion(key) => {
      ...model,
      active:
        switch (model.active) {
        | Some(active) when String.equal(active, key) => None
        | _ => Some(key)
        },
    }
  },
  Cmd.none,
);
let disclosure = (~id, ~open_, ~on_toggle, ~keep_mounted=true, title, content) =>
  D.view(
    ~id,
    ~open_,
    ~on_toggle,
    ~keep_mounted,
    ~to_view=
      p =>
        Html.div(
          [],
          [
            Html.h3([], [Html.button(p.button, [Html.text(title)])]),
            p.mounted
              ? Html.div(
                  p.panel @ [Html.class_("panel")],
                  [
                    Html.p([], [Html.text(content)]),
                    Html.a(
                      [Html.id(id ++ "-link"), Html.href("#end")],
                      [Html.text("Continue reading")],
                    ),
                  ],
                )
              : Html.text(""),
          ],
        ),
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Make room for detail")]),
      Html.p(
        [],
        [
          Html.text(
            "The page owns what is open. Native buttons keep focus and expose each panel's visibility without requiring a stylesheet.",
          ),
        ],
      ),
      Html.label(
        [],
        [
          Html.input(
            [
              Html.id("keep"),
              Html.type_("checkbox"),
              Html.checked(model.keep),
              Html.on_check(value => Keep(value)),
            ],
            [],
          ),
          Html.text(" Keep closed details mounted"),
        ],
      ),
      Html.section(
        [],
        [
          Html.h2([], [Html.text("One disclosure")]),
          disclosure(
            ~id="details",
            ~open_=model.open_,
            ~on_toggle=Toggle,
            ~keep_mounted=model.keep,
            "Storage details",
            "Your files stay available while you are offline.",
          ),
        ],
      ),
      Html.section(
        [],
        [
          Html.h2([], [Html.text("An exclusive accordion")]),
          Html.p(
            [],
            [
              Html.text(
                "Only one section may be open; all may be closed. Heading levels belong to this page's information architecture.",
              ),
            ],
          ),
          disclosure(
            ~id="first",
            ~open_=model.active == Some("first"),
            ~on_toggle=Accordion("first"),
            "Access",
            "Your team members can read shared files.",
          ),
          disclosure(
            ~id="second",
            ~open_=model.active == Some("second"),
            ~on_toggle=Accordion("second"),
            "Retention",
            "Archived files remain available for thirty days.",
          ),
          disclosure(
            ~id="third",
            ~open_=model.active == Some("third"),
            ~on_toggle=Accordion("third"),
            "Recovery",
            "Restore an archived file from its history.",
          ),
        ],
      ),
      Html.p(
        [Html.id("end")],
        [Html.text("End of the disclosure examples.")],
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
              open_: false,
              keep: true,
              active: None,
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
