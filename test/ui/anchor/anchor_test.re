open Tea;
open Test_support;
module A = Ui.Anchor;
let near = (label, actual, expected) =>
  check(label, abs_float(actual -. expected) < 0.00001);
let sides = [A.Top, A.Bottom, A.Left, A.Right];
let alignments = [A.Start, A.Center, A.End];
let pure = () => {
  let viewports: list(A.rect) = [
    {
      x: 0.,
      y: 0.,
      width: 500.,
      height: 400.,
    },
    {
      x: 50.,
      y: (-20.),
      width: 130.,
      height: 80.,
    },
    {
      x: 0.,
      y: 0.,
      width: 3.,
      height: 2.,
    },
    {
      x: 0.,
      y: 0.,
      width: 0.,
      height: 0.,
    },
  ];
  let anchors: list(A.rect) = [
    {
      x: 200.,
      y: 150.,
      width: 80.,
      height: 30.,
    },
    {
      x: (-20.),
      y: (-10.),
      width: 80.,
      height: 30.,
    },
    {
      x: 470.,
      y: 380.,
      width: 80.,
      height: 30.,
    },
    {
      x: 600.,
      y: 500.,
      width: 80.,
      height: 30.,
    },
    {
      x: 70.,
      y: 20.,
      width: 0.,
      height: 0.,
    },
  ];
  List.iter(
    viewport =>
      List.iter(
        anchor =>
          List.iter(
            preferred =>
              List.iter(
                alignment =>
                  List.iter(
                    flip =>
                      List.iter(
                        shift_padding =>
                          List.iter(
                            dir =>
                              List.iter(
                                match_width => {
                                  let floating: A.size = {
                                    width: 100.,
                                    height: 60.,
                                  };
                                  let p =
                                    A.compute(
                                      ~anchor,
                                      ~floating,
                                      ~viewport,
                                      ~placement=(preferred, alignment),
                                      ~flip,
                                      ~shift_padding,
                                      ~dir,
                                      ~match_width,
                                      ~offset=7.,
                                      ~arrow_size=10.,
                                      (),
                                    );
                                  let pad =
                                    Option.value(~default=0., shift_padding);
                                  let px = min(pad, viewport.width /. 2.);
                                  let py = min(pad, viewport.height /. 2.);
                                  let available = side => {
                                    let (origin, end_, edge) =
                                      switch (side) {
                                      | A.Top => (
                                          viewport.y +. py,
                                          viewport.y +. viewport.height -. py,
                                          anchor.y -. 7.,
                                        )
                                      | A.Bottom => (
                                          viewport.y +. py,
                                          viewport.y +. viewport.height -. py,
                                          anchor.y +. anchor.height +. 7.,
                                        )
                                      | A.Left => (
                                          viewport.x +. px,
                                          viewport.x +. viewport.width -. px,
                                          anchor.x -. 7.,
                                        )
                                      | A.Right => (
                                          viewport.x +. px,
                                          viewport.x +. viewport.width -. px,
                                          anchor.x +. anchor.width +. 7.,
                                        )
                                      };
                                    switch (side) {
                                    | A.Top
                                    | A.Left =>
                                      max(0., min(end_, edge) -. origin)
                                    | _ => max(0., end_ -. max(origin, edge))
                                    };
                                  };
                                  let opposite =
                                    switch (preferred) {
                                    | A.Top => A.Bottom
                                    | A.Bottom => A.Top
                                    | A.Left => A.Right
                                    | A.Right => A.Left
                                    };
                                  let width =
                                    match_width
                                      ? anchor.width : floating.width;
                                  let required =
                                    switch (preferred) {
                                    | A.Top
                                    | A.Bottom => floating.height
                                    | _ => width
                                    };
                                  let expected_side =
                                    flip
                                    && available(preferred) < required
                                    && available(opposite)
                                    > available(preferred)
                                      ? opposite : preferred;
                                  let (side, actual_alignment) = p.placement;
                                  check(
                                    "matrix resolved placement",
                                    side == expected_side
                                    && alignment == actual_alignment,
                                  );
                                  let is_vertical =
                                    switch (side) {
                                    | A.Top
                                    | A.Bottom => true
                                    | _ => false
                                    };
                                  let logical_start =
                                    switch (alignment, dir) {
                                    | (A.Start, A.Rtl) when is_vertical => A.End
                                    | (A.End, A.Rtl) when is_vertical => A.Start
                                    | _ => alignment
                                    };
                                  let anchor_start =
                                    is_vertical ? anchor.x : anchor.y;
                                  let anchor_length =
                                    is_vertical ? anchor.width : anchor.height;
                                  let panel_length =
                                    is_vertical ? width : floating.height;
                                  let cross =
                                    switch (logical_start) {
                                    | A.Start => anchor_start
                                    | A.Center =>
                                      anchor_start
                                      +. anchor_length
                                      /. 2.
                                      -. panel_length
                                      /. 2.
                                    | A.End =>
                                      anchor_start
                                      +. anchor_length
                                      -. panel_length
                                    };
                                  let origin =
                                    is_vertical
                                      ? viewport.x +. px : viewport.y +. py;
                                  let end_ =
                                    is_vertical
                                      ? viewport.x +. viewport.width -. px
                                      : viewport.y +. viewport.height -. py;
                                  let cross =
                                    Option.is_none(shift_padding)
                                      ? cross
                                      : max(
                                          origin,
                                          min(
                                            max(origin, end_ -. panel_length),
                                            cross,
                                          ),
                                        );
                                  near(
                                    "matrix cross alignment/shift/RTL",
                                    is_vertical ? p.x : p.y,
                                    cross,
                                  );
                                  let main =
                                    switch (side) {
                                    | A.Top =>
                                      anchor.y -. 7. -. floating.height
                                    | A.Bottom =>
                                      anchor.y +. anchor.height +. 7.
                                    | A.Left => anchor.x -. 7. -. width
                                    | A.Right => anchor.x +. anchor.width +. 7.
                                    };
                                  near(
                                    "matrix main position",
                                    is_vertical ? p.y : p.x,
                                    main,
                                  );
                                  near("matrix width", p.width, width);
                                  near(
                                    "matrix height",
                                    p.height,
                                    floating.height,
                                  );
                                  near(
                                    "matrix available width",
                                    p.available_width,
                                    is_vertical
                                      ? viewport.width -. 2. *. px
                                      : available(side),
                                  );
                                  near(
                                    "matrix available height",
                                    p.available_height,
                                    is_vertical
                                      ? available(side)
                                      : viewport.height -. 2. *. py,
                                  );
                                  let hidden =
                                    anchor.width == 0.
                                    || anchor.height == 0.
                                    || viewport.width == 0.
                                    || viewport.height == 0.
                                    || anchor.x >= viewport.x
                                    +. viewport.width
                                    || anchor.x
                                    +. anchor.width <= viewport.x
                                    || anchor.y >= viewport.y
                                    +. viewport.height
                                    || anchor.y
                                    +. anchor.height <= viewport.y;
                                  check(
                                    "matrix hidden reason",
                                    p.reference_hidden == hidden
                                    && Option.is_some(p.hidden_reason)
                                    == hidden,
                                  );
                                  near(
                                    "matrix arrow offset",
                                    Option.get(p.arrow_offset),
                                    max(
                                      0.,
                                      min(
                                        max(0., panel_length -. 10.),
                                        anchor_start
                                        +. anchor_length
                                        /. 2.
                                        -. cross
                                        -. 5.,
                                      ),
                                    ),
                                  );
                                },
                                [false, true],
                              ),
                            [A.Ltr, A.Rtl],
                          ),
                        [None, Some(0.), Some(8.), Some(100.)],
                      ),
                    [false, true],
                  ),
                alignments,
              ),
            sides,
          ),
        anchors,
      ),
    viewports,
  );
  let anchor: A.rect = {
    x: 100.,
    y: 100.,
    width: 80.,
    height: 20.,
  };
  let viewport: A.rect = {
    x: 0.,
    y: 0.,
    width: 280.,
    height: 220.,
  };
  let p =
    A.compute(
      ~anchor,
      ~floating={
        width: 300.,
        height: 300.,
      },
      ~viewport,
      ~placement=(A.Top, A.Center),
      ~shift_padding=None,
      (),
    );
  check(
    "neither fits, tied sides retain preferred",
    fst(p.placement) == A.Top,
  );
  check("arrow omitted by default", Option.is_none(p.arrow_offset));
  let p =
    A.compute(
      ~anchor,
      ~floating={
        width: 1.,
        height: 1.,
      },
      ~viewport,
      ~placement=(A.Bottom, A.Start),
      ~offset=-5.,
      (),
    );
  near("negative offset overlaps", p.y, 115.);
  let p =
    A.compute(
      ~anchor={
        x: nan,
        y: infinity,
        width: (-1.),
        height: infinity,
      },
      ~floating={
        width: nan,
        height: (-1.),
      },
      ~viewport,
      ~placement=(A.Bottom, A.Start),
      ~offset=nan,
      ~shift_padding=Some(nan),
      (),
    );
  check(
    "nonfinite inputs never escape to styles",
    p.x == 0.
    && p.y == 0.
    && p.width == 0.
    && p.height == 0.
    && p.hidden_reason == Some(A.Reference_empty),
  );
  List.iter(
    side =>
      List.iter(
        alignment =>
          check(
            "placement names include alignment",
            String.contains(A.placement_name((side, alignment)), '-'),
          ),
        alignments,
      ),
    sides,
  );
};
type resize_fixture;
external install_resize: unit => resize_fixture =
  "teaInstallResizeObserverMock";
[@mel.send] external notify: (resize_fixture, float, float) => unit = "notify";
[@mel.send] external count: resize_fixture => int = "count";
[@mel.send] external disconnected: resize_fixture => int = "disconnected";
[@mel.send] external restore: resize_fixture => unit = "restore";
[@mel.send]
external style_value: (Dom.cssStyleDeclaration, string) => string =
  "getPropertyValue";
let style = (selector, name) =>
  style_value(get(node(selector), "style"), name);
let fake_rect = (element, r: A.rect) =>
  put(element, "getBoundingClientRect", () =>
    {
      "x": r.x,
      "y": r.y,
      "left": r.x,
      "top": r.y,
      "right": r.x +. r.width,
      "bottom": r.y +. r.height,
      "width": r.width,
      "height": r.height,
    }
  );
type msg =
  | Anchor(A.msg)
  | Start
  | Stop
  | Refresh
  | Match(bool)
  | Top_layer
  | Missing(bool)
  | Remove_floating(bool);
type model = {
  anchor: A.model,
  changes: int,
  missing: bool,
  no_floating: bool,
  unavailable: int,
};
let update = (msg, model) => {
  let (anchor, cmd, events) =
    switch (msg) {
    | Anchor(msg) => A.update(msg, model.anchor)
    | _ =>
      let (anchor, cmd) =
        switch (msg) {
        | Start => A.start(model.anchor)
        | Stop => A.stop(model.anchor)
        | Match(match_width) => A.configure(~match_width, model.anchor)
        | Top_layer => A.configure(~top_layer=true, model.anchor)
        | _ => A.refresh(model.anchor)
        };
      (anchor, cmd, []);
    };
  (
    {
      anchor,
      changes: model.changes + List.length(events),
      missing:
        switch (msg) {
        | Missing(value) => value
        | _ => model.missing
        },
      no_floating:
        switch (msg) {
        | Remove_floating(value) => value
        | _ => model.no_floating
        },
      unavailable:
        model.unavailable
        + List.fold_left(
            (n, event) =>
              switch (event) {
              | A.Unavailable => n + 1
              | _ => n
              },
            0,
            events,
          ),
    },
    Cmd.map(msg => Anchor(msg), cmd),
  );
};
let view = model =>
  Html.div(
    [],
    [
      model.missing
        ? Html.text("")
        : Ui.Button.button(
            ~id="reference",
            ~on_press=Start,
            [Html.text("Reference")],
          ),
      model.no_floating
        ? Html.text("")
        : A.view(
            ~model=model.anchor,
            ~to_parent=msg => Anchor(msg),
            ~to_view=
              parts => Html.div(parts.floating, [Html.text("Panel")]),
            (),
          ),
    ],
  );
let scroll = () =>
  ignore(dispatch(node("#app"), event("scroll", { "bubbles": false })));
let resize = () =>
  ignore(dispatch(window, event("resize", { "bubbles": false })));
let runtime = () => {
  let resize_fixture = install_resize();
  let ratio = get(window, "devicePixelRatio");
  put(window, "devicePixelRatio", 2.);
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              anchor:
                A.init(
                  ~anchor_id="reference",
                  ~floating_id="panel",
                  ~offset=4.,
                  (),
                ),
              changes: 0,
              missing: false,
              no_floating: false,
              unavailable: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model => Sub.map(msg => Anchor(msg), A.subscriptions(model.anchor)),
      ~node="#app",
      ~flags=(),
      (),
    );
  check(
    "inactive hidden and no sources",
    get(node("#panel"), "hidden")
    && listener_count() == 0
    && count(resize_fixture) == 0,
  );
  app.dispatch(Start);
  check(
    "active view hidden before first measurement, no flash",
    List.exists(
      fact =>
        switch (fact) {
        | Tea_vdom.Vdom.Style("visibility", "hidden") => true
        | _ => false
        },
      A.attrs(app.model().anchor),
    )
    && Option.is_none(A.position(app.model().anchor)),
  );
  fake_rect(
    node("#reference"),
    {
      x: 30.3,
      y: 40.3,
      width: 100.,
      height: 20.,
    },
  );
  fake_rect(
    node("#panel"),
    {
      x: 0.,
      y: 0.,
      width: 150.,
      height: 50.,
    },
  );
  frame();
  check("one measured result", app.model().changes == 1);
  check(
    "floating visible, no roles invented",
    String.equal(style("#panel", "visibility"), "visible")
    && attr("#panel", "role") == None
    && attr("#panel", "aria-hidden") == None,
  );
  check(
    "fixed positioning and rounding",
    String.equal(style("#panel", "position"), "fixed")
    && String.equal(style("#panel", "left"), "30.5px")
    && String.equal(style("#panel", "top"), "64.5px"),
  );
  check(
    "data placement and custom properties",
    attr("#panel", "data-placement") == Some("bottom-start")
    && String.equal(style("#panel", "--tea-anchor-width"), "100px")
    && String.equal(style("#panel", "--tea-anchor-height"), "20px")
    && String.equal(style("#panel", "--tea-available-width"), "1008px")
    && String.equal(style("#panel", "--tea-available-height"), "695.7px"),
  );
  check(
    "all browser sources installed once",
    listener_count_for("document", "scroll", true) == 1
    && listener_count_for("window", "resize", false) == 1
    && count(resize_fixture) == 2,
  );
  focus(node("#reference"));
  List.iter(
    key_ =>
      check("no keyboard interception", key("#reference", "keydown", key_)),
    ["Tab", "Escape", "ArrowDown", " "],
  );
  check("no focus movement", has_focus("#reference"));
  let before = app.model().changes;
  for (_ in 1 to 100) {
    scroll();
    resize();
    notify(resize_fixture, 1., 1.);
  };
  frame();
  check("unchanged sources never dispatch", app.model().changes == before);
  fake_rect(
    node("#reference"),
    {
      x: 50.,
      y: 60.,
      width: 120.,
      height: 30.,
    },
  );
  for (_ in 1 to 100) {
    scroll();
    resize();
    notify(resize_fixture, 2., 2.);
  };
  check("burst waits for frame", app.model().changes == before);
  flush();
  check(
    "burst sends exactly one result in frame",
    app.model().changes == before + 1,
  );
  flush();
  check(
    "burst has no trailing message",
    app.model().changes == before
    + 1
    && String.equal(style("#panel", "top"), "94px"),
  );
  fake_rect(
    node("#panel"),
    {
      x: 50.,
      y: 94.,
      width: 190.,
      height: 80.,
    },
  );
  notify(resize_fixture, 190., 80.);
  frame();
  near(
    "floating resize remeasures",
    Option.get(A.position(app.model().anchor)).height,
    80.,
  );
  app.dispatch(Match(true));
  frame();
  frame();
  check(
    "match width sets border-box width",
    String.equal(style("#panel", "width"), "120px"),
  );
  app.dispatch(Match(false));
  frame();
  frame();
  check(
    "turning match off clears inline width",
    String.equal(style("#panel", "width"), ""),
  );
  fake_rect(
    node("#reference"),
    {
      x: (-200.),
      y: 0.,
      width: 100.,
      height: 20.,
    },
  );
  scroll();
  frame();
  check(
    "offscreen reference marked and hidden",
    attr("#panel", "data-reference-hidden") == Some("")
    && String.equal(style("#panel", "visibility"), "hidden"),
  );
  fake_rect(
    node("#reference"),
    {
      x: (-50.),
      y: 0.,
      width: 100.,
      height: 20.,
    },
  );
  resize();
  frame();
  check(
    "partially visible reference restored",
    attr("#panel", "data-reference-hidden") == None
    && String.equal(style("#panel", "visibility"), "visible"),
  );
  let before = app.model().changes;
  app.dispatch(Start);
  frame();
  check("start idempotent", app.model().changes == before);
  fake_rect(
    node("#reference"),
    {
      x: 200.,
      y: 200.,
      width: 100.,
      height: 20.,
    },
  );
  scroll();
  app.dispatch(Stop);
  check(
    "stop immediately disposes listeners and observers",
    listener_count() == 0 && count(resize_fixture) == 0,
  );
  frame();
  check(
    "stale measurement ignored after stop",
    app.model().changes == before
    && Option.is_none(A.position(app.model().anchor))
    && get(node("#panel"), "hidden"),
  );
  app.dispatch(Stop);
  app.dispatch(Start);
  app.dispatch(Stop);
  app.dispatch(Start);
  frame();
  frame();
  check(
    "rapid lifecycle has only live sources",
    listener_count() == 2
    && count(resize_fixture) == 2
    && app.model().changes == before
    + 1,
  );
  app.dispatch(Missing(true));
  frame();
  frame();
  check(
    "missing reference reports once",
    app.model().unavailable == 1
    && Option.is_none(A.position(app.model().anchor))
    && String.equal(style("#panel", "visibility"), "hidden"),
  );
  resize();
  scroll();
  frame();
  check("missing does not storm", app.model().unavailable == 1);
  app.dispatch(Missing(false));
  flush();
  fake_rect(
    node("#reference"),
    {
      x: 70.,
      y: 80.,
      width: 110.,
      height: 22.,
    },
  );
  frame();
  check(
    "replacement reference observed and measured",
    count(resize_fixture) == 2
    && Option.is_some(A.position(app.model().anchor)),
  );
  app.dispatch(Remove_floating(true));
  frame();
  frame();
  check("missing floating reports unavailable", app.model().unavailable == 2);
  app.dispatch(Remove_floating(false));
  flush();
  fake_rect(
    node("#panel"),
    {
      x: 0.,
      y: 0.,
      width: 150.,
      height: 50.,
    },
  );
  frame();
  check(
    "replacement floating recovered",
    Option.is_some(A.position(app.model().anchor)),
  );
  /* jsdom has no Popover API. Explicit fixture verifies ownership/cleanup;
     real native top-layer geometry and clipping are tested in Chromium. */
  let panel = node("#panel");
  let opened = ref(false);
  let shows = ref(0);
  let hides = ref(0);
  put(panel, "matches", selector =>
    String.equal(selector, ":popover-open") && opened^
  );
  put(
    panel,
    "showPopover",
    () => {
      opened := true;
      incr(shows);
    },
  );
  put(
    panel,
    "hidePopover",
    () => {
      opened := false;
      incr(hides);
    },
  );
  app.dispatch(Top_layer);
  frame();
  frame();
  check(
    "manual top-layer opt-in opens once",
    opened^ && shows^ == 1 && attr("#panel", "popover") == Some("manual"),
  );
  scroll();
  frame();
  check("measurement does not reopen popover", shows^ == 1);
  app.dispatch(Stop);
  frame();
  check("stop closes top layer", ! opened^ && hides^ == 1);
  app.dispatch(Start);
  frame();
  frame();
  check("restart reopens top layer", opened^ && shows^ == 2);
  app.dispatch(Stop);
  frame();
  put(
    panel,
    "showPopover",
    () => {
      /* Native beforetoggle runs before the browser actually opens the panel. */
      app.dispatch(Stop);
      opened := true;
      incr(shows);
    },
  );
  app.dispatch(Start);
  frame();
  frame();
  check(
    "synchronous stop during show cannot leak a top-layer panel",
    ! opened^
    && !A.active(app.model().anchor)
    && listener_count() == 0
    && count(resize_fixture) == 0,
  );
  put(
    panel,
    "showPopover",
    () => {
      opened := true;
      incr(shows);
    },
  );
  app.dispatch(Start);
  frame();
  frame();
  check(
    "restart after synchronous stop is independent",
    opened^ && shows^ == 4,
  );
  scroll();
  let hides_before_shutdown = hides^;
  let before = app.model().changes;
  app.shutdown();
  frame();
  frame();
  check(
    "shutdown hides top layer and cancels stale work",
    ! opened^
    && hides^ == hides_before_shutdown
    + 1
    && app.model().changes == before,
  );
  check(
    "shutdown leaves no observers/listeners",
    count(resize_fixture) == 0
    && listener_count() == 0
    && Tea_dom.Debug.active_listeners() == 0
    && disconnected(resize_fixture) >= 2,
  );
  restore(resize_fixture);
  put(window, "devicePixelRatio", ratio);
};
let fallback = () => {
  let app =
    App.element(
      ~init=
        () => {
          let (anchor, cmd) =
            A.start(
              A.init(
                ~anchor_id="reference",
                ~floating_id="panel",
                ~top_layer=true,
                (),
              ),
            );
          (
            {
              anchor,
              changes: 0,
              missing: false,
              no_floating: false,
              unavailable: 0,
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
  fake_rect(
    node("#reference"),
    {
      x: 40.,
      y: 50.,
      width: 100.,
      height: 20.,
    },
  );
  fake_rect(
    node("#panel"),
    {
      x: 0.,
      y: 0.,
      width: 150.,
      height: 40.,
    },
  );
  frame();
  frame();
  check(
    "unsupported ResizeObserver and Popover API retain initial fixed placement",
    app.model().changes == 1
    && String.equal(style("#panel", "position"), "fixed")
    && String.equal(style("#panel", "visibility"), "visible"),
  );
  fake_rect(
    node("#reference"),
    {
      x: 60.,
      y: 70.,
      width: 100.,
      height: 20.,
    },
  );
  resize();
  frame();
  check(
    "fallback remeasures on window resize",
    String.equal(style("#panel", "left"), "60px")
    && String.equal(style("#panel", "top"), "90px"),
  );
  app.dispatch(Refresh);
  frame();
  frame();
  check(
    "explicit refresh works without element observers",
    app.model().changes == 3,
  );
  app.shutdown();
  frame();
  check(
    "fallback shutdown cleans up",
    listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
  );
};
let completed = {
  pure();
  runtime();
  fallback();
  report("anchor");
  Js.Promise.resolve();
};
