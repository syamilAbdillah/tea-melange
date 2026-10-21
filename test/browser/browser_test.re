open Tea;
open Test_support;
module B = Browser_dom;
module E = Browser_events;
let ok = result => result == Some(Ok());
let missing = result => result == Some(Error(B.Not_found("missing")));
let mount = view =>
  Tea_dom.App.beginner(
    ~init=(),
    ~update=(_, m) => m,
    ~view=_ => view,
    ~node="#app",
    (),
  );
let tasks = () => {
  let app =
    Tea_dom.App.element(
      ~init=() => (false, Cmd.none),
      ~update=(_, _) => (true, B.focus_cmd("created")),
      ~view=
        created =>
          Html.div(
            [],
            created ? [Html.button([Html.id("created")], [])] : [],
          ),
      ~subscriptions=_ => Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  app.dispatch();
  check("focus command waits for creating render", !has_focus("#app"));
  flush();
  check("focus after update creates element", has_focus("#created"));
  check("blur", ok(run_task(B.blur("created"))));
  check(
    "prevent-scroll focus",
    ok(run_task(B.focus(~prevent_scroll=true, "created"))),
  );
  check(
    "active id",
    run_task(B.active_element_id) == Some(Ok(Some("created"))),
  );
  check("missing focus", missing(run_task(B.focus("missing"))));
  check("missing blur", missing(run_task(B.blur("missing"))));
  check(
    "missing focus first",
    missing(run_task(B.focus_first(~within="missing"))),
  );
  check(
    "missing scroll",
    missing(run_task(B.scroll_into_view("missing"))),
  );
  check(
    "missing measurement",
    missing(run_task(B.get_element("missing"))),
  );
  List.iter(
    task => check("missing dialog/animations", missing(run_task(task))),
    [
      B.show_modal("missing"),
      B.show("missing"),
      B.close("missing"),
      B.wait_for_animations("missing"),
    ],
  );
  List.iter(
    block =>
      check(
        "scroll fallback",
        ok(run_task(B.scroll_into_view(~block, "created"))),
      ),
    [`Nearest, `Center, `Start, `End],
  );
  let scrolled = ref("");
  put(node("#created"), "scrollIntoView", options =>
    scrolled := get(options, "block")
  );
  ignore(run_task(B.scroll_into_view(~block=`Center, "created")));
  check("scroll block passed", scrolled^ == "center");
  switch (run_task(B.get_element("created"))) {
  | Some(Ok(info)) =>
    check(
      "measurement",
      info.rect.width == 0.
      && info.viewport_width > 0.
      && info.viewport_height > 0.
      && info.scroll_x == 0.
      && info.scroll_y == 0.,
    )
  | _ => failwith("measurement did not complete")
  };
  app.shutdown();
  frame();
  let app =
    mount(
      Html.div(
        [Html.id("within")],
        [
          Html.button([Html.disabled(true)], []),
          Html.button([Html.tabindex(-1)], []),
          Html.div([Html.hidden(true)], [Html.button([], [])]),
          Html.div([Html.attribute("inert", "")], [Html.button([], [])]),
          Html.button([Html.style("display", "none")], []),
          Html.button([Html.style("visibility", "hidden")], []),
          Html.details([], [Html.button([], [])]),
          Html.fieldset([Html.disabled(true)], [Html.button([], [])]),
          Html.input([Html.type_("hidden"), Html.tabindex(1)], []),
          Html.button([Html.id("ordinary")], []),
          Html.button([Html.id("first"), Html.tabindex(1)], []),
          Html.dialog([Html.id("dialog")], []),
        ],
      ),
    );
  check(
    "first tabbable",
    ok(run_task(B.focus_first(~within="within"))) && has_focus("#first"),
  );
  check(
    "no tabbable descendants",
    run_task(B.focus_first(~within="ordinary"))
    == Some(Error(B.Not_found("ordinary"))),
  );
  check(
    "show modal fallback",
    ok(run_task(B.show_modal("dialog")))
    && attr("#dialog", "open") == Some(""),
  );
  check("already open", ok(run_task(B.show_modal("dialog"))));
  check(
    "close fallback",
    ok(run_task(B.close("dialog"))) && attr("#dialog", "open") == None,
  );
  check("already closed", ok(run_task(B.close("dialog"))));
  check(
    "show fallback",
    ok(run_task(B.show("dialog"))) && attr("#dialog", "open") == Some(""),
  );
  ignore(run_task(B.close("dialog")));
  ignore(run_task(B.blur("first")));
  check("body has no id", run_task(B.active_element_id) == Some(Ok(None)));
  check(
    "missing getAnimations resolves",
    ok(run_task(B.wait_for_animations("ordinary"))),
  );
  put(node("#ordinary"), "getAnimations", _ => [||]);
  check(
    "empty animation snapshot",
    ok(run_task(B.wait_for_animations("ordinary"))),
  );
  app.shutdown();
  frame();
  let app =
    mount(
      Html.div(
        [Html.id("radios")],
        [
          Html.details(
            [Html.attribute("open", "")],
            [
              Html.node(
                "summary",
                [Html.tabindex(-1)],
                [Html.text("Not tabbable")],
              ),
              Html.node(
                "summary",
                [],
                [Html.text("Not the first summary")],
              ),
            ],
          ),
          Html.input(
            [
              Html.id("unchecked"),
              Html.type_("radio"),
              Html.name("group"),
            ],
            [],
          ),
          Html.input(
            [
              Html.id("checked"),
              Html.type_("radio"),
              Html.name("group"),
              Html.checked(true),
            ],
            [],
          ),
        ],
      ),
    );
  check(
    "radio group and non-first summary tab stops",
    ok(run_task(B.focus_first(~within="radios"))) && has_focus("#checked"),
  );
  app.shutdown();
  frame();
};
type msg =
  | Enable(bool)
  | Record(string)
  | Version;
type model = {
  enabled: bool,
  version: int,
  seen: list(string),
};
let fixture = () =>
  Html.div(
    [],
    [
      Html.div(
        [Html.id("outer")],
        [Html.button([Html.id("inner")], [])],
      ),
      Html.button([Html.id("outside")], []),
    ],
  );
let subscriptions = model =>
  model.enabled
    ? Sub.batch([
        E.on_key_down(
          ~key="test",
          Decode.and_then(
            key =>
              key == "a"
                ? Decode.succeed(
                    Record("key" ++ string_of_int(model.version)),
                  )
                : Decode.fail("not a"),
            Decode.field("key", Decode.string),
          ),
        ),
        E.on_pointer_down_outside(
          ~ids=["outer", "inner", "missing"],
          ~focus=true,
          Record("outside"),
        ),
        E.on_resize(size => Record(size.width > 0. ? "resize" : "bad-size")),
        E.on_scroll(Record("scroll")),
        E.on_visibility_change(_ => Record("visibility")),
        E.on_element_resize(~id="outer", _ => Record("element")),
        E.on_media_query(~query="(prefers-reduced-motion: reduce)", matches =>
          Record(matches ? "media:true" : "media:false")
        ),
      ])
    : Sub.none;
let start = (~subs=subscriptions, ()) =>
  Tea_dom.App.element(
    ~init=
      () =>
        (
          {
            enabled: true,
            version: 0,
            seen: [],
          },
          Cmd.none,
        ),
    ~update=
      (msg, model) =>
        (
          switch (msg) {
          | Enable(enabled) => {
              ...model,
              enabled,
            }
          | Version => {
              ...model,
              version: model.version + 1,
            }
          | Record(value) => {
              ...model,
              seen: model.seen @ [value],
            }
          },
          Cmd.none,
        ),
    ~view=_ => fixture(),
    ~subscriptions=subs,
    ~node="#app",
    ~flags=(),
    (),
  );
let events = () => {
  let app = start();
  frame();
  check(
    "missing media delivers initial false",
    app.model().seen == ["media:false"],
  );
  check(
    "capture outside",
    listener_count_for("document", "pointerdown", true) == 1
    && listener_count_for("document", "focusin", true) == 1,
  );
  let before = List.length(app.model().seen);
  pointer("#inner");
  pointer("#outer");
  check(
    "nested ids inside do not fire",
    List.length(app.model().seen) == before,
  );
  pointer("#outside");
  ignore(
    dispatch(
      node("#outside"),
      focus_event("focusin", { "bubbles": true }),
    ),
  );
  check(
    "pointer and focus outside",
    List.length(app.model().seen) == before + 2,
  );
  ignore(key("#inner", "keydown", "b"));
  check("decoder rejects", List.length(app.model().seen) == before + 2);
  app.dispatch(Version);
  let count = listener_count();
  ignore(key("#inner", "keydown", "a"));
  check("retained key decoder fresh", List.mem("key1", app.model().seen));
  frame();
  check("kept listeners not restarted", listener_count() == count);
  ignore(dispatch(window, event("resize", { "bubbles": false })));
  ignore(dispatch(node("#inner"), event("scroll", { "bubbles": false })));
  ignore(
    dispatch(document, event("visibilitychange", { "bubbles": false })),
  );
  check(
    "resize scroll visibility",
    List.for_all(
      value => List.mem(value, app.model().seen),
      ["resize", "scroll", "visibility"],
    ),
  );
  app.dispatch(Enable(false));
  check("stop removes all listeners immediately", listener_count() == 0);
  frame();
  app.dispatch(Enable(true));
  frame();
  check("restart listeners", listener_count() == count);
  app.shutdown();
  check("shutdown removes listeners", listener_count() == 0);
  frame();
  let app = start();
  app.shutdown();
  frame();
  check("shutdown cancels deferred starts", listener_count() == 0);
  let app =
    start(
      ~subs=
        m =>
          m.enabled
            ? E.on_pointer_down_outside(
                ~ids=["missing"],
                Record("outside"),
              )
            : Sub.none,
      (),
    );
  frame();
  pointer("#inner");
  check("missing ids count as outside", app.model().seen == ["outside"]);
  check(
    "focus default is off",
    listener_count_for("document", "focusin", true) == 0,
  );
  app.shutdown();
  frame();
  let app =
    start(
      ~subs=
        m =>
          m.enabled
            ? E.on_key_down(
                ~key="capture",
                ~capture=true,
                Decode.succeed(Record("capture")),
              )
            : Sub.none,
      (),
    );
  frame();
  check(
    "capture keydown",
    listener_count_for("document", "keydown", true) == 1,
  );
  app.shutdown();
  frame();
};
type resize_mock;
external install_resize_mock: unit => resize_mock =
  "teaInstallResizeObserverMock";
[@mel.send]
external notify_resize: (resize_mock, float, float) => unit = "notify";
[@mel.send] external resize_count: resize_mock => int = "count";
[@mel.send] external resize_disconnected: resize_mock => int = "disconnected";
[@mel.send] external restore_resize: resize_mock => unit = "restore";
let resize = () => {
  let mock = install_resize_mock();
  let app =
    start(
      ~subs=
        m =>
          m.enabled
            ? E.on_element_resize(~id="outer", size =>
                Record(
                  Js.Float.toString(size.width)
                  ++ ":"
                  ++ Js.Float.toString(size.height),
                )
              )
            : Sub.none,
      (),
    );
  check("resize observer deferred", resize_count(mock) == 0);
  frame();
  check("resize observes", resize_count(mock) == 1);
  notify_resize(mock, 120., 40.);
  check("resize content rect", app.model().seen == ["120:40"]);
  app.dispatch(Enable(false));
  check(
    "resize disconnect on stop",
    resize_count(mock) == 0 && resize_disconnected(mock) == 1,
  );
  app.dispatch(Enable(true));
  frame();
  app.shutdown();
  frame();
  check(
    "resize disconnect on shutdown",
    resize_count(mock) == 0 && resize_disconnected(mock) == 2,
  );
  let app =
    start(
      ~subs=_ => E.on_element_resize(~id="missing", _ => Record("bad")),
      (),
    );
  frame();
  check("missing resize id no observer", resize_count(mock) == 0);
  app.shutdown();
  frame();
  restore_resize(mock);
};
let media = () => {
  let saved = get(window, "matchMedia");
  let send = ref(_ => ());
  let stopped = ref(0);
  let matches = ref(true);
  let media = {
    "matches": true,
    "addEventListener": (_, cb, _) => send := cb,
    "removeEventListener": (_, _, _) => incr(stopped),
  };
  put(window, "matchMedia", _ => media);
  let app =
    start(
      ~subs=
        m =>
          m.enabled
            ? E.on_media_query(~query="test", value =>
                Record(value ? "yes" : "no")
              )
            : Sub.none,
      (),
    );
  frame();
  check("media current", app.model().seen == ["yes"]);
  matches := false;
  put(media, "matches", matches^);
  send^(Js.Json.null);
  check("media changes", app.model().seen == ["yes", "no"]);
  app.shutdown();
  frame();
  check("media cleanup", stopped^ == 1);
  /* Initial delivery can synchronously remove its own subscription. */
  let app =
    Tea_dom.App.element(
      ~init=() => (true, Cmd.none),
      ~update=(_, _) => (false, Cmd.none),
      ~view=_ => fixture(),
      ~subscriptions=
        enabled =>
          enabled ? E.on_media_query(~query="self-stop", _ => ()) : Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  frame();
  check(
    "self-removing media cleans installation",
    !app.model() && stopped^ == 2,
  );
  app.shutdown();
  frame();
  let legacy_stops = ref(0);
  put(window, "matchMedia", _ =>
    {
      "matches": false,
      "addListener": cb => send := cb,
      "removeListener": _ => incr(legacy_stops),
    }
  );
  let app =
    start(
      ~subs=_ => E.on_media_query(~query="legacy", _ => Record("legacy")),
      (),
    );
  frame();
  check("legacy media", app.model().seen == ["legacy"]);
  app.shutdown();
  frame();
  check("legacy cleanup", legacy_stops^ == 1);
  put(window, "matchMedia", saved);
};
let locks = () => {
  let html = node("html");
  let body = node("body");
  let saved_html = attribute(html, "style");
  let saved_body = attribute(body, "style");
  set_attribute(html, "style", "overflow: auto !important; color: red;");
  set_attribute(body, "style", "padding-right: 7px; color: blue;");
  let exact_html = attribute(html, "style");
  let exact_body = attribute(body, "style");
  let app =
    Tea_dom.App.element(
      ~init=() => (2, Cmd.none),
      ~update=(n, _) => (n, Cmd.none),
      ~view=_ => fixture(),
      ~subscriptions=
        n =>
          Sub.batch([
            n > 0 ? E.scroll_lock(~key="a") : Sub.none,
            n > 1 ? E.scroll_lock(~key="b") : Sub.none,
          ]),
      ~node="#app",
      ~flags=(),
      (),
    );
  check("locks defer", attribute(html, "style") == exact_html);
  frame();
  check("lock acquired", get(get(html, "style"), "overflow") == "hidden");
  check(
    "scrollbar compensated",
    get(get(body, "style"), "paddingRight") != "7px",
  );
  app.dispatch(1);
  frame();
  check(
    "one lease remains",
    get(get(html, "style"), "overflow") == "hidden",
  );
  app.dispatch(0);
  frame();
  check(
    "exact style restoration",
    attribute(html, "style") == exact_html
    && attribute(body, "style") == exact_body,
  );
  app.dispatch(2);
  frame();
  app.shutdown();
  frame();
  check(
    "shutdown releases locks",
    attribute(html, "style") == exact_html
    && attribute(body, "style") == exact_body,
  );
  let app = start(~subs=_ => E.scroll_lock(~key="cancelled"), ());
  app.shutdown();
  frame();
  check(
    "cancelled lock never acquires",
    attribute(html, "style") == exact_html,
  );
  let restore = (element, saved) =>
    switch (Js.Nullable.toOption(saved)) {
    | Some(value) => set_attribute(element, "style", value)
    | None => remove_attribute(element, "style")
    };
  restore(html, saved_html);
  restore(body, saved_body);
  let app = start(~subs=_ => E.scroll_lock(~key="absent-style"), ());
  frame();
  app.shutdown();
  frame();
  check(
    "absent inline style preserved",
    attribute(html, "style") == saved_html
    && attribute(body, "style") == saved_body,
  );
};
type scroll_mock;
external install_scroll_mock: (string, int) => scroll_mock =
  "teaInstallScrollPlatformMock";
[@mel.send]
external scroll_calls: scroll_mock => array(array(float)) = "calls";
[@mel.send] external restore_scroll: scroll_mock => unit = "restore";
let ios_locks = () =>
  List.iter(
    ((platform, touches, ios)) => {
      let mock = install_scroll_mock(platform, touches);
      let app = start(~subs=_ => E.scroll_lock(~key="platform"), ());
      frame();
      let style = get(node("body"), "style");
      check(
        "fixed body only on iOS",
        String.equal(get(style, "position"), "fixed") == ios,
      );
      if (ios) {
        check(
          "fixed body keeps scroll offset",
          get(style, "left") == "-12px" && get(style, "top") == "-80px",
        );
      };
      app.shutdown();
      frame();
      check(
        "iOS scroll restoration only",
        scroll_calls(mock) == (ios ? [|[|12., 80.|]|] : [||]),
      );
      restore_scroll(mock);
    },
    [
      ("iPhone", 1, true),
      ("MacIntel", 2, true),
      ("MacIntel", 0, false),
      ("Linux", 5, false),
    ],
  );
let restoring_focus = () => {
  let app =
    Tea_dom.App.element(
      ~init=() => (false, Cmd.none),
      ~update=
        (opened, _) => (opened, opened ? B.focus_cmd("overlay") : Cmd.none),
      ~view=
        opened =>
          Html.div(
            [],
            [
              Html.button([Html.id("trigger")], []),
              Html.button([Html.id("other")], []),
              opened
                ? Html.button([Html.id("overlay")], []) : Html.text(""),
            ],
          ),
      ~subscriptions=
        opened => opened ? E.restore_focus(~key="overlay", ()) : Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  focus(node("#trigger"));
  app.dispatch(true);
  frame();
  check("overlay focus", has_focus("#overlay"));
  app.dispatch(false);
  frame();
  check("closing returns focus after unmount", has_focus("#trigger"));
  app.dispatch(true);
  frame();
  focus(node("#other"));
  app.dispatch(false);
  frame();
  check("never steal intentional focus", has_focus("#other"));
  focus(node("#trigger"));
  app.dispatch(true);
  frame();
  remove(node("#trigger"));
  app.dispatch(false);
  frame();
  check("disconnected remembered element ignored", !has_focus("#other"));
  app.shutdown();
  frame();
  /* An overlay kept mounted while closing (leave animation) still holds focus. */
  let app =
    Tea_dom.App.element(
      ~init=() => (false, Cmd.none),
      ~update=
        (opened, _) => (opened, opened ? B.focus_cmd("kept") : Cmd.none),
      ~view=
        opened =>
          Html.div(
            [],
            [
              Html.button([Html.id("opener")], []),
              Html.button([Html.id("bystander")], []),
              Html.div(
                [Html.id("panel"), Html.hidden(!opened)],
                [Html.button([Html.id("kept")], [])],
              ),
            ],
          ),
      ~subscriptions=
        opened =>
          opened
            ? E.restore_focus(~key="panel", ~within="panel", ()) : Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  focus(node("#opener"));
  app.dispatch(true);
  frame();
  check("kept overlay focused", has_focus("#kept"));
  app.dispatch(false);
  frame();
  check("focus leaves a mounted closed overlay", has_focus("#opener"));
  app.dispatch(true);
  frame();
  focus(node("#bystander"));
  app.dispatch(false);
  frame();
  check("within never steals outside focus", has_focus("#bystander"));
  app.shutdown();
  frame();
};
type resources = {
  timers: int,
  observers: int,
};
external resources: unit => resources = "teaResourceProbe";
let cancelled_animation_wait = () => {
  let app = mount(Html.div([Html.id("cancelled")], []));
  let pending = Js.Promise.make((~resolve as _, ~reject as _) => ());
  put(node("#cancelled"), "getAnimations", _ =>
    [|{ "finished": pending }|]
  );
  let before = resources();
  let count = ref(0);
  let cancel = B.watch_animations("cancelled", _ => incr(count));
  flush();
  let during = resources();
  check(
    "wait holds a watchdog and an observer",
    during.timers == before.timers
    + 1
    && during.observers == before.observers
    + 1,
  );
  cancel();
  cancel();
  let after = resources();
  check(
    "cancel releases watchdog and observer at once",
    after.timers == before.timers && after.observers == before.observers,
  );
  remove(node("#cancelled"));
  check("cancelled wait never completes", count^ == 0);
  let early = B.watch_animations("cancelled", _ => incr(count));
  early();
  flush();
  check("cancel before render never starts or completes", count^ == 0);
  app.shutdown();
  frame();
};
let animations = () => {
  let app = mount(Html.div([Html.id("animated")], []));
  let reject_animation = ref(_ => ());
  let promise =
    Js.Promise.make((~resolve as _, ~reject) =>
      reject_animation := (error => reject(. error))
    );
  put(node("#animated"), "getAnimations", _ => [|{ "finished": promise }|]);
  let count = ref(0);
  B.wait_for_animations("animated", _ => incr(count));
  flush();
  check("awaits pending animation", count^ == 0);
  reject_animation^(Failure("cancelled"));
  Js.Promise.then_(
    _ => {
      check("cancelled animation settles", count^ == 1);
      let pending = Js.Promise.make((~resolve as _, ~reject as _) => ());
      put(node("#animated"), "getAnimations", _ =>
        [|{ "finished": pending }|]
      );
      B.wait_for_animations("animated", _ => incr(count));
      flush();
      remove(node("#animated"));
      Js.Promise.then_(
        _ => {
          check("removed element settles", count^ == 2);
          app.shutdown();
          frame();
          let app = mount(Html.div([Html.id("watchdog")], []));
          put(node("#watchdog"), "getAnimations", _ =>
            [|{ "finished": pending }|]
          );
          let bounded =
            Js.Promise.make((~resolve, ~reject as _) => {
              B.wait_for_animations("watchdog", value => resolve(. value));
              flush();
            });
          Js.Promise.then_(
            result => {
              check("infinite/paused animation bounded", result == Ok());
              app.shutdown();
              frame();
              report("browser");
              Js.Promise.resolve();
            },
            bounded,
          );
        },
        tick(),
      );
    },
    tick(),
  );
};
let completed = {
  tasks();
  events();
  resize();
  media();
  locks();
  ios_locks();
  restoring_focus();
  cancelled_animation_wait();
  animations();
};
