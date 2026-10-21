open Tea;

type msg =
  | Request(Navigation.url_request)
  | Changed(Url.t)
  | Push(string)
  | Replace(string)
  | Back
  | Tick;
type model = {
  path: string,
  requests: list(Navigation.url_request),
  changes: list(string),
  ticks: int,
};
let check = (label, condition) =>
  if (!condition) {
    failwith("application: " ++ label);
  };
let flush: unit => unit = [%mel.raw {|function() { teaFlushFrame(); }|}];
let title: unit => string = [%mel.raw
  {|function() { return document.title; }|}
];
let body: unit => Dom.element = [%mel.raw
  {|function() { return document.body; }|}
];
let html: unit => string = [%mel.raw
  {|function() { return document.body.innerHTML; }|}
];
let click: (string, string) => bool = [%mel.raw
  {|function(selector, kind) {
    var node = document.querySelector(selector);
    var e = new window.MouseEvent('click', {
      bubbles: true, cancelable: true, button: kind === 'right' ? 2 : 0,
      ctrlKey: kind === 'ctrl', metaKey: kind === 'meta',
      shiftKey: kind === 'shift', altKey: kind === 'alt'
    });
    if (kind === 'prevented') e.preventDefault();
    // Suppress jsdom's unsupported default navigation AFTER our app sees it.
    var prevented = false;
    function observe(event) { prevented = event.defaultPrevented; event.preventDefault(); }
    window.addEventListener('click', observe, {once: true});
    node.dispatchEvent(e);
    return prevented;
  }|}
];
let pop: unit => unit = [%mel.raw
  {|function() { window.dispatchEvent(new window.PopStateEvent('popstate')); }|}
];
let wait_pop: unit => Js.Promise.t(unit) = [%mel.raw
  {|function() {
    return new Promise(function(resolve, reject) {
      var timer = setTimeout(function() { reject(new Error('back timed out')); }, 1000);
      window.addEventListener('popstate', function() { clearTimeout(timer); resolve(); }, {once:true});
    });
  }|}
];
let update = (model, msg) =>
  switch (msg) {
  | Request(request) => (
      {
        ...model,
        requests: [request, ...model.requests],
      },
      switch (request) {
      | Navigation.Internal(url) => Navigation.push_url(Url.to_string(url))
      | Navigation.External(_) => Cmd.none
      },
    )
  | Changed(url) => (
      {
        ...model,
        path: url.path,
        changes: [url.path, ...model.changes],
      },
      Cmd.none,
    )
  | Push(path) => (model, Navigation.push_url(path))
  | Replace(path) => (model, Navigation.replace_url(path))
  | Back => (model, Navigation.back(1))
  | Tick => (
      {
        ...model,
        ticks: model.ticks + 1,
      },
      Cmd.none,
    )
  };
let view = (model): App.document_view(msg) => {
  title: model.path,
  body: [
    Html.a(
      [Html.id("internal"), Html.href("/inside")],
      [Html.span([Html.id("nested")], [Html.text("inside")])],
    ),
    Html.a(
      [Html.id("external"), Html.href("https://other.test/out")],
      [Html.text("outside")],
    ),
    Html.a(
      [
        Html.id("blank"),
        Html.href("/blank"),
        Html.attribute("target", "_blank"),
      ],
      [],
    ),
    Html.a(
      [
        Html.id("download"),
        Html.href("/file"),
        Html.attribute("download", ""),
      ],
      [],
    ),
    Html.button(
      [Html.id("tick"), Html.on_click(Tick)],
      [Html.text(string_of_int(model.ticks))],
    ),
  ],
};
let hooks: string => int = [%mel.raw
  {|function(type) { return teaHookCount(type); }|}
];
let stopped = ref(0);
let original_body = body();
let app =
  App.application(
    ~init=
      (flags, url) => {
        check("flags and initial URL", flags == 42 && url.path == "/start");
        (
          {
            path: url.path,
            requests: [],
            changes: [],
            ticks: 0,
          },
          Navigation.push_url("/initialized"),
        );
      },
    ~update,
    ~view,
    ~subscriptions=
      _ =>
        Sub.make(
          ~home="application-test",
          ~key="test",
          ~tagger=x => x,
          ~start=(_, ()) => stopped := stopped^ + 1,
        ),
    ~on_url_request=request => Request(request),
    ~on_url_change=url => Changed(url),
    ~flags=42,
    (),
  );
let () = {
  check("body retained", body() === original_body);
  check(
    "navigation hooks installed",
    hooks("popstate") == 1
    && hooks("tea:navigation") == 1
    && hooks("links") == 1,
  );
  check("initial render synchronous", title() == "/start");
  check("init push observed", app.model().changes == ["/initialized"]);
  flush();
  check("title updates on frame", title() == "/initialized");
  check("no body wrapper", String.starts_with(~prefix="<a ", html()));
  check("nested internal link intercepted", click("#nested", "normal"));
  check(
    "internal request",
    switch (List.hd(app.model().requests)) {
    | Navigation.Internal(url) => url.path == "/inside"
    | _ => false
    },
  );
  check("push notification", app.model().path == "/inside");
  check("external link intercepted", click("#external", "normal"));
  check(
    "external request",
    List.hd(app.model().requests)
    == Navigation.External("https://other.test/out"),
  );
  check(
    "external does not navigate automatically",
    Navigation.current_url().path == "/inside",
  );
  List.iter(
    kind => check("ignored " ++ kind, !click("#internal", kind)),
    ["ctrl", "meta", "shift", "alt", "right"],
  );
  check("already prevented preserved", click("#internal", "prevented"));
  check("target blank ignored", !click("#blank", "normal"));
  check("download ignored", !click("#download", "normal"));
  check(
    "ignored clicks send no requests",
    List.length(app.model().requests) == 2,
  );
  app.dispatch(Push("/next"));
  app.dispatch(Replace("/replaced"));
  check(
    "push/replace",
    app.model().changes == ["/replaced", "/next", "/inside", "/initialized"],
  );
  flush();
  check("document frame updates", title() == "/replaced");
  ignore(click("#tick", "normal"));
  check("DOM events share runtime", app.model().ticks == 1);
  /* Compile/use both sides of the merged Random API. */
  let (n, _) = Random.step(Random.int(7, 7), Random.initial_seed(1));
  check("pure Random reexport", n == 7);
  ignore(Random.generate(_ => Tick, Random.constant(1)));
};
let completion = {
  let waiting = wait_pop();
  app.dispatch(Back);
  Js.Promise.then_(
    () => {
      check("real back event", app.model().path == "/inside");
      check("back recorded once", List.length(app.model().changes) == 5);
      /* Shutdown while a render is pending: neither that frame nor future
         browser events may modify the document or committed model. */
      let before = app.model();
      let old_title = title();
      let listeners = Tea_dom.Debug.active_listeners();
      check("managed DOM listener installed", listeners > 0);
      app.shutdown();
      app.shutdown();
      // wait_pop's once-only observer is counted by the scaffold, too.
      check(
        "navigation notification hook removed",
        hooks("tea:navigation") == 0,
      );
      check("popstate app hook removed", hooks("popstate") == 1);
      check("link hook removed", hooks("links") == 0);
      check("user subscriptions stopped once", stopped^ == 1);
      check(
        "DOM listeners removed",
        Tea_dom.Debug.active_listeners() == listeners - 1,
      );
      check("link interception removed", !click("#internal", "normal"));
      ignore(click("#tick", "normal"));
      app.dispatch(Push("/ignored"));
      pop();
      flush();
      check("shutdown ignores dispatch/events", app.model() == before);
      check("pending frame ignored", title() == old_title);
      /* A subsequent application can own the same body without old hooks. */
      let second =
        App.application(
          ~init=
            ((), url) =>
              (
                {
                  ...before,
                  path: url.path,
                  requests: [],
                  changes: [],
                },
                Cmd.none,
              ),
          ~update,
          ~view,
          ~subscriptions=_ => Sub.none,
          ~on_url_request=request => Request(request),
          ~on_url_change=url => Changed(url),
          ~flags=(),
          (),
        );
      check("remount click", click("#internal", "normal"));
      check(
        "one request after remount",
        List.length(second.model().requests) == 1,
      );
      check("old app stays stopped", app.model() == before);
      second.shutdown();
      check(
        "remount cleanup",
        hooks("tea:navigation") == 0 && hooks("links") == 0,
      );
      print_endline(
        "application: clicks, history, frames, shutdown and Tea API passed",
      );
      Js.Promise.resolve();
    },
    waiting,
  );
};
