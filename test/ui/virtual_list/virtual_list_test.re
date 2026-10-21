open Tea;
open Test_support;
module V = Ui.Virtual_list;
module G = V.Geometry;
/* Explicit layout/observer fixture: jsdom never supplies real geometry. */
let fixture: unit => Js.Json.t = [%raw
  {|function () {
  const previous = globalThis.ResizeObserver;
  const live = new Set();
  let stopped = 0;
  globalThis.ResizeObserver = class {
    constructor(callback) { this.callback = callback; this.targets = new Set(); live.add(this); }
    observe(target) { this.targets.add(target); }
    unobserve(target) { this.targets.delete(target); }
    disconnect() { live.delete(this); this.targets.clear(); stopped++; }
  };
  return {
    layout(height) {
      Object.defineProperty(document.querySelector('#test-viewport'), 'clientHeight', {configurable:true, value:height});
    },
    notify(changes) {
      for (const observer of live) {
        const entries = [];
        for (const target of observer.targets) {
          const key = target.getAttribute('data-tea-virtual-key');
          const size = changes[key];
          if (size !== undefined) {
            target.getBoundingClientRect = () => ({height:size});
            entries.push({target});
          }
        }
        if (entries.length) observer.callback(entries);
      }
    },
    count: () => live.size,
    targets: () => [...live].reduce((n, o) => n + o.targets.size, 0),
    stopped: () => stopped,
    restore() { globalThis.ResizeObserver = previous; }
  };
}|}
];
[@mel.send] external layout: (Js.Json.t, float) => unit = "layout";
[@mel.send] external notify: (Js.Json.t, Js.t({..})) => unit = "notify";
[@mel.send] external observer_count: Js.Json.t => int = "count";
[@mel.send] external target_count: Js.Json.t => int = "targets";
[@mel.send] external stopped: Js.Json.t => int = "stopped";
[@mel.send] external restore: Js.Json.t => unit = "restore";
let near = (a, b) => abs_float(a -. b) < 0.00001;
let pure = () => {
  for (n in 0 to 64) {
    let geometry = ref(G.create(~count=n, ~estimate=20.));
    let sizes =
      Array.init(n, i => float_of_int(1 + (i * 53 + n * 7) mod 117));
    let original = geometry^;
    Array.iteri(
      (index, size) => geometry := G.measure(~index, ~size, geometry^),
      sizes,
    );
    check(
      "geometry persistent",
      near(G.total(original), float_of_int(n) *. 20.),
    );
    let expected = ref(0.);
    Array.iteri(
      (index, size) => {
        check(
          "prefix oracle",
          near(G.offset(~index, geometry^), expected^),
        );
        expected := expected^ +. size;
      },
      sizes,
    );
    check("total oracle", near(G.total(geometry^), expected^));
    check(
      "offset clamp",
      G.offset(~index=-3, geometry^) == 0.
      && near(G.offset(~index=n + 3, geometry^), expected^),
    );
    for (vp in 0 to 5) {
      let viewport = float_of_int(vp * 31);
      for (position in 0 to 25) {
        let scroll = float_of_int(position * 57 - 20);
        for (overscan in 0 to 3) {
          let range = G.range(~viewport, ~scroll, ~overscan, geometry^);
          if (n == 0 || vp == 0) {
            check("empty range", range.first == 0 && range.last == 0);
          } else {
            let scroll =
              min(max(0., scroll), max(0., expected^ -. viewport));
            let locate = y => {
              let remaining = ref(y);
              let result = ref(n - 1);
              let found = ref(false);
              Array.iteri(
                (i, size) => {
                  if (! found^ && remaining^ < size) {
                    result := i;
                    found := true;
                  };
                  remaining := remaining^ -. size;
                },
                sizes,
              );
              result^;
            };
            check(
              "range linear oracle",
              range.first == max(0, locate(scroll) - overscan)
              && range.last
              == min(n, locate(scroll +. viewport) + 1 + overscan),
            );
          };
        };
      };
    };
  };
  let huge = ref(G.create(~count=1000000, ~estimate=32.));
  for (i in 0 to 999) {
    huge :=
      G.measure(~index=i * 997, ~size=float_of_int(10 + i mod 100), huge^);
  };
  for (i in 0 to 99) {
    let range =
      G.range(~viewport=600., ~scroll=float_of_int(i * 310000), huge^);
    check(
      "million row inverse prefix bound",
      range.operations <= 42 && range.operations >= 38,
    );
  };
  let fixed = G.create(~count=1000000, ~estimate=32.);
  check(
    "fixed range independent of distance",
    G.range(~viewport=320., ~scroll=16000000., ~overscan=0, fixed).first
    == 500000,
  );
  let oversized =
    G.range(~viewport=320., ~scroll=16000000., ~overscan=max_int, fixed);
  check(
    "overscan saturates without integer overflow",
    oversized.first == 0 && oversized.last == 1000000,
  );
  let invalid = G.create(~count=-5, ~estimate=-2.);
  check("negative count", G.total(invalid) == 0.);
  let one = G.create(~count=1, ~estimate=0.);
  check("invalid estimate normalized", G.total(one) == 1.);
  check(
    "invalid measurements ignored",
    G.total(
      G.measure(~index=0, ~size=0., G.measure(~index=5, ~size=50., one)),
    )
    == 1.,
  );
  let special = V.items([|"__proto__", "constructor", "toString", ""|]);
  let model =
    V.init(
      ~id="special",
      ~items=special,
      ~row_size=V.Fixed(20.),
      ~viewport=10.,
      (),
    );
  let (model, _) =
    V.scroll_to_key(
      ~items=special,
      ~key="constructor",
      ~align=V.Start,
      model,
    );
  check(
    "prototype-looking keys are ordinary keys",
    V.count(special) == 4 && V.scroll_offset(model) == 20.,
  );
  let keys = [|"a", "b"|];
  let collection = V.items(keys);
  keys[0] = "changed";
  check(
    "collection owns immutable copy",
    V.key_at(~index=0, collection) == Some("a"),
  );
  check(
    "key bounds",
    V.key_at(~index=-1, collection) == None
    && V.key_at(~index=2, collection) == None,
  );
};
type msg =
  | Virtual(V.msg)
  | Replace(V.items)
  | Jump(int, V.align)
  | Jump_key(string)
  | Pin(list(int));
type model = {
  virtual_: V.model,
  items: V.items,
  frames: int,
  outputs: int,
  pinned: list(int),
};
let update = (msg, model) =>
  switch (msg) {
  | Virtual(msg) =>
    let (virtual_, cmd, out) =
      V.update(~items=model.items, msg, model.virtual_);
    (
      {
        ...model,
        virtual_,
        frames: model.frames + 1,
        outputs: model.outputs + List.length(out),
      },
      Cmd.map(msg => Virtual(msg), cmd),
    );
  | Replace(items) =>
    let (virtual_, cmd) = V.refresh(~items, model.virtual_);
    (
      {
        ...model,
        virtual_,
        items,
      },
      Cmd.map(msg => Virtual(msg), cmd),
    );
  | Jump(index, align) =>
    let (virtual_, cmd) =
      V.scroll_to_index(~items=model.items, ~index, ~align, model.virtual_);
    (
      {
        ...model,
        virtual_,
      },
      Cmd.map(msg => Virtual(msg), cmd),
    );
  | Jump_key(key) =>
    let (virtual_, cmd) =
      V.scroll_to_key(
        ~items=model.items,
        ~key,
        ~align=V.Start,
        model.virtual_,
      );
    (
      {
        ...model,
        virtual_,
      },
      Cmd.map(msg => Virtual(msg), cmd),
    );
  | Pin(pinned) => (
      {
        ...model,
        pinned,
      },
      Cmd.none,
    )
  };
let view = (~semantics=V.Set, model) =>
  V.view(
    ~model=model.virtual_,
    ~items=model.items,
    ~pinned=model.pinned,
    ~semantics,
    ~to_parent=msg => Virtual(msg),
    ~to_view=
      parts =>
        Html.div(
          parts.container
          @ [
            Html.role(semantics == V.Grid ? "grid" : "list"),
            Html.tabindex(0),
            Html.aria("label", "Rows"),
            Html.style("height", "100px"),
          ],
          [
            Html.keyed(
              "div",
              parts.content @ [Html.role("presentation")],
              List.map(
                (row: V.row(msg)) =>
                  (
                    row.key,
                    Html.div(
                      row.attrs
                      @ [Html.role(semantics == V.Grid ? "row" : "listitem")],
                      [
                        semantics == V.Grid
                          ? Html.div(
                              [Html.role("gridcell")],
                              [Html.text(row.key)],
                            )
                          : Html.text(row.key),
                      ],
                    ),
                  ),
                parts.rows,
              ),
            ),
          ],
        ),
    (),
  );
let raw_scroll = top => {
  put(node("#test-viewport"), "scrollTop", top);
  ignore(
    dispatch(
      node("#test-viewport"),
      event("scroll", { "bubbles": false }),
    ),
  );
};
let rows = () => query_all(document, "[data-tea-virtual-key]");
let settle = () => {
  frame();
  frame();
};
let completed = {
  pure();
  let fixture = fixture();
  let items = V.items(Array.init(10000, i => "key-" ++ string_of_int(i)));
  let mount = (~semantics=V.Set, row_size) =>
    App.element(
      ~init=
        () =>
          (
            {
              virtual_:
                V.init(
                  ~id="test",
                  ~items,
                  ~row_size,
                  ~viewport=100.,
                  ~overscan=2,
                  (),
                ),
              items,
              frames: 0,
              outputs: 0,
              pinned: [],
            },
            Cmd.none,
          ),
      ~update,
      ~view=model => view(~semantics, model),
      ~subscriptions=
        model =>
          Sub.map(msg => Virtual(msg), V.subscriptions(model.virtual_)),
      ~node="#app",
      ~flags=(),
      (),
    );
  let app = mount(V.Estimated(20.));
  layout(fixture, 100.);
  settle();
  check("one observer", observer_count(fixture) == 1);
  check("bounded initial DOM", Array.length(rows()) == 8);
  check(
    "row full cardinality",
    attr("#test-item-key-0", "aria-setsize") == Some("10000")
    && attr("#test-item-key-0", "aria-posinset") == Some("1"),
  );
  let before = app.model().frames;
  raw_scroll(100.);
  raw_scroll(120.);
  raw_scroll(200.);
  check("scroll is deferred", app.model().frames == before);
  frame();
  check(
    "scroll events coalesced latest wins",
    app.model().frames == before
    + 1
    && V.scroll_offset(app.model().virtual_) == 200.,
  );
  check("viewport notification", app.model().outputs == app.model().frames);
  check(
    "row position in full collection",
    attr("#test-item-key-10", "aria-posinset") == Some("11"),
  );
  let identity = node("#test-item-key-10");
  raw_scroll(220.);
  settle();
  check(
    "keyed DOM identity survives window shift",
    identity === node("#test-item-key-10"),
  );
  check("unclaimed Tab", key("#test-viewport", "keydown", "Tab"));
  check(
    "unclaimed Shift Tab",
    key(~shift=true, "#test-viewport", "keydown", "Tab"),
  );
  List.iter(
    key_ =>
      check(
        "unclaimed consumer keyboard",
        key("#test-viewport", "keydown", key_),
      ),
    [
      "ArrowDown",
      "ArrowUp",
      "Home",
      "End",
      "PageDown",
      "PageUp",
      " ",
      "Enter",
    ],
  );
  check(
    "unclaimed modifiers and IME",
    key(~ctrl=true, "#test-viewport", "keydown", "Home")
    && key(~alt=true, "#test-viewport", "keydown", "ArrowDown")
    && key(~meta=true, "#test-viewport", "keydown", "End")
    && key(~composing=true, "#test-viewport", "keydown", "ArrowDown"),
  );
  focus(node("#test-viewport"));
  app.dispatch(Pin([9999, 9999, (-1), 10000]));
  settle();
  check(
    "pin forces absent row",
    attr("#test-item-key-9999", "data-pinned") == Some(""),
  );
  check("pins deduplicated and bounded", Array.length(rows()) <= 11);
  check("pin never moves focus", has_focus("#test-viewport"));
  app.dispatch(Jump(5000, V.Start));
  settle();
  check(
    "estimated distant jump",
    near(V.scroll_offset(app.model().virtual_), 100000.),
  );
  check(
    "destination rendered before measurement",
    attr("#test-item-key-5000", "aria-posinset") == Some("5001"),
  );
  Js.Promise.then_(
    _ => {
      notify(
        fixture,
        {
          "key-4998": 40.,
          "key-4999": 30.,
          "key-5000": 60.,
        },
      );
      settle();
      check(
        "estimated jump corrected by measured prefix",
        near(V.scroll_offset(app.model().virtual_), 100030.),
      );
      check(
        "measurements keyed",
        V.measurement_count(app.model().virtual_) == 3,
      );
      raw_scroll(100040.);
      settle();
      notify(fixture, { "key-4998": 60. });
      settle();
      check(
        "anchor keeps within-row offset",
        near(V.scroll_offset(app.model().virtual_), 100060.),
      );
      check(
        "anchor DOM updated",
        near(get(node("#test-viewport"), "scrollTop"), 100060.),
      );
      let before = app.model().frames;
      notify(fixture, { "key-4998": 70. });
      notify(fixture, { "key-4998": 80. });
      frame();
      check("resize batches coalesced", app.model().frames == before + 1);
      check(
        "latest measurement wins",
        near(V.row_offset(~index=5000, app.model().virtual_), 100070.),
      );
      app.dispatch(Replace(V.items([|"key-5000", "new", "key-4998"|])));
      settle();
      check(
        "removed measurements pruned",
        V.measurement_count(app.model().virtual_) == 2,
      );
      check(
        "measured key reorder",
        V.row_offset(~index=1, app.model().virtual_) == 60.
        && V.row_offset(~index=3, app.model().virtual_) == 160.,
      );
      check(
        "full ARIA set after replacement",
        attr("#test-item-key-5000", "aria-setsize") == Some("3")
        && attr("#test-item-key-4998", "aria-posinset") == Some("3"),
      );
      app.dispatch(Replace(V.items([||])));
      settle();
      check(
        "empty repairs pins and window",
        Array.length(rows()) == 0
        && V.total_size(app.model().virtual_) == 0.
        && V.measurement_count(app.model().virtual_) == 0,
      );
      app.dispatch(Jump(4, V.Start));
      app.dispatch(Jump_key("missing"));
      settle();
      check(
        "empty commands no-op",
        V.scroll_offset(app.model().virtual_) == 0.,
      );
      app.dispatch(Replace(V.items([|"unsafe /_", "unsafe_0020/", ""|])));
      settle();
      let current = app.model().virtual_;
      check(
        "injective safe IDs",
        V.row_id(~key="unsafe /_", current)
        != V.row_id(~key="unsafe_0020/", current),
      );
      check(
        "zero measurements after replace",
        V.measurement_count(current) == 0,
      );
      raw_scroll(0.);
      app.shutdown();
      settle();
      check(
        "observer disconnect and pending frame cancellation",
        observer_count(fixture) == 0
        && target_count(fixture) == 0
        && stopped(fixture) == 1,
      );
      check("root listeners gone", Tea_dom.Debug.active_listeners() == 0);
      let app = mount(V.Fixed(20.));
      layout(fixture, 100.);
      settle();
      app.dispatch(Jump(500, V.Center));
      settle();
      check(
        "center alignment",
        V.scroll_offset(app.model().virtual_) == 9960.,
      );
      app.dispatch(Jump(500, V.End));
      settle();
      check("end alignment", V.scroll_offset(app.model().virtual_) == 9920.);
      app.dispatch(Jump(500, V.Auto));
      settle();
      check(
        "auto visible no-op",
        V.scroll_offset(app.model().virtual_) == 9920.,
      );
      app.dispatch(Jump_key("key-9000"));
      settle();
      check(
        "key jump before rendering",
        V.scroll_offset(app.model().virtual_) == 180000.,
      );
      app.dispatch(Jump(1, V.Start));
      app.dispatch(Jump(9999, V.Start));
      settle();
      check(
        "rapid command final target and end clamp",
        V.scroll_offset(app.model().virtual_) == 199900.
        && near(get(node("#test-viewport"), "scrollTop"), 199900.),
      );
      Js.Promise.then_(
        _ => {
          notify(fixture, { "key-9999": 200. });
          settle();
          check(
            "fixed ignores measurements",
            V.measurement_count(app.model().virtual_) == 0
            && V.total_size(app.model().virtual_) == 200000.,
          );
          app.dispatch(Replace(V.items([|"single"|])));
          settle();
          check(
            "single item",
            Array.length(rows()) == 1
            && V.scroll_offset(app.model().virtual_) == 0.,
          );
          raw_scroll(0.);
          app.dispatch(Jump(7, V.Start));
          settle();
          check(
            "new command defeats a queued stale scroll",
            V.scroll_offset(app.model().virtual_) == 0.,
          );
          app.dispatch(Replace(items));
          settle();
          raw_scroll(1000.);
          app.dispatch(Jump(7, V.Start));
          settle();
          check(
            "queued old scroll cannot undo command",
            V.scroll_offset(app.model().virtual_) == 140.
            && near(get(node("#test-viewport"), "scrollTop"), 140.),
          );
          app.shutdown();
          settle();
          let app = mount(~semantics=V.Grid, V.Fixed(20.));
          layout(fixture, 100.);
          settle();
          check(
            "grid positional semantics",
            attr("#test-viewport", "aria-rowcount") == Some("10000")
            && attr("#test-item-key-0", "aria-rowindex") == Some("1")
            && attr("#test-item-key-0", "aria-setsize") == None
            && attr("#test-item-key-0", "aria-posinset") == None,
          );
          app.shutdown();
          settle();
          let app = mount(V.Fixed(20.));
          app.shutdown();
          settle();
          check(
            "shutdown before deferred install",
            observer_count(fixture) == 0
            && Tea_dom.Debug.active_listeners() == 0,
          );
          restore(fixture);
          let app = mount(V.Estimated(20.));
          layout(fixture, 100.);
          settle();
          raw_scroll(200.);
          settle();
          check(
            "unsupported observer retains estimates and scroll",
            V.measurement_count(app.model().virtual_) == 0
            && V.scroll_offset(app.model().virtual_) == 200.,
          );
          app.shutdown();
          settle();
          check(
            "fallback shutdown leaves no root listeners",
            Tea_dom.Debug.active_listeners() == 0,
          );
          report("Virtual_list");
          Js.Promise.resolve();
        },
        tick(),
      );
    },
    tick(),
  );
};
