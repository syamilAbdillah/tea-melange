open Tea;
module A = Ui.Anchor;
type settings = {
  placement: A.placement,
  offset: float,
  flip: bool,
  shift: bool,
  match_width: bool,
  rtl: bool,
  top_layer: bool,
};
type model = {
  anchor: A.model,
  settings,
  changes: int,
};
type msg =
  | Anchor(A.msg)
  | Toggle
  | Placement(string)
  | Offset(string)
  | Flip(bool)
  | Shift(bool)
  | Match(bool)
  | Rtl(bool)
  | Top_layer(bool)
  | Refresh;
let placements = [
  (A.Top, A.Start),
  (A.Top, A.Center),
  (A.Top, A.End),
  (A.Bottom, A.Start),
  (A.Bottom, A.Center),
  (A.Bottom, A.End),
  (A.Left, A.Start),
  (A.Left, A.Center),
  (A.Left, A.End),
  (A.Right, A.Start),
  (A.Right, A.Center),
  (A.Right, A.End),
];
let update = (msg, model) =>
  switch (msg) {
  | Anchor(msg) =>
    let (anchor, cmd, changes) = A.update(msg, model.anchor);
    (
      {
        ...model,
        anchor,
        changes: model.changes + List.length(changes),
      },
      Cmd.map(msg => Anchor(msg), cmd),
    );
  | Toggle
  | Refresh =>
    let (anchor, cmd) =
      switch (msg) {
      | Refresh => A.refresh(model.anchor)
      | _ =>
        A.active(model.anchor)
          ? A.stop(model.anchor) : A.start(model.anchor)
      };
    (
      {
        ...model,
        anchor,
      },
      Cmd.map(msg => Anchor(msg), cmd),
    );
  | _ =>
    let s = model.settings;
    let settings =
      switch (msg) {
      | Placement(name) => {
          ...s,
          placement:
            Option.value(
              ~default=s.placement,
              List.find_opt(
                p => String.equal(A.placement_name(p), name),
                placements,
              ),
            ),
        }
      | Offset(value) => {
          ...s,
          offset:
            Option.value(~default=s.offset, float_of_string_opt(value)),
        }
      | Flip(flip) => {
          ...s,
          flip,
        }
      | Shift(shift) => {
          ...s,
          shift,
        }
      | Match(match_width) => {
          ...s,
          match_width,
        }
      | Rtl(rtl) => {
          ...s,
          rtl,
        }
      | Top_layer(top_layer) => {
          ...s,
          top_layer,
        }
      | _ => s
      };
    let (anchor, cmd) =
      A.configure(
        ~placement=settings.placement,
        ~offset=settings.offset,
        ~flip=settings.flip,
        ~shift_padding=settings.shift ? Some(8.) : None,
        ~match_width=settings.match_width,
        ~dir=settings.rtl ? A.Rtl : A.Ltr,
        ~top_layer=settings.top_layer,
        model.anchor,
      );
    (
      {
        ...model,
        settings,
        anchor,
      },
      Cmd.map(msg => Anchor(msg), cmd),
    );
  };
let check_control = (id, label, checked, to_msg) =>
  Html.label(
    [Html.for_(id)],
    [
      Html.input(
        [
          Html.id(id),
          Html.type_("checkbox"),
          Html.checked(checked),
          Html.on_check(to_msg),
        ],
        [],
      ),
      Html.text(" " ++ label),
    ],
  );
let view = model => {
  let s = model.settings;
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Position is a relationship.")]),
      Html.p(
        [],
        [
          Html.text(
            "Move the reference, scroll the canvas, or resize the button. The panel follows without changing its place in the document.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("controls")],
        [
          Html.label(
            [Html.for_("placement")],
            [
              Html.text("Placement "),
              Html.select(
                [
                  Html.id("placement"),
                  Html.on_change(name => Placement(name)),
                ],
                List.map(
                  p =>
                    Html.option(
                      [
                        Html.value(A.placement_name(p)),
                        Html.selected(
                          String.equal(
                            A.placement_name(p),
                            A.placement_name(s.placement),
                          ),
                        ),
                      ],
                      [Html.text(A.placement_name(p))],
                    ),
                  placements,
                ),
              ),
            ],
          ),
          Html.label(
            [Html.for_("offset")],
            [
              Html.text("Offset (px) "),
              Html.input(
                [
                  Html.id("offset"),
                  Html.type_("number"),
                  Html.attribute("min", "-100"),
                  Html.attribute("max", "100"),
                  Html.value(Js.Float.toString(s.offset)),
                  Html.on_input(value => Offset(value)),
                ],
                [],
              ),
            ],
          ),
          check_control("flip", "Flip", s.flip, value => Flip(value)),
          check_control("shift", "Shift (8px padding)", s.shift, value =>
            Shift(value)
          ),
          check_control("match", "Match width", s.match_width, value =>
            Match(value)
          ),
          check_control("rtl", "RTL", s.rtl, value => Rtl(value)),
          check_control("top-layer", "Native top layer", s.top_layer, value =>
            Top_layer(value)
          ),
          Ui.Button.button(
            ~id="refresh",
            ~on_press=Refresh,
            [Html.text("Measure again")],
          ),
        ],
      ),
      Html.div(
        [Html.id("clip")],
        [
          Html.div(
            [
              Html.id("scroller"),
              Html.tabindex(0),
              Html.role("region"),
              Html.aria("label", "Scrollable positioning canvas"),
            ],
            [
              Html.div(
                [Html.id("surface")],
                [
                  Html.div(
                    [Html.id("resizer")],
                    [
                      Ui.Button.button(
                        ~id="reference",
                        ~on_press=Toggle,
                        ~attrs=[
                          Html.aria(
                            "expanded",
                            A.active(model.anchor) ? "true" : "false",
                          ),
                          Html.aria("controls", "floating"),
                          Html.attribute("dir", s.rtl ? "rtl" : "ltr"),
                        ],
                        [Html.text("Toggle panel")],
                      ),
                    ],
                  ),
                  A.view(
                    ~model=model.anchor,
                    ~to_parent=msg => Anchor(msg),
                    ~to_view=
                      parts =>
                        Html.div(
                          parts.floating
                          @ [
                            Html.role("region"),
                            Html.aria("label", "Floating panel"),
                          ],
                          [
                            Html.p(
                              [],
                              [Html.text("Attached, not contained.")],
                            ),
                            Html.p(
                              [],
                              [Html.text("Scroll to see it follow.")],
                            ),
                          ],
                        ),
                    (),
                  ),
                ],
              ),
            ],
          ),
        ],
      ),
      Html.p(
        [Html.class_("hint")],
        [
          Html.text(
            "The blue frame clips overflow and establishes a transformed containing block. The native top layer escapes both. Drag the button’s bottom-right corner to resize.",
          ),
        ],
      ),
      Html.node(
        "output",
        [Html.id("measurements")],
        [
          Html.text("Measurements changed: " ++ string_of_int(model.changes)),
        ],
      ),
    ],
  );
};
let () = {
  let app =
    App.element(
      ~init=
        () => {
          let (anchor, cmd) =
            A.start(
              A.init(
                ~anchor_id="reference",
                ~floating_id="floating",
                ~offset=8.,
                ~top_layer=true,
                (),
              ),
            );
          (
            {
              anchor,
              settings: {
                placement: (A.Bottom, A.Start),
                offset: 8.,
                flip: true,
                shift: true,
                match_width: false,
                rtl: false,
                top_layer: true,
              },
              changes: 0,
            },
            Cmd.map(msg => Anchor(msg), cmd),
          );
        },
      ~update,
      ~view,
      ~subscriptions=
        model => Sub.map(msg => Anchor(msg), A.subscriptions(model.anchor)),
      ~node="#app",
      ~flags=(),
      (),
    );
  ignore(app);
};
