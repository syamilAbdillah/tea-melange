open Tea;
open Test_support;
module D = Ui.Dialog;
module A = Ui.Animation;
external resources:
  unit =>
  {
    .
    "timers": int,
    "observers": int,
  } =
  "teaResourceProbe";
[@mel.new]
external mouse_event: (string, Js.t({..})) => Dom.event = "MouseEvent";
[@mel.scope "window"] [@mel.new]
external submit_event: (string, Js.t({..})) => Dom.event = "SubmitEvent";
type msg =
  | Show
  | Hide
  | Progress(D.msg)
  | Hook
  | Remove;
type model = {
  dialog: D.model,
  events: list(D.out_msg),
  hooks: int,
  removed: bool,
};
let settle = () => {
  for (_i in 1 to 14) {
    flush();
  };
};
let start =
    (
      ~guarded=false,
      ~close_on_backdrop=?,
      ~kind=D.Modal,
      ~initial_focus=?,
      ~description=false,
      ~named=false,
      ~actions=true,
      ~unmount=false,
      (),
    ) =>
  App.element(
    ~init=
      () =>
        (
          {
            dialog:
              D.init(
                ~id="test",
                ~kind,
                ~initial_focus?,
                ~dismissible=!guarded,
                ~close_on_backdrop?,
                (),
              ),
            events: [],
            hooks: 0,
            removed: false,
          },
          Cmd.none,
        ),
    ~update=
      (message, model) => {
        let (dialog, command, outs) =
          switch (message) {
          | Show =>
            let (dialog, command) = D.open_(model.dialog);
            (dialog, command, []);
          | Hide =>
            let (dialog, command) = D.close(model.dialog);
            (dialog, command, []);
          | Progress(message) => D.update(message, model.dialog)
          | Remove
          | Hook => (model.dialog, Cmd.none, [])
          };
        (
          {
            dialog,
            events: model.events @ outs,
            hooks: model.hooks + (message == Hook ? 1 : 0),
            removed: model.removed || message == Remove,
          },
          Cmd.map(message => Progress(message), command),
        );
      },
    ~view=
      model =>
        Html.div(
          [],
          [
            Ui.Button.button(
              ~id="opener",
              ~on_press=Show,
              [Html.text("Open")],
            ),
            Ui.Button.button(
              ~id="elsewhere",
              ~on_press=Hide,
              [Html.text("Elsewhere")],
            ),
            !model.removed && (!unmount || D.is_visible(model.dialog))
              ? D.view(
                  ~model=model.dialog,
                  ~name=named ? D.Label("Named") : D.Title,
                  ~has_description=description,
                  ~labels={ D.close: "Dismiss" },
                  ~on_key_down=
                    key => String.equal(key, "F2") ? Some(Hook) : None,
                  ~to_parent=message => Progress(message),
                  ~to_view=
                    parts =>
                      Html.node(
                        "dialog",
                        parts.root,
                        [
                          named
                            ? Html.text("")
                            : Html.h2(parts.title, [Html.text("Title")]),
                          description
                            ? Html.p(
                                parts.description,
                                [Html.text("Description")],
                              )
                            : Html.text(""),
                          actions
                            ? Html.div(
                                [],
                                [
                                  Html.button(
                                    [
                                      Html.id("first"),
                                      Html.type_("button"),
                                    ],
                                    [Html.text("First")],
                                  ),
                                  Html.button(
                                    parts.close_button,
                                    [Html.text("Dismiss")],
                                  ),
                                  Html.form(
                                    [
                                      Html.id("form"),
                                      Html.attribute("method", "dialog"),
                                    ],
                                    [
                                      Html.button(
                                        [
                                          Html.id("save"),
                                          Html.attribute("value", "saved"),
                                        ],
                                        [Html.text("Save")],
                                      ),
                                    ],
                                  ),
                                ],
                              )
                            : Html.text(""),
                        ],
                      ),
                  (),
                )
              : Html.text(""),
          ],
        ),
    ~subscriptions=
      model =>
        model.removed
          ? Sub.none
          : Sub.map(
              message => Progress(message),
              D.subscriptions(model.dialog),
            ),
    ~node="#app",
    ~flags=(),
    (),
  );
let stop = (app: Tea_core.Runtime.handle(model, msg)) => {
  app.shutdown();
  settle();
  check(
    "no element listeners after shutdown",
    Tea_dom.Debug.active_listeners() == 0,
  );
  check("no document/window listeners", listener_count() == 0);
  check(
    "scroll lock released",
    get(get(document, "documentElement"), "style")##overflow == "",
  );
};
let opened = app => {
  focus(node("#opener"));
  app.Tea_core.Runtime.dispatch(Show);
  settle();
};
let requests = (reason, events) =>
  List.length(
    List.filter(
      out =>
        switch (out) {
        | D.Close_requested(actual) => actual == reason
        | _ => false
        },
      events,
    ),
  );
let closed = events =>
  List.length(
    List.filter(
      out =>
        switch (out) {
        | D.Closed => true
        | _ => false
        },
      events,
    ),
  );
let point = (selector, name, x, y) => {
  ignore(
    dispatch(
      node(selector),
      pointer_event(
        name,
        {
          "bubbles": true,
          "cancelable": true,
          "clientX": x,
          "clientY": y,
          "pointerType": "mouse",
        },
      ),
    ),
  );
};
let click_point = (selector, x, y) =>
  ignore(
    dispatch(
      node(selector),
      mouse_event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
          "clientX": x,
          "clientY": y,
        },
      ),
    ),
  );
let completed = {
  let app = start(~description=true, ());
  check(
    "initial closed",
    !D.is_visible(app.model().dialog) && get(node("#test-dialog"), "hidden"),
  );
  check(
    "closed mounted Escape propagates without prevention",
    key_at_document("#first", "Escape") == (true, true),
  );
  check("native role not overridden", attr("#test-dialog", "role") == None);
  check(
    "root semantics",
    attr("#test-dialog", "tabindex") == Some("-1")
    && attr("#test-dialog", "aria-modal") == Some("true"),
  );
  check(
    "title idref resolves",
    attr("#test-dialog", "aria-labelledby") == attr("#test-title", "id"),
  );
  check(
    "description idref resolves",
    attr("#test-dialog", "aria-describedby")
    == attr("#test-description", "id"),
  );
  check(
    "localized close",
    attr("#test-close", "aria-label") == Some("Dismiss"),
  );
  let original = app.model().dialog;
  opened(app);
  check(
    "opening does not mutate prior model",
    D.phase(original) == A.Closed,
  );
  check(
    "open with default first focus",
    D.phase(app.model().dialog) == A.Open && has_focus("#first"),
  );
  check("native show fallback open", get(node("#test-dialog"), "open"));
  check(
    "scroll locked",
    get(get(document, "documentElement"), "style")##overflow == "hidden",
  );
  check("opened once", List.length(app.model().events) == 1);
  app.dispatch(Show);
  settle();
  check("idempotent open", List.length(app.model().events) == 1);
  List.iter(
    key_ => {
      check(
        "ctrl shortcut not claimed",
        key(~ctrl=true, "#first", "keydown", key_),
      );
      check(
        "alt shortcut not claimed",
        key(~alt=true, "#first", "keydown", key_),
      );
      check(
        "meta shortcut not claimed",
        key(~meta=true, "#first", "keydown", key_),
      );
      check(
        "composition not claimed",
        key(~composing=true, "#first", "keydown", key_),
      );
    },
    ["Escape", "Tab", "Enter", " ", "ArrowDown"],
  );
  check("hook never prevents", key("#first", "keydown", "F2"));
  check("hook mapped to parent", app.model().hooks == 1);
  ignore(key(~ctrl=true, "#first", "keydown", "F2"));
  ignore(key(~composing=true, "#first", "keydown", "F2"));
  check("hook respects modifiers and composition", app.model().hooks == 1);
  check("Tab never prevented", key("#first", "keydown", "Tab"));
  check(
    "Shift Tab never prevented",
    key(~shift=true, "#first", "keydown", "Tab"),
  );
  check("Enter native", key("#first", "keydown", "Enter"));
  check("Space native", key("#first", "keydown", " "));
  check(
    "modified Escape propagates",
    key_at_document(~ctrl=true, "#first", "Escape") == (true, true)
    && key_at_document(~alt=true, "#first", "Escape") == (true, true)
    && key_at_document(~meta=true, "#first", "Escape") == (true, true)
    && key_at_document(~composing=true, "#first", "Escape") == (true, true),
  );
  check(
    "Escape prevented and stopped at document",
    key_at_document("#first", "Escape") == (false, false),
  );
  check(
    "leave starts immediately",
    D.phase(app.model().dialog) == A.Leaving,
  );
  check(
    "repeat Escape prevented",
    key_at_document(~repeat=true, "#first", "Escape") == (false, false),
  );
  check(
    "only one request during leave",
    requests(D.Escape, app.model().events) == 1,
  );
  flush();
  check(
    "leave keeps native open before completion",
    get(node("#test-dialog"), "open"),
  );
  app.dispatch(Show);
  settle();
  check(
    "reverse leave never closes",
    D.phase(app.model().dialog) == A.Open
    && closed(app.model().events) == 0
    && get(node("#test-dialog"), "open"),
  );
  click("#test-close");
  settle();
  check(
    "close button emits request and closed",
    requests(D.Close_button, app.model().events) == 1
    && closed(app.model().events) == 1,
  );
  check(
    "native closes after leave",
    !get(node("#test-dialog"), "open")
    && get(node("#test-dialog"), "hidden"),
  );
  check("focus returns to opener", has_focus("#opener"));
  app.dispatch(Hide);
  settle();
  check("idempotent close", closed(app.model().events) == 1);
  opened(app);
  point("#first", "pointerdown", 5, 5);
  point("#test-dialog", "pointerup", 5, 5);
  click_point("#test-dialog", 5, 5);
  settle();
  check("drag out ignored", D.is_open(app.model().dialog));
  point("#test-dialog", "pointerdown", 5, 5);
  point("#test-dialog", "pointercancel", 5, 5);
  point("#test-dialog", "pointerup", 5, 5);
  click_point("#test-dialog", 5, 5);
  settle();
  check("pointercancel clears gesture", D.is_open(app.model().dialog));
  point("#test-dialog", "pointerdown", 5, 5);
  point("#test-dialog", "pointerup", 5, 5);
  click_point("#test-dialog", 5, 5);
  settle();
  check(
    "backdrop down/up/click dismisses",
    !D.is_visible(app.model().dialog)
    && requests(D.Backdrop, app.model().events) == 1,
  );
  opened(app);
  let submission =
    submit_event(
      "submit",
      {
        "bubbles": true,
        "cancelable": true,
        "submitter": node("#save"),
      },
    );
  check(
    "native form default prevented",
    !dispatch(node("#form"), submission),
  );
  settle();
  check(
    "native form value",
    D.return_value(app.model().dialog) == "saved"
    && requests(D.Native, app.model().events) == 1
    && !D.is_visible(app.model().dialog),
  );
  opened(app);
  check(
    "fresh open resets return value",
    D.return_value(app.model().dialog) == "",
  );
  check("cancel always prevented", !emit("#test-dialog", "cancel"));
  settle();
  check(
    "cancel routed through model",
    requests(D.Escape, app.model().events) == 2
    && !D.is_visible(app.model().dialog),
  );
  opened(app);
  put(node("#test-dialog"), "returnValue", "imperative");
  put(node("#test-dialog"), "open", false);
  ignore(emit("#test-dialog", "close"));
  settle();
  check(
    "imperative native close synchronizes model",
    D.return_value(app.model().dialog) == "imperative"
    && !D.is_visible(app.model().dialog),
  );
  opened(app);
  ignore(emit("#test-dialog", "close"));
  settle();
  check(
    "stale close on reopened element ignored",
    D.is_open(app.model().dialog),
  );
  app.dispatch(Hide);
  focus(node("#elsewhere"));
  settle();
  check("deliberate external focus not stolen", has_focus("#elsewhere"));
  opened(app);
  let removed_opener = node("#opener");
  remove(removed_opener);
  app.dispatch(Hide);
  settle();
  /* jsdom's fallback close removes open, but does not run native focus steps.
     Assert the actual requirement: the disconnected opener is not refocused. */
  check(
    "removed opener never refocused",
    !get(removed_opener, "isConnected")
    && get(document, "activeElement") !== removed_opener,
  );
  stop(app);

  let guarded = start(~guarded=true, ~kind=D.Alertdialog("test-close"), ());
  check(
    "alertdialog role",
    attr("#test-dialog", "role") == Some("alertdialog"),
  );
  check(
    "no dangling description",
    attr("#test-dialog", "aria-describedby") == None,
  );
  opened(guarded);
  check("least destructive initial focus", has_focus("#test-close"));
  for (_i in 1 to 6) {
    ignore(key("#test-close", "keydown", "Escape"));
  };
  settle();
  check(
    "parent guard rejects batched requests",
    D.is_open(guarded.model().dialog)
    && requests(D.Escape, guarded.model().events) == 6,
  );
  point("#test-dialog", "pointerdown", 5, 5);
  point("#test-dialog", "pointerup", 5, 5);
  click_point("#test-dialog", 5, 5);
  settle();
  check(
    "alert backdrop default disabled",
    requests(D.Backdrop, guarded.model().events) == 0,
  );
  put(node("#test-dialog"), "returnValue", "external");
  put(node("#test-dialog"), "open", false);
  ignore(emit("#test-dialog", "close"));
  settle();
  check(
    "guard repairs external native close",
    D.is_open(guarded.model().dialog)
    && get(node("#test-dialog"), "open")
    && requests(D.Native, guarded.model().events) == 1,
  );
  /* jsdom lacks formMethod; explicitly model its normalized DOM property. */
  put(node("#save"), "formMethod", "post");
  check(
    "normal form override is not intercepted",
    dispatch(
      node("#form"),
      submit_event(
        "submit",
        {
          "bubbles": true,
          "cancelable": true,
          "submitter": node("#save"),
        },
      ),
    ),
  );
  check(
    "normal form leaves model untouched",
    requests(D.Native, guarded.model().events) == 1,
  );
  put(node("#form"), "method", "post");
  put(node("#save"), "formMethod", "dialog");
  check(
    "dialog formmethod override is intercepted",
    !
      dispatch(
        node("#form"),
        submit_event(
          "submit",
          {
            "bubbles": true,
            "cancelable": true,
            "submitter": node("#save"),
          },
        ),
      ),
  );
  settle();
  check(
    "guarded form value exposed without closing",
    D.return_value(guarded.model().dialog) == "saved"
    && D.is_open(guarded.model().dialog)
    && requests(D.Native, guarded.model().events) == 2,
  );
  put(node("#form"), "method", "dialog");
  check(
    "implicit dialog submission prevented",
    !
      dispatch(
        node("#form"),
        submit_event(
          "submit",
          {
            "bubbles": true,
            "cancelable": true,
          },
        ),
      ),
  );
  check(
    "no submitter means empty value",
    D.return_value(guarded.model().dialog) == "",
  );
  guarded.dispatch(Hide);
  settle();
  check(
    "programmatic close bypasses guard",
    !D.is_visible(guarded.model().dialog),
  );
  stop(guarded);
  List.iter(
    ((initial_focus, expected, actions, named)) => {
      let app = start(~initial_focus, ~actions, ~named, ~unmount=true, ());
      opened(app);
      check("initial focus selection", has_focus(expected));
      if (named) {
        check(
          "label omits dangling title",
          attr("#test-dialog", "aria-labelledby") == None
          && attr("#test-dialog", "aria-label") == Some("Named"),
        );
      };
      app.dispatch(Remove);
      settle();
      check(
        "unmount removes dialog",
        Array.length(query_all(document, "#test-dialog")) == 0,
      );
      stop(app);
    },
    [
      ("test-close", "#test-close", true, false),
      ("missing", "#first", true, false),
      ("opener", "#first", true, false),
      ("missing", "#test-dialog", false, true),
    ],
  );
  /* Initial focus lands in the same frame as showModal(), so a Tab or click
     right after opening is never stolen back by a deferred focus. */
  let early = start();
  focus(node("#opener"));
  early.dispatch(Show);
  let rec until_open = remaining =>
    if (remaining > 0 && !get(node("#test-dialog"), "open")) {
      flush();
      until_open(remaining - 1);
    };
  until_open(14);
  check(
    "initial focus applied in the showModal frame",
    get(node("#test-dialog"), "open") && has_focus("#first"),
  );
  focus(node("#save"));
  settle();
  check("focus moved right after opening is kept", has_focus("#save"));
  stop(early);
  let ordered = start();
  set_attribute(node("#save"), "tabindex", "1");
  set_attribute(node("#first"), "tabindex", "2");
  opened(ordered);
  check("shared first selector drives initial focus", has_focus("#save"));
  ignore(run_task(Browser_dom.blur("save")));
  check(
    "reverse Tab is not cancelled",
    key(~shift=true, "#first", "keydown", "Tab"),
  );
  settle();
  check(
    "shared last selector repairs reverse Tab",
    has_focus("#test-close"),
  );
  ignore(run_task(Browser_dom.blur("test-close")));
  check("forward Tab is not cancelled", key("#first", "keydown", "Tab"));
  settle();
  check("shared first selector repairs forward Tab", has_focus("#save"));
  stop(ordered);
  let optout = start(~close_on_backdrop=false, ());
  opened(optout);
  point("#test-dialog", "pointerdown", 5, 5);
  point("#test-dialog", "pointerup", 5, 5);
  click_point("#test-dialog", 5, 5);
  settle();
  check("modal backdrop optout", D.is_open(optout.model().dialog));
  stop(optout);
  let shutdown = start();
  opened(shutdown);
  stop(shutdown);
  check(
    "shutdown closes mounted native dialog",
    !get(node("#test-dialog"), "open"),
  );
  let rapid = start();
  rapid.dispatch(Show);
  rapid.dispatch(Hide);
  rapid.dispatch(Show);
  rapid.dispatch(Hide);
  settle();
  check(
    "same-frame intents do not desync",
    !D.is_visible(rapid.model().dialog)
    && !get(node("#test-dialog"), "open"),
  );
  stop(rapid);
  let waiting = start();
  let never =
    Js.Promise.make((~resolve, ~reject) => {
      ignore(resolve);
      ignore(reject);
    });
  put(node("#test-dialog"), "getAnimations", _ =>
    [|{ "finished": never }|]
  );
  opened(waiting);
  check(
    "enter waits for real pending animation",
    D.phase(waiting.model().dialog) == A.Entering,
  );
  check(
    "wait owns watchdog and observer",
    resources()##timers == 1 && resources()##observers == 1,
  );
  waiting.dispatch(Hide);
  settle();
  check(
    "reversal cancels previous wait",
    resources()##timers == 1 && resources()##observers == 1,
  );
  stop(waiting);
  check(
    "shutdown cancels in-flight animation wait",
    resources()##timers == 0 && resources()##observers == 0,
  );
  let pending = start();
  pending.dispatch(Show);
  pending.shutdown();
  settle();
  check(
    "shutdown before native show leaves closed",
    !get(node("#test-dialog"), "open"),
  );
  check(
    "pending start releases listeners",
    Tea_dom.Debug.active_listeners() == 0 && listener_count() == 0,
  );
  report("Dialog");
  Js.Promise.resolve();
};
