open Tea;
open Test_support;
module S = Ui.Slider;
let thumb = (key, initial) => {
  S.key,
  initial,
  label: S.Label("Thumb " ++ key),
  disabled: false,
};
let single = [|thumb("a", 40.)|];
let pair = [|thumb("a", 20.), thumb("b", 80.)|];
type model = {
  slider: S.model,
  items: array(S.thumb),
  outputs: list(S.out_msg),
  hooks: int,
};
type msg =
  | Slider(S.msg)
  | Replace(S.model, array(S.thumb))
  | Values(S.values)
  | Disable(bool)
  | Hook;
let update = (message, model) =>
  switch (message) {
  | Slider(message) =>
    let (slider, cmd, out) =
      S.update(~items=model.items, message, model.slider);
    (
      {
        ...model,
        slider,
        outputs: model.outputs @ out,
      },
      Cmd.map(m => Slider(m), cmd),
    );
  | Replace(slider, items) => (
      {
        slider,
        items,
        outputs: [],
        hooks: 0,
      },
      Cmd.none,
    )
  | Values(values) =>
    let (slider, cmd) =
      S.set_values(~items=model.items, ~values, model.slider);
    (
      {
        ...model,
        slider,
      },
      Cmd.map(m => Slider(m), cmd),
    );
  | Disable(disabled) =>
    let (slider, cmd) = S.set_disabled(disabled, model.slider);
    (
      {
        ...model,
        slider,
      },
      Cmd.map(m => Slider(m), cmd),
    );
  | Hook => (
      {
        ...model,
        hooks: model.hooks + 1,
      },
      Cmd.none,
    )
  };
let view = model =>
  Html.div(
    [],
    [
      Html.button(
        [Html.id("before"), Html.type_("button")],
        [Html.text("Before")],
      ),
      Html.span([Html.id("label")], [Html.text("Volume")]),
      S.view(
        ~model=model.slider,
        ~items=model.items,
        ~to_parent=m => Slider(m),
        ~name="setting",
        ~format_value=(key, v) => key ++ ": " ++ Js.Float.toString(v),
        ~on_key_down=key => key == "x" ? Some(Hook) : None,
        ~to_view=
          p =>
            Html.div(
              p.root,
              [
                Html.div(
                  p.track,
                  [
                    Html.div(p.range, []),
                    Html.Keyed.div(
                      [],
                      Array.to_list(
                        Array.map(
                          (t: S.thumb_parts(msg)) =>
                            (t.key, Html.div(t.root, [])),
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
let invalid = (label, f) => {
  let raised =
    try(
      {
        ignore(f());
        false;
      }
    ) {
    | Invalid_argument(_) => true
    };
  check(label, raised);
};
let pointer_at = (~id=1, ~button=0, target, kind, x, y) =>
  dispatch(
    target,
    pointer_event(
      kind,
      {
        "bubbles": true,
        "cancelable": true,
        "pointerId": id,
        "button": button,
        "clientX": x,
        "clientY": y,
      },
    ),
  );
let completed = {
  List.iter(
    ((min, max, step)) =>
      invalid("invalid numeric config", () =>
        S.init(~id="s", ~items=single, ~min, ~max, ~step, ())
      ),
    [
      (0., 100., 0.),
      (0., 100., (-1.)),
      (1., 1., 1.),
      (2., 1., 1.),
      (nan, 1., 1.),
      (0., nan, 1.),
      (0., 1., nan),
      (0., infinity, 1.),
      (0., 1., infinity),
      (0., 1., 1e-16),
      (0., 1., 2.),
    ],
  );
  invalid("negative distance", () =>
    S.init(~id="s", ~items=single, ~min_distance=-1, ())
  );
  invalid("zero large step", () =>
    S.init(~id="s", ~items=single, ~large_step=0, ())
  );
  invalid("duplicate keys", () =>
    S.init(~id="s", ~items=[|thumb("a", 0.), thumb("a", 1.)|], ())
  );
  invalid("nonfinite initial", () =>
    S.init(~id="s", ~items=[|thumb("a", nan)|], ())
  );
  invalid("empty name", () =>
    S.init(
      ~id="s",
      ~items=[|
        {
          ...single[0],
          label: S.Label(""),
        },
      |],
      (),
    )
  );
  invalid("empty idrefs", () =>
    S.init(
      ~id="s",
      ~items=[|
        {
          ...single[0],
          label: S.Labelled_by([]),
        },
      |],
      (),
    )
  );
  invalid("infeasible distance", () =>
    S.init(~id="s", ~items=pair, ~min_distance=101, ())
  );
  let grid_items = [|thumb("a", 0.), thumb("b", 1.), thumb("c", 2.)|];
  let grid =
    S.init(
      ~id="grid",
      ~items=grid_items,
      ~max=2.,
      ~step=0.1,
      ~min_distance=2,
      (),
    );
  for (i in (-20) to 40) {
    let input = float_of_int(i) /. 10.;
    let (next, _) =
      S.set_values(
        ~items=grid_items,
        ~values=[("a", input), ("b", 2. -. input), ("c", input)],
        grid,
      );
    switch (S.values(~items=grid_items, next)) {
    | [(_, a), (_, b), (_, c)] =>
      check(
        "grid and spacing invariant",
        a >= 0.
        && c <= 2.
        && b
        -. a >= 0.2
        -. 1e-12
        && c
        -. b >= 0.2
        -. 1e-12
        && abs_float(a *. 10. -. floor(a *. 10. +. 0.5)) < 1e-9,
      )
    | _ => check("three keyed values", false)
    };
  };
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              slider: S.init(~id="s", ~items=single, ()),
              items: single,
              outputs: [],
              hooks: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        m => Sub.map(m => Slider(m), S.subscriptions(m.slider)),
      ~node="#app",
      ~flags=(),
      (),
    );
  let reset =
      (
        ~items=single,
        ~min=0.,
        ~max=100.,
        ~step=1.,
        ~min_distance=0,
        ~large_step=10,
        ~orientation=S.Horizontal,
        ~dir=S.Ltr,
        (),
      ) => {
    app.dispatch(
      Replace(
        S.init(
          ~id="s",
          ~items,
          ~min,
          ~max,
          ~step,
          ~min_distance,
          ~large_step,
          ~orientation,
          ~dir,
          (),
        ),
        items,
      ),
    );
    frame();
  };
  let value = key =>
    List.assoc(key, S.values(~items=app.model().items, app.model().slider));
  let count = () => List.length(app.model().outputs);
  let press = (key, expected) => {
    check(
      "handled key " ++ key,
      !Test_support.key("#s-item-a", "keydown", key),
    );
    frame();
    check("key value " ++ key, value("a") == expected);
  };
  check("role", attr("#s-item-a", "role") == Some("slider"));
  check("tabindex", attr("#s-item-a", "tabindex") == Some("0"));
  check(
    "aria initial",
    attr("#s-item-a", "aria-valuenow") == Some("40")
    && attr("#s-item-a", "aria-valuemin") == Some("0")
    && attr("#s-item-a", "aria-valuemax") == Some("100"),
  );
  check(
    "formatted value",
    attr("#s-item-a", "aria-valuetext") == Some("a: 40"),
  );
  check("name", attr("#s-item-a", "aria-label") == Some("Thumb a"));
  check(
    "orientation",
    attr("#s-item-a", "aria-orientation") == Some("horizontal")
    && attr("#s-track", "data-orientation") == Some("horizontal"),
  );
  check(
    "touch behaviour",
    get(get(node("#s-track"), "style"), "touchAction") == "none",
  );
  check("no focus data", attr("#s-item-a", "data-focus") == None);
  check(
    "hidden form",
    get(node("input[name=setting]"), "value") == "40"
    && attr("input", "type") == Some("hidden"),
  );
  focus(node("#s-item-a"));
  press("ArrowRight", 41.);
  press("ArrowUp", 42.);
  press("ArrowLeft", 41.);
  press("ArrowDown", 40.);
  press("PageUp", 50.);
  press("PageDown", 40.);
  press("Home", 0.);
  press("End", 100.);
  check("keyboard focus preserved", has_focus("#s-item-a"));
  check("changed committed pairs", count() == 16);
  let before = count();
  press("End", 100.);
  check("bound no output", count() == before);
  ignore(key(~shift=true, "#s-item-a", "keydown", "ArrowLeft"));
  frame();
  check("shift large step", value("a") == 90.);
  List.iter(
    k => {
      check("ctrl untouched", key(~ctrl=true, "#s-item-a", "keydown", k));
      check("alt untouched", key(~alt=true, "#s-item-a", "keydown", k));
      check("meta untouched", key(~meta=true, "#s-item-a", "keydown", k));
      check(
        "IME untouched",
        key(~composing=true, "#s-item-a", "keydown", k),
      );
    },
    [
      "ArrowRight",
      "ArrowUp",
      "ArrowDown",
      "ArrowLeft",
      "PageUp",
      "PageDown",
      "Home",
      "End",
    ],
  );
  check("Tab untouched", key("#s-item-a", "keydown", "Tab"));
  check(
    "Shift Tab untouched",
    key(~shift=true, "#s-item-a", "keydown", "Tab"),
  );
  check("unhandled key", key("#s-item-a", "keydown", "x"));
  check("hook delivered", app.model().hooks == 1);
  ignore(key("#s-item-a", "keydown", "ArrowLeft"));
  ignore(key("#s-item-a", "keydown", "ArrowLeft"));
  frame();
  check("rapid intents", value("a") == 88.);
  let before = count();
  app.dispatch(Values([("a", 200.)]));
  frame();
  check(
    "setter clamps without output",
    value("a") == 100. && count() == before,
  );
  invalid("setter nonfinite", () =>
    S.set_values(~items=single, ~values=[("a", nan)], app.model().slider)
  );
  reset(~items=[|thumb("a", 0.1)|], ~max=1., ~step=0.1, ());
  press("ArrowUp", 0.2);
  press("ArrowUp", 0.3);
  app.dispatch(Values([("a", 0.1 +. 0.2)]));
  frame();
  check(
    "no binary drift",
    attr("#s-item-a", "aria-valuenow") == Some("0.3"),
  );
  app.dispatch(Values([("a", 0.15)]));
  frame();
  check("half step rounds upward", value("a") == 0.2);
  reset(~min=0.05, ~max=1., ~step=0.1, ~items=[|thumb("a", 0.05)|], ());
  press("ArrowRight", 0.15);
  press("End", 0.95);
  reset(~items=[|thumb("a", 0.)|], ~max=0.2999999999999999, ~step=0.1, ());
  press("End", 0.2);
  reset(~items=[|thumb("a", 0.)|], ~max=1e-15, ~step=1e-16, ());
  press("ArrowUp", 1e-16);
  press("End", 1e-15);
  invalid("native decimal precision limit", () =>
    S.init(~id="tiny", ~items=[||], ~max=1e-100, ~step=1e-101, ())
  );
  reset(~dir=S.Rtl, ~large_step=20, ());
  press("ArrowRight", 39.);
  press("ArrowLeft", 40.);
  press("ArrowUp", 41.);
  press("PageUp", 61.);
  reset(~orientation=S.Vertical, ());
  press("ArrowUp", 41.);
  press("ArrowDown", 40.);
  check(
    "vertical aria",
    attr("#s-item-a", "aria-orientation") == Some("vertical"),
  );
  reset(~items=pair, ~min_distance=10, ());
  check(
    "neighbour aria",
    attr("#s-item-a", "aria-valuemax") == Some("70")
    && attr("#s-item-b", "aria-valuemin") == Some("30"),
  );
  press("End", 70.);
  check(
    "neighbour updates",
    attr("#s-item-b", "aria-valuemin") == Some("80"),
  );
  check(
    "form naming",
    get(node("input[name='setting[a]']"), "value") == "70"
    && get(node("input[name='setting[b]']"), "value") == "80",
  );
  ignore(key("#s-item-b", "keydown", "Home"));
  frame();
  check("no crossing", value("b") == 80.);
  app.dispatch(Values([("a", 100.), ("b", 0.)]));
  frame();
  check("setter reserves spacing", value("a") == 90. && value("b") == 100.);
  let changed_items = [|
    thumb("b", 0.),
    thumb("c /_", 10.),
    thumb("d", 90.),
  |];
  app.dispatch(Replace(app.model().slider, changed_items));
  frame();
  check(
    "collection repair",
    S.values(~items=changed_items, app.model().slider)
    == [("b", 80.), ("c /_", 90.), ("d", 100.)],
  );
  check(
    "removed thumb absent",
    Array.length(query_all(document, "#s-item-a")) == 0,
  );
  check(
    "safe unique ids",
    Array.length(query_all(document, "[role=slider]")) == 3,
  );
  reset(
    ~items=[|
      {
        ...single[0],
        label: S.Labelled_by(["label"]),
      },
    |],
    (),
  );
  check("idref", attr("#s-item-a", "aria-labelledby") == Some("label"));
  app.dispatch(Disable(true));
  frame();
  check(
    "disabled semantics",
    attr("#s-item-a", "aria-disabled") == Some("true")
    && attr("#s-item-a", "tabindex") == Some("-1")
    && attr("#s-item-a", "data-disabled") == Some(""),
  );
  ignore(key("#s-item-a", "keydown", "Home"));
  frame();
  check("disabled key ignored", value("a") == 40. && count() == 0);
  check("disabled input unsuccessful", get(node("input"), "disabled"));
  ignore(pointer_at(node("#s-track"), "pointerdown", 50., 0.));
  frame();
  check("disabled no listeners", listener_count() == 0);
  reset(
    ~items=[|
      {
        ...single[0],
        disabled: true,
      },
    |],
    (),
  );
  ignore(pointer_at(node("#s-track"), "pointerdown", 50., 0.));
  frame();
  check("all disabled inert", listener_count() == 0 && count() == 0);
  reset(~items=[||], ());
  check(
    "empty collection",
    S.values(~items=[||], app.model().slider) == []
    && Array.length(query_all(document, "[role=slider]")) == 0,
  );
  reset();
  /* Explicit layout fixture: jsdom has no layout. Restore before shutdown. */
  let track = node("#s-track");
  let previous_rect = get(track, "getBoundingClientRect");
  let offset = ref(0.);
  put(track, "getBoundingClientRect", () =>
    {
      "x": 0.,
      "y": 0.,
      "left": offset^,
      "top": 0.,
      "right": 100.,
      "bottom": 100.,
      "width": 100.,
      "height": 100.,
    }
  );
  ignore(pointer_at(~button=2, track, "pointerdown", 10., 0.));
  check("right pointer ignored", listener_count() == 0);
  focus(node("#before"));
  ignore(pointer_at(track, "pointerdown", 60., 0.));
  check(
    "listeners installed before rect",
    listener_count_for("document", "pointermove", true) == 1,
  );
  ignore(pointer_at(document, "pointerup", 70., 0.));
  frame();
  frame();
  check(
    "release before measurement",
    value("a") == 70. && count() == 2 && listener_count() == 0,
  );
  check("track focuses thumb", has_focus("#s-item-a"));
  let before_rapid = count();
  ignore(pointer_at(track, "pointerdown", 10., 0.));
  ignore(pointer_at(document, "pointerup", 15., 0.));
  ignore(pointer_at(track, "pointerdown", 20., 0.));
  ignore(pointer_at(document, "pointerup", 25., 0.));
  frame();
  frame();
  check(
    "rapid pointer gestures both commit",
    value("a") == 25. && count() == before_rapid + 4 && listener_count() == 0,
  );
  focus(node("#before"));
  ignore(pointer_at(track, "pointerdown", 30., 0.));
  frame();
  frame();
  check(
    "drag state",
    attr("#s-item-a", "data-dragging") == Some("") && value("a") == 30.,
  );
  let before = count();
  ignore(pointer_at(~id=9, document, "pointermove", 90., 0.));
  frame();
  check("other pointer ignored", value("a") == 30.);
  ignore(pointer_at(document, "pointermove", 31., 0.));
  ignore(pointer_at(document, "pointermove", 32., 0.));
  ignore(pointer_at(document, "pointermove", 33., 0.));
  check("movement coalesces", count() == before);
  frame();
  frame();
  check("latest movement once", value("a") == 33. && count() == before + 1);
  ignore(pointer_at(document, "pointermove", 34., 0.));
  ignore(pointer_at(document, "pointerup", 90., 0.));
  frame();
  check(
    "release wins pending move",
    value("a") == 90. && count() == before + 3 && listener_count() == 0,
  );
  check("drag state removed", attr("#s-item-a", "data-dragging") == None);
  ignore(pointer_at(track, "pointerdown", 90., 0.));
  frame();
  let before = count();
  ignore(pointer_at(document, "pointerup", 90., 0.));
  frame();
  check("no-op gesture no output", count() == before);
  ignore(pointer_at(track, "pointerdown", 25., 0.));
  frame();
  let before = count();
  ignore(pointer_at(document, "pointercancel", 30., 0.));
  frame();
  check(
    "cancel commits current",
    value("a") == 25. && count() == before + 1 && listener_count() == 0,
  );
  ignore(pointer_at(track, "pointerdown", 20., 0.));
  frame();
  ignore(dispatch(window, event("blur", { "bubbles": false })));
  frame();
  check("blur cleanup", listener_count() == 0);
  ignore(pointer_at(track, "pointerdown", 50., 0.));
  frame();
  offset := 10.;
  ignore(dispatch(document, event("scroll", { "bubbles": false })));
  frame();
  frame();
  check("scroll remeasures track", value("a") == 40.);
  offset := 20.;
  ignore(dispatch(window, event("resize", { "bubbles": false })));
  frame();
  frame();
  check("resize remeasures track", value("a") == 30.);
  ignore(pointer_at(document, "pointerup", 50., 0.));
  frame();
  offset := 0.;
  ignore(pointer_at(track, "pointerdown", 50., 0.));
  frame();
  app.dispatch(Values([("a", 15.)]));
  frame();
  check(
    "setter during drag cancels silently",
    value("a") == 15. && listener_count() == 0,
  );
  reset(~items=[|thumb("a", 50.), thumb("b", 50.)|], ());
  ignore(pointer_at(track, "pointerdown", 75., 0.));
  frame();
  frame();
  ignore(pointer_at(document, "pointerup", 75., 0.));
  frame();
  check(
    "coincident tie moves upper",
    value("a") == 50. && value("b") == 75. && has_focus("#s-item-b"),
  );
  ignore(pointer_at(track, "pointerdown", 25., 0.));
  frame();
  frame();
  ignore(pointer_at(document, "pointerup", 25., 0.));
  frame();
  check("nearest lower", value("a") == 25. && has_focus("#s-item-a"));
  reset(
    ~items=[|
      {
        ...thumb("a", 20.),
        disabled: true,
      },
      thumb("b", 80.),
    |],
    (),
  );
  ignore(pointer_at(track, "pointerdown", 20., 0.));
  frame();
  frame();
  ignore(pointer_at(document, "pointerup", 20., 0.));
  frame();
  check(
    "nearest skips disabled thumb",
    value("a") == 20. && value("b") == 20. && has_focus("#s-item-b"),
  );
  reset(~orientation=S.Vertical, ());
  ignore(pointer_at(track, "pointerdown", 0., 25.));
  frame();
  frame();
  ignore(pointer_at(document, "pointerup", 0., 25.));
  frame();
  check("vertical geometry", value("a") == 75.);
  check(
    "fill custom geometry",
    get(get(node("#s-track > div"), "style"), "cssText") != "",
  );
  reset(~dir=S.Rtl, ());
  ignore(pointer_at(track, "pointerdown", 20., 0.));
  frame();
  frame();
  ignore(pointer_at(document, "pointerup", 20., 0.));
  frame();
  check("RTL geometry", value("a") == 80.);
  reset();
  remove_attribute(track, "id");
  ignore(pointer_at(track, "pointerdown", 30., 0.));
  frame();
  frame();
  check(
    "missing track aborts safely",
    value("a") == 40. && count() == 0 && listener_count() == 0,
  );
  set_attribute(track, "id", "s-track");
  let layout = get(track, "getBoundingClientRect");
  put(track, "getBoundingClientRect", () =>
    {
      "x": 0.,
      "y": 0.,
      "left": 0.,
      "top": 0.,
      "right": 0.,
      "bottom": 0.,
      "width": 0.,
      "height": 0.,
    }
  );
  ignore(pointer_at(track, "pointerdown", 30., 0.));
  frame();
  frame();
  ignore(pointer_at(document, "pointerup", 30., 0.));
  frame();
  check("zero track has deterministic finite value", value("a") == 0.);
  put(track, "getBoundingClientRect", layout);
  reset();
  ignore(pointer_at(track, "pointerdown", 30., 0.));
  frame();
  check(
    "element focus blur does not terminate drag",
    attr("#s-item-a", "data-dragging") == Some(""),
  );
  app.dispatch(Disable(true));
  frame();
  check("disable during drag cleanup", listener_count() == 0);
  reset();
  ignore(pointer_at(track, "pointerdown", 30., 0.));
  frame();
  app.dispatch(Replace(app.model().slider, [||]));
  frame();
  ignore(pointer_at(document, "pointermove", 80., 0.));
  frame();
  check("removal between messages cleans drag", listener_count() == 0);
  reset();
  ignore(pointer_at(track, "pointerdown", 30., 0.));
  frame();
  ignore(pointer_at(document, "pointermove", 80., 0.));
  put(track, "getBoundingClientRect", previous_rect);
  app.shutdown();
  frame();
  frame();
  check("shutdown no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("shutdown no global listeners", listener_count() == 0);
  report("slider");
  Js.Promise.resolve();
};
