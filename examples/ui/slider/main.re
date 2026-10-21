open Tea;
module S = Ui.Slider;
let thumb = (key, label, initial) => {
  S.key,
  label: S.Label(label),
  initial,
  disabled: false,
};
let items = [|
  [|
    {
      S.key: "volume",
      label: S.Labelled_by(["volume-label"]),
      initial: 40.,
      disabled: false,
    },
  |],
  [|
    thumb("minimum", "Minimum price", 20.),
    thumb("maximum", "Maximum price", 80.),
  |],
  [|thumb("temperature", "Temperature", 50.)|],
  [|thumb("balance", "Balance", 40.)|],
  [|thumb("locked", "Locked volume", 40.)|],
|];
type model = {
  sliders: array(S.model),
  changed: int,
  committed: int,
};
type msg =
  | Slider(int, S.msg)
  | Reset;
let initial = () => {
  sliders: [|
    S.init(~id="volume", ~items=items[0], ~step=0.1, ()),
    S.init(~id="price", ~items=items[1], ~min_distance=10, ()),
    S.init(~id="vertical", ~items=items[2], ~orientation=S.Vertical, ()),
    S.init(~id="rtl", ~items=items[3], ~dir=S.Rtl, ~large_step=20, ()),
    S.init(~id="disabled", ~items=items[4], ~disabled=true, ()),
  |],
  changed: 0,
  committed: 0,
};
let update = (message, model) =>
  switch (message) {
  | Reset => (initial(), Cmd.none)
  | Slider(i, message) =>
    let (slider, command, outputs) =
      S.update(~items=items[i], message, model.sliders[i]);
    let sliders =
      Array.mapi((j, old) => i == j ? slider : old, model.sliders);
    let (changed, committed) =
      List.fold_left(
        ((changed, committed), output) =>
          switch (output) {
          | S.Changed(_) => (changed + 1, committed)
          | S.Committed(_) => (changed, committed + 1)
          },
        (model.changed, model.committed),
        outputs,
      );
    (
      {
        sliders,
        changed,
        committed,
      },
      Cmd.map(m => Slider(i, m), command),
    );
  };
let render = (model, i, title) =>
  Html.section(
    [],
    [
      Html.h2(
        [Html.id(i == 0 ? "volume-label" : "heading-" ++ string_of_int(i))],
        [Html.text(title)],
      ),
      S.view(
        ~model=model.sliders[i],
        ~items=items[i],
        ~to_parent=m => Slider(i, m),
        ~name=i == 1 ? "price" : "value-" ++ string_of_int(i),
        ~format_value=
          (_, v) =>
            Js.Float.toString(v)
            ++ (i == 1 ? " dollars" : i == 2 ? " degrees" : " percent"),
        ~to_view=
          p =>
            Html.div(
              p.root @ [Html.class_("slider")],
              [
                Html.div(
                  p.track @ [Html.class_("track")],
                  [
                    Html.div(p.range @ [Html.class_("fill")], []),
                    Html.Keyed.div(
                      [],
                      Array.to_list(
                        Array.map(
                          (t: S.thumb_parts(msg)) =>
                            (
                              t.key,
                              Html.div(
                                t.root @ [Html.class_("thumb")],
                                [Html.text(Js.Float.toString(t.value))],
                              ),
                            ),
                          p.thumbs,
                        ),
                      ),
                    ),
                  ],
                ),
                Html.div(
                  [],
                  Array.to_list(
                    Array.map(
                      (t: S.thumb_parts(msg)) => Html.input(t.input, []),
                      p.thumbs,
                    ),
                  ),
                ),
              ],
            ),
        (),
      ),
    ],
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("A value in your hands.")]),
      Html.p(
        [],
        [
          Html.text(
            "Drag the track, or focus a handle and use the arrow keys. Home and End reach its limits; Page Up and Page Down take larger steps.",
          ),
        ],
      ),
      Html.button(
        [Html.id("reset"), Html.type_("button"), Html.on_click(Reset)],
        [Html.text("Reset values")],
      ),
      Html.form(
        [Html.aria("label", "Slider settings"), Html.on_submit(Reset)],
        [
          render(model, 0, "Volume"),
          render(model, 1, "Price range · at least $10 apart"),
          render(model, 2, "Vertical temperature"),
          render(model, 3, "Right-to-left balance"),
          render(model, 4, "Disabled volume"),
        ],
      ),
      Html.output(
        [Html.id("activity")],
        [
          Html.text(
            "Changes: "
            ++ string_of_int(model.changed)
            ++ ". Commits: "
            ++ string_of_int(model.committed)
            ++ ".",
          ),
        ],
      ),
    ],
  );
let () =
  ignore(
    App.element(
      ~init=() => (initial(), Cmd.none),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch(
            Array.to_list(
              Array.mapi(
                (i, slider) =>
                  Sub.map(m => Slider(i, m), S.subscriptions(slider)),
                model.sliders,
              ),
            ),
          ),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
