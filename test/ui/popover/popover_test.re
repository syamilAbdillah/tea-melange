open Tea;
open Test_support;
module P = Ui.Popover;
module V = Tea_vdom.Vdom;
[@mel.send] external blur: Dom.node => unit = "blur";
type msg =
  | Progress(P.msg)
  | Open
  | Close
  | Remove
  | Hook;
type model = {
  popover: P.model,
  outs: list(P.out_msg),
  removed: bool,
  hooks: int,
};
[@mel.send]
external listen: (Dom.document, string, Dom.event => unit) => unit =
  "addEventListener";
[@mel.send]
external unlisten: (Dom.document, string, Dom.event => unit) => unit =
  "removeEventListener";
let escape_stopped = selector => {
  let reached = ref(false);
  let spy = _ => reached := true;
  listen(document, "keydown", spy);
  let e =
    keyboard_event(
      "keydown",
      {
        "key": "Escape",
        "bubbles": true,
        "cancelable": true,
      },
    );
  ignore(dispatch(node(selector), e));
  unlisten(document, "keydown", spy);
  ! reached^;
};
let settle = () =>
  for (_i in 1 to 12) {
    flush();
  };
let start =
    (
      ~role: P.role=P.None,
      ~focus_on_open=P.None,
      ~close_on_focus_out=true,
      ~group_triggers=[],
      ~unmount=false,
      ~top_layer=false,
      ~advanced_focus=false,
      (),
    ) =>
  App.element(
    ~init=
      () =>
        (
          {
            popover:
              P.init(
                ~id="test",
                ~role,
                ~focus_on_open,
                ~close_on_focus_out,
                ~group_triggers,
                ~top_layer,
                (),
              ),
            outs: [],
            removed: false,
            hooks: 0,
          },
          Cmd.none,
        ),
    ~update=
      (message, model) => {
        let (popover, command, outs) =
          switch (message) {
          | Progress(message) => P.update(message, model.popover)
          | Open =>
            let (popover, command) = P.open_(model.popover);
            (popover, command, []);
          | Close =>
            let (popover, command) = P.close(model.popover);
            (popover, command, []);
          | Remove
          | Hook => (model.popover, Cmd.none, [])
          };
        (
          {
            popover,
            outs: model.outs @ outs,
            hooks:
              model.hooks
              + (
                switch (message) {
                | Hook => 1
                | _ => 0
                }
              ),
            removed:
              model.removed
              || (
                switch (message) {
                | Remove => true
                | _ => false
                }
              ),
          },
          Cmd.map(message => Progress(message), command),
        );
      },
    ~view=
      model =>
        Html.div(
          [],
          [
            !model.removed
              ? P.view(
                  ~model=model.popover,
                  ~to_parent=message => Progress(message),
                  ~on_trigger_key_down=
                    key => String.equal(key, "Home") ? Some(Hook) : None,
                  ~on_panel_key_down=
                    key => String.equal(key, "End") ? Some(Hook) : None,
                  ~to_view=
                    parts => {
                      /* Reject duplicate facts before the VDOM normalizer can mask them. */
                      check(
                        "exactly one panel id",
                        List.length(
                          List.filter(
                            fact =>
                              switch (fact) {
                              | V.Attribute("id", _) => true
                              | _ => false
                              },
                            parts.panel,
                          ),
                        )
                        == 1,
                      );
                      check(
                        "exactly one hidden property",
                        List.length(
                          List.filter(
                            fact =>
                              switch (fact) {
                              | V.Property("hidden", _) => true
                              | _ => false
                              },
                            parts.panel,
                          ),
                        )
                        == 1,
                      );
                      Html.div(
                        [],
                        [
                          Html.button(parts.trigger, [Html.text("Filters")]),
                          !unmount || parts.rendered
                            ? Html.div(
                                parts.panel,
                                [
                                  Html.input(
                                    [
                                      Html.id("disabled"),
                                      Html.disabled(true),
                                    ],
                                    [],
                                  ),
                                  Html.input(
                                    [
                                      Html.id("negative"),
                                      Html.tabindex(-1),
                                    ],
                                    [],
                                  ),
                                  Html.div(
                                    [Html.hidden(true)],
                                    [
                                      Html.input(
                                        [Html.id("hidden-field")],
                                        [],
                                      ),
                                    ],
                                  ),
                                  Html.div(
                                    [Html.attribute("inert", "")],
                                    [
                                      Html.input(
                                        [Html.id("inert-field")],
                                        [],
                                      ),
                                    ],
                                  ),
                                  Html.details(
                                    [],
                                    [
                                      Html.input(
                                        [Html.id("details-field")],
                                        [],
                                      ),
                                    ],
                                  ),
                                  Html.input([Html.type_("hidden")], []),
                                  advanced_focus
                                    ? Html.div(
                                        [],
                                        [
                                          Html.input(
                                            [
                                              Html.id("radio-a"),
                                              Html.type_("radio"),
                                              Html.name("choice"),
                                            ],
                                            [],
                                          ),
                                          Html.input(
                                            [
                                              Html.id("radio-b"),
                                              Html.type_("radio"),
                                              Html.name("choice"),
                                              Html.checked(true),
                                            ],
                                            [],
                                          ),
                                        ],
                                      )
                                    : Html.text(""),
                                  Html.input([Html.id("first")], []),
                                  Html.input([Html.id("last")], []),
                                  Html.button(
                                    parts.close_button,
                                    [Html.text("Done")],
                                  ),
                                ],
                              )
                            : Html.text(""),
                        ],
                      );
                    },
                  (),
                )
              : Html.text(""),
            Html.button([Html.id("outside")], [Html.text("Outside")]),
            Html.button([Html.id("peer")], [Html.text("Peer")]),
            Html.div(
              [Html.id("nonfocusable")],
              [Html.text("Nonfocusable outside")],
            ),
          ],
        ),
    ~subscriptions=
      model =>
        model.removed
          ? Sub.none
          : Sub.map(
              message => Progress(message),
              P.subscriptions(model.popover),
            ),
    ~node="#app",
    ~flags=(),
    (),
  );
let layout = () => {
  put(node("#test-trigger"), "getBoundingClientRect", () =>
    {
      "x": 50.,
      "y": 50.,
      "top": 50.,
      "left": 50.,
      "right": 150.,
      "bottom": 80.,
      "width": 100.,
      "height": 30.,
    }
  );
  if (!Js.testAny(node("#test-panel"))) {
    put(node("#test-panel"), "getBoundingClientRect", () =>
      {
        "x": 50.,
        "y": 88.,
        "top": 88.,
        "left": 50.,
        "right": 250.,
        "bottom": 188.,
        "width": 200.,
        "height": 100.,
      }
    );
  };
};
let stop = app => {
  app.Tea_core.Runtime.shutdown();
  settle();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
};
let expanded = value =>
  check(
    "expanded",
    attr("#test-trigger", "aria-expanded") == Some(value ? "true" : "false"),
  );
let hidden = value =>
  check("hidden property", get(node("#test-panel"), "hidden") == value);
let latest = (app, out) => {
  check(
    "out reason",
    List.hd(List.rev(app.Tea_core.Runtime.model().outs)) == out,
  );
};
let completed = {
  let app = start(~role=P.Dialog("Filters"), ());
  layout();
  expanded(false);
  hidden(true);
  check(
    "ids derive from prefix",
    P.trigger_id(app.model().popover) == "test-trigger"
    && P.panel_id(app.model().popover) == "test-panel",
  );
  check(
    "closed has no controls",
    attr("#test-trigger", "aria-controls") == None,
  );
  check(
    "native semantics",
    attr("#test-trigger", "type") == Some("button")
    && attr("#test-trigger", "role") == None,
  );
  check(
    "dialog semantics",
    attr("#test-trigger", "aria-haspopup") == Some("dialog")
    && attr("#test-panel", "role") == Some("dialog")
    && attr("#test-panel", "aria-label") == Some("Filters")
    && attr("#test-panel", "aria-modal") == None,
  );
  check("plain positioning opt-out", attr("#test-panel", "popover") == None);
  focus(node("#test-trigger"));
  click("#test-trigger");
  settle();
  expanded(true);
  hidden(false);
  latest(app, P.Opened);
  check("open does not move focus", has_focus("#test-trigger"));
  check(
    "control idref resolves",
    attr("#test-trigger", "aria-controls") == Some("test-panel"),
  );
  check("position ready", Option.is_some(P.position(app.model().popover)));
  check(
    "trigger hook default preserved",
    key("#test-trigger", "keydown", "Home"),
  );
  check("panel hook default preserved", key("#first", "keydown", "End"));
  check("hooks mapped to parent", app.model().hooks == 2);
  check(
    "data flag presence",
    attr("#test-trigger", "data-open") == Some("")
    && attr("#test-panel", "data-open") == Some(""),
  );
  focus(node("#first"));
  focus(node("#test-trigger"));
  settle();
  check("focus within union remains open", P.is_open(app.model().popover));
  focus(node("#first"));
  List.iter(
    key_ => {
      check("unhandled key preserved", key("#first", "keydown", key_));
      check(
        "shift key preserved",
        key(~shift=true, "#first", "keydown", key_),
      );
    },
    ["Tab", "Enter", " ", "ArrowDown", "ArrowUp", "Home", "End"],
  );
  List.iter(
    selector => {
      check(
        "ctrl Escape untouched",
        key(~ctrl=true, selector, "keydown", "Escape"),
      );
      check(
        "alt Escape untouched",
        key(~alt=true, selector, "keydown", "Escape"),
      );
      check(
        "meta Escape untouched",
        key(~meta=true, selector, "keydown", "Escape"),
      );
      check(
        "IME Escape untouched",
        key(~composing=true, selector, "keydown", "Escape"),
      );
    },
    ["#first", "#test-trigger"],
  );
  expanded(true);
  check(
    "handled Escape stops propagation to enclosing overlays",
    escape_stopped("#first"),
  );
  flush();
  expanded(false);
  hidden(false);
  check(
    "leaving keeps controls",
    attr("#test-trigger", "aria-controls") == Some("test-panel"),
  );
  check(
    "leaving visible",
    P.phase(app.model().popover) == Ui.Animation.Leaving
    && attr("#test-panel", "data-leave") == Some("")
    && listener_count_for("document", "scroll", true) > 0,
  );
  latest(app, P.Closed(P.Escape));
  settle();
  hidden(true);
  check("Escape focus return", has_focus("#test-trigger"));
  check(
    "controls removed after Left",
    attr("#test-trigger", "aria-controls") == None,
  );
  check(
    "closed Escape untouched",
    key("#test-trigger", "keydown", "Escape")
    && !escape_stopped("#test-trigger"),
  );
  check(
    "Anchor stopped on Left",
    listener_count_for("document", "scroll", true) == 0,
  );
  click("#test-trigger");
  settle();
  focus(node("#first"));
  pointer("#outside");
  focus(node("#outside"));
  settle();
  latest(app, P.Closed(P.Outside));
  check("outside click leaves focus at destination", has_focus("#outside"));
  hidden(true);
  click("#test-trigger");
  settle();
  focus(node("#first"));
  pointer("#nonfocusable");
  settle();
  latest(app, P.Closed(P.Outside));
  check(
    "nonfocusable outside never returns focus to trigger",
    !has_focus("#test-trigger"),
  );
  click("#test-trigger");
  settle();
  focus(node("#first"));
  focus(node("#outside"));
  settle();
  latest(app, P.Closed(P.Focus_out));
  hidden(true);
  check("programmatic focus out not stolen", has_focus("#outside"));
  click("#test-trigger");
  settle();
  focus(node("#first"));
  blur(node("#first"));
  settle();
  latest(app, P.Closed(P.Focus_out));
  click("#test-trigger");
  settle();
  click("#test-trigger");
  settle();
  latest(app, P.Closed(P.Trigger));
  click("#test-trigger");
  settle();
  focus(node("#test-close"));
  click("#test-close");
  settle();
  latest(app, P.Closed(P.Programmatic));
  check("close helper returns focus", has_focus("#test-trigger"));
  let outs = app.model().outs;
  app.dispatch(Open);
  app.dispatch(Open);
  settle();
  check(
    "programmatic open is silent",
    app.model().outs == outs && P.is_open(app.model().popover),
  );
  app.dispatch(Close);
  app.dispatch(Close);
  settle();
  check(
    "programmatic close silent",
    app.model().outs == outs && !P.is_open(app.model().popover),
  );
  ignore(
    dispatch(
      node("#test-trigger"),
      event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
        },
      ),
    ),
  );
  ignore(key("#test-trigger", "keydown", "Escape"));
  settle();
  expanded(false);
  latest(app, P.Closed(P.Escape));
  /* Dispatch on the same rendered node without flushing to prove intent,
     rather than precomputed state, drives batched toggles. */
  for (_i in 1 to 101) {
    ignore(
      dispatch(
        node("#test-trigger"),
        event(
          "click",
          {
            "bubbles": true,
            "cancelable": true,
          },
        ),
      ),
    );
  };
  settle();
  expanded(true);
  for (_i in 1 to 100) {
    ignore(
      dispatch(
        node("#test-trigger"),
        event(
          "click",
          {
            "bubbles": true,
            "cancelable": true,
          },
        ),
      ),
    );
  };
  settle();
  expanded(true);
  app.dispatch(Close);
  check(
    "close enters Leaving",
    P.phase(app.model().popover) == Ui.Animation.Leaving,
  );
  app.dispatch(Open);
  settle();
  expanded(true);
  hidden(false);
  check("reverse leave", P.phase(app.model().popover) == Ui.Animation.Open);
  app.dispatch(Remove);
  settle();
  check("unmount cleans every source", listener_count() == 0);
  stop(app);
  let app = start(~role=P.Region, ~focus_on_open=P.First_tabbable, ());
  layout();
  check(
    "region labelled by trigger",
    attr("#test-panel", "role") == Some("region")
    && attr("#test-panel", "aria-labelledby") == Some("test-trigger")
    && attr("#test-trigger", "aria-haspopup") == None,
  );
  focus(node("#test-trigger"));
  click("#test-trigger");
  settle();
  check("focus first skips disabled and negative", has_focus("#first"));
  stop(app);
  let app = start(~focus_on_open=P.First_tabbable, ~advanced_focus=true, ());
  layout();
  click("#test-trigger");
  settle();
  check("checked radio is the group's Tab stop", has_focus("#radio-b"));
  stop(app);
  let app = start(~focus_on_open=P.First_tabbable, ());
  layout();
  set_attribute(node("#last"), "tabindex", "1");
  set_attribute(node("#first"), "tabindex", "2");
  click("#test-trigger");
  settle();
  check("positive tabindex order", has_focus("#last"));
  stop(app);
  let app = start(~focus_on_open=P.Element("last"), ());
  layout();
  check(
    "unsemantic role omitted",
    attr("#test-panel", "role") == None
    && attr("#test-trigger", "aria-haspopup") == None,
  );
  click("#test-trigger");
  settle();
  check("explicit initial focus", has_focus("#last"));
  stop(app);
  let app = start(~focus_on_open=P.Element("missing"), ());
  layout();
  focus(node("#test-trigger"));
  click("#test-trigger");
  settle();
  check(
    "missing focus target nonfatal",
    has_focus("#test-trigger") && P.is_open(app.model().popover),
  );
  stop(app);
  let app = start(~close_on_focus_out=false, ());
  layout();
  click("#test-trigger");
  settle();
  focus(node("#outside"));
  settle();
  check("opt out of focus out dismissal", P.is_open(app.model().popover));
  pointer("#outside");
  settle();
  check("opt out still light dismisses", !P.is_open(app.model().popover));
  stop(app);
  let app = start(~group_triggers=["peer"], ());
  layout();
  click("#test-trigger");
  settle();
  focus(node("#peer"));
  settle();
  check(
    "group trigger focus does not flicker",
    P.is_open(app.model().popover),
  );
  focus(node("#outside"));
  settle();
  latest(app, P.Closed(P.Focus_out));
  stop(app);
  let app = start(~unmount=true, ());
  check("unmounted initially", Js.testAny(node("#test-panel")));
  app.dispatch(Open);
  settle();
  check("mount on show", !Js.testAny(node("#test-panel")));
  app.dispatch(Close);
  settle();
  check("unmount after Left", Js.testAny(node("#test-panel")));
  stop(app);
  let rec await_position = (app, remaining) =>
    if (remaining > 0
        && Option.is_none(P.position(app.Tea_core.Runtime.model().popover))) {
      flush();
      await_position(app, remaining - 1);
    };
  let app = start(~focus_on_open=P.First_tabbable, ());
  layout();
  app.dispatch(Open);
  await_position(app, 20);
  check(
    "focus request awaits visible geometry",
    Option.is_some(P.position(app.model().popover)),
  );
  app.dispatch(Close);
  focus(node("#outside"));
  settle();
  check("closing cancels deferred opening focus", has_focus("#outside"));
  stop(app);
  let app = start(~focus_on_open=P.First_tabbable, ());
  layout();
  app.dispatch(Open);
  await_position(app, 20);
  app.dispatch(Remove);
  focus(node("#outside"));
  settle();
  check("unmount cancels deferred opening focus", has_focus("#outside"));
  stop(app);
  let app = start();
  layout();
  click("#test-trigger");
  settle();
  focus(node("#first"));
  ignore(key("#first", "keydown", "Escape"));
  focus(node("#outside"));
  settle();
  check(
    "Escape never steals subsequent intentional focus",
    has_focus("#outside"),
  );
  latest(app, P.Closed(P.Escape));
  click("#test-trigger");
  settle();
  focus(node("#first"));
  ignore(key("#first", "keydown", "Escape"));
  pointer("#nonfocusable");
  settle();
  check(
    "outside pointer cancels pending Escape focus",
    !has_focus("#test-trigger"),
  );
  latest(app, P.Closed(P.Escape));
  stop(app);
  let app = start();
  layout();
  let unfinished = Js.Promise.make((~resolve as _, ~reject as _) => ());
  put(node("#test-panel"), "getAnimations", _ =>
    [|{ "finished": unfinished }|]
  );
  click("#test-trigger");
  settle();
  check(
    "waiting for real animation completion",
    P.phase(app.model().popover) == Ui.Animation.Entering,
  );
  stop(app);
  /* jsdom blocks focus in manual popovers but has no showPopover method.
     Exercise native promotion and Tab order in Chromium, not a fake polyfill. */
  let app = start(~top_layer=true, ());
  check(
    "top layer attrs",
    attr("#test-panel", "popover") == Some("manual"),
  );
  layout();
  app.dispatch(Open);
  app.shutdown();
  settle();
  check(
    "shutdown before first draw cancels starts",
    listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
  );
  report("popover");
  Js.Promise.resolve();
};
