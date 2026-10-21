open Tea_dom;
open Tea_vdom;
open Tea_html;
module D = Tea_core.Decode;
external document: Dom.document = "document";
external global: Js.Json.t = "globalThis";
[@mel.send]
external query: (Dom.document, string) => Dom.node = "querySelector";
[@mel.get_index] external get: ('a, string) => Js.Json.t;
[@mel.set_index] external put: ('a, string, 'b) => unit;
external cast: 'a => 'b = "%identity";
[@mel.get] external children: Dom.node => array(Dom.node) = "childNodes";
[@mel.get] external outer: Dom.node => string = "outerHTML";
[@mel.get] external content: Dom.node => string = "textContent";
[@mel.get] external node_type: Dom.node => int = "nodeType";
[@mel.send]
external style_get: (Js.Json.t, string) => string = "getPropertyValue";
[@mel.get] external namespace: Dom.node => string = "namespaceURI";
[@mel.send]
external attribute: (Dom.node, string) => Js.Nullable.t(string) =
  "getAttribute";
[@mel.send]
external attribute_ns: (Dom.node, string, string) => Js.Nullable.t(string) =
  "getAttributeNS";
[@mel.send] external append: (Dom.node, Dom.node) => unit = "appendChild";
[@mel.send]
external dispatch_event: (Dom.node, Dom.event) => bool = "dispatchEvent";
[@mel.new] external event: (string, Js.t({..})) => Dom.event = "Event";
external flush: unit => unit = "teaFlushFrame";
external requests: unit => int = "teaFrameRequests";
let check = (message, condition) =>
  if (!condition) {
    failwith(message);
  };
let equal = (message, a, b) =>
  check(message ++ ": " ++ a ++ " != " ++ b, a == b);
let prop = (n, key): string => cast(get(n, key));
let bool_prop = (n, key): bool => cast(get(n, key));
let patch = (dom, old, next) => Patch.apply(dom, Diff.diff(old, next));
let no_dispatch = (_: int) => ();
let rendered = v => Render.create(no_dispatch, v);
let emit = (node, name) => {
  let e =
    event(
      name,
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  ignore(dispatch_event(node, e));
  e;
};
let attr = (node, key) =>
  Option.value(~default="", Js.Nullable.toOption(attribute(node, key)));
let platform = () => {
  let old_raf = get(global, "requestAnimationFrame");
  let old_timeout = get(global, "setTimeout");
  let called = ref(false);
  Fun.protect(
    ~finally=
      () => {
        put(global, "requestAnimationFrame", old_raf);
        put(global, "setTimeout", old_timeout);
      },
    () => {
      put(global, "requestAnimationFrame", Js.undefined);
      put(
        global,
        "setTimeout",
        (callback: unit => unit, delay: int) => {
          check("fallback timeout delay", delay == 16);
          callback();
          0;
        },
      );
      Browser_platform.request_animation_frame(timestamp => {
        called := timestamp > 0.
      });
    },
  );
  check("rAF fallback", called^);
  let old_clock = get(global, "performance");
  Fun.protect(
    ~finally=() => put(global, "performance", old_clock),
    () => {
      put(global, "performance", Js.undefined);
      check("Date.now fallback", Browser_platform.now() > 1000000000000.);
    },
  );
};
let styles = () => {
  let old =
    Vdom.node(
      "input",
      [
        Attr.none,
        Attr.value("model"),
        Attr.checked(true),
        Attr.disabled(true),
        Attr.readonly(true),
        Attr.required(true),
        Attr.autofocus(true),
        Attr.multiple(true),
        Attr.hidden(true),
        Attr.style("color", "red"),
        Attr.style("--x", "10"),
      ],
      [],
    );
  let dom = rendered(old);
  equal("value is property", prop(dom, "value"), "model");
  check(
    "value is not an attribute",
    Js.Nullable.toOption(attribute(dom, "value")) == None,
  );
  List.iter(
    key => check("boolean " ++ key, bool_prop(dom, key)),
    [
      "checked",
      "disabled",
      "readOnly",
      "required",
      "autoFocus",
      "multiple",
      "hidden",
    ],
  );
  put(dom, "value", "typed");
  put(dom, "checked", false);
  ignore(patch(dom, old, old));
  equal("unchanged model resets typed value", prop(dom, "value"), "model");
  check("unchanged model resets checkbox", bool_prop(dom, "checked"));
  let next =
    Vdom.node("input", [Attr.value("new"), Attr.style("--x", "20")], []);
  ignore(patch(dom, old, next));
  equal("new value", prop(dom, "value"), "new");
  check("removed checked", !bool_prop(dom, "checked"));
  List.iter(
    k => check("removed boolean " ++ k, !bool_prop(dom, k)),
    ["disabled", "readOnly", "required", "autoFocus", "multiple", "hidden"],
  );
  let style = get(dom, "style");
  equal("style removed", cast(get(style, "color")), "");
  equal("custom style retained", style_get(style, "--x"), "20");
  let empty = Vdom.node("input", [], []);
  ignore(patch(dom, next, empty));
  equal("removed value", prop(dom, "value"), "");
  equal("custom style removed", style_get(style, "--x"), "");
  List.iter(
    tag => {
      let before = Vdom.node(tag, [Attr.value("model")], []);
      let element = rendered(before);
      put(element, "value", "edited");
      ignore(patch(element, before, before));
      equal("controlled " ++ tag, prop(element, "value"), "model");
      Render.dispose(element);
    },
    ["textarea"],
  );
  let select_old =
    Html.select(
      [Attr.value("b")],
      [
        Html.option([Attr.attribute("value", "a")], [Html.text("A")]),
        Html.option([Attr.attribute("value", "b")], [Html.text("B")]),
      ],
    );
  let select = rendered(select_old);
  equal("select applies value after options", prop(select, "value"), "b");
  put(select, "value", "a");
  ignore(patch(select, select_old, select_old));
  equal("select value restored", prop(select, "value"), "b");
  Render.dispose(select);
  let selected = Vdom.node("option", [Attr.selected(true)], []);
  let option = rendered(selected);
  put(option, "selected", false);
  ignore(patch(option, selected, selected));
  check("selected is controlled", bool_prop(option, "selected"));
  ignore(patch(option, selected, Vdom.node("option", [], [])));
  check("selected removal", !bool_prop(option, "selected"));
  let a = Svg.svg([Svg.Attr.xlink_href("#a")], [Svg.circle([], [])]);
  let svg = rendered(a);
  equal("svg namespace", namespace(svg), Svg.namespace);
  equal("child namespace", namespace(children(svg)[0]), Svg.namespace);
  equal(
    "xlink",
    Option.get(
      Js.Nullable.toOption(
        attribute_ns(svg, "http://www.w3.org/1999/xlink", "href"),
      ),
    ),
    "#a",
  );
  ignore(patch(svg, a, Svg.svg([], [])));
  check(
    "xlink removal uses local name",
    Js.Nullable.toOption(
      attribute_ns(svg, "http://www.w3.org/1999/xlink", "href"),
    )
    == None,
  );
  List.iter(Render.dispose, [dom, option, svg]);
};
let attributes = () => {
  let facts = [
    Attr.class_("a"),
    Attr.classList([("a", true), ("b", false)]),
    Attr.data("x", "1"),
    Attr.aria("label", "x"),
    Attr.id("i"),
    Attr.href("/"),
    Attr.src("x"),
    Attr.alt("a"),
    Attr.type_("text"),
    Attr.placeholder("p"),
    Attr.name("n"),
    Attr.for_("f"),
    Attr.title("t"),
    Attr.role("button"),
    Attr.width(10),
    Attr.height(20),
    Attr.tabindex(2),
    Attr.attribute("inert", ""),
    Attr.property("custom", Vdom.Float(2.)),
  ];
  let old = Vdom.node("div", facts, []);
  let dom = rendered(old);
  List.iter(
    ((key, value)) => equal("attribute " ++ key, attr(dom, key), value),
    [
      ("class", "a"),
      ("data-x", "1"),
      ("aria-label", "x"),
      ("id", "i"),
      ("href", "/"),
      ("src", "x"),
      ("alt", "a"),
      ("type", "text"),
      ("placeholder", "p"),
      ("name", "n"),
      ("for", "f"),
      ("title", "t"),
      ("role", "button"),
      ("width", "10"),
      ("height", "20"),
      ("tabindex", "2"),
    ],
  );
  check(
    "boolean attribute present",
    Js.Nullable.toOption(attribute(dom, "inert")) == Some(""),
  );
  ignore(patch(dom, old, Vdom.node("div", [], [])));
  check(
    "custom float property reset",
    Js.typeof(get(dom, "custom")) == "undefined",
  );
  check(
    "boolean attribute removed",
    Js.Nullable.toOption(attribute(dom, "inert")) == None,
  );
  Render.dispose(dom);
};
let events = () => {
  let messages = ref([]);
  let send = x => messages := [x, ...messages^];
  let input =
    Html.input(
      [
        Events.on_input(s => String.length(s)),
        Events.on_check(b => if (b) {10} else {20}),
      ],
      [],
    );
  let old = Html.map(x => x + 100, Html.map(x => x * 2, input));
  let dom = Render.create(send, old);
  let before = Debug.active_listeners();
  put(dom, "value", "abcd");
  ignore(emit(dom, "input"));
  check("input decoder and nested taggers", messages^ == [108]);
  put(dom, "checked", true);
  ignore(emit(dom, "change"));
  check("check decoder", List.hd(messages^) == 120);
  let next = Html.map(x => x + 1000, Html.map(x => x * 3, input));
  let root = patch(dom, old, next);
  check("tagger update retains DOM", root === dom);
  ignore(emit(dom, "input"));
  check("old handler sees updated slots", List.hd(messages^) == 1012);
  check("slot updates retain listeners", Debug.active_listeners() == before);
  let submit = Html.form([Events.on_submit(7)], []);
  let form = Render.create(send, submit);
  let e = emit(form, "submit");
  check("submit prevents default", cast(get(e, "defaultPrevented")): bool);
  let custom =
    Html.form(
      [
        Events.custom(
          "submit",
          D.succeed({
            Vdom.message: 8,
            stop_propagation: true,
            prevent_default: true,
          }),
        ),
      ],
      [],
    );
  ignore(patch(form, submit, custom));
  let e = emit(form, "submit");
  check(
    "custom handler variant replacement",
    List.hd(messages^) == 8 && (cast(get(e, "defaultPrevented")): bool),
  );
  let fail =
    Html.form([Events.on("submit", Vdom.Normal(D.fail("no")))], []);
  ignore(patch(form, custom, fail));
  ignore(emit(form, "submit"));
  check("failed decode ignored", List.hd(messages^) == 8);
  ignore(patch(form, fail, Html.form([], [])));
  check("explicit remove detaches", Debug.active_listeners() == before);
  Render.dispose(dom);
  Render.dispose(form);
  let bubbling =
    Html.div(
      [Events.on_click(99)],
      [
        Html.button(
          [Events.stop_propagation_on("click", D.succeed((42, true)))],
          [],
        ),
      ],
    );
  let root = Render.create(send, bubbling);
  let prior = List.length(messages^);
  ignore(emit(children(root)[0], "click"));
  check(
    "stopPropagation suppresses parent",
    List.length(messages^) == prior + 1 && List.hd(messages^) == 42,
  );
  Render.dispose(root);
  let raw =
    Js.Json.parseExn(
      "{\"values\":[1,2,3],\"flag\":true,\"nil\":null,\"s\":\"abc\"}",
    );
  check(
    "array decoder",
    Dom_decode.run(D.field("values", D.array(D.int)), raw)
    == Ok([|1, 2, 3|]),
  );
  check(
    "list decoder",
    Dom_decode.run(D.field("values", D.list(D.float)), raw)
    == Ok([1., 2., 3.]),
  );
  check(
    "index decoder",
    Dom_decode.run(D.at(["values"], D.index(1, D.int)), raw) == Ok(2),
  );
  check(
    "bool/null/map2",
    Dom_decode.run(
      D.map2(
        (a, b) => a && b,
        D.field("flag", D.bool),
        D.field("nil", D.null(true)),
      ),
      raw,
    )
    == Ok(true),
  );
  check(
    "one_of/and_then",
    Dom_decode.run(
      D.one_of([
        D.fail("no"),
        D.and_then(
          s => D.succeed(String.length(s)),
          D.field("s", D.string),
        ),
      ]),
      raw,
    )
    == Ok(3),
  );
  check(
    "optional",
    Dom_decode.run(D.optional(D.field("missing", D.int)), raw) == Ok(None),
  );
  check(
    "missing field rejects even succeed",
    Result.is_error(Dom_decode.run(D.field("missing", D.succeed(1)), raw)),
  );
  check(
    "strict typeof",
    Result.is_error(Dom_decode.run(D.int, Js.Json.string("1"))),
  );
  check(
    "fractional int rejected",
    Result.is_error(Dom_decode.run(D.int, Js.Json.number(1.5))),
  );
  check(
    "array-like",
    Dom_decode.run(
      D.array(D.string),
      Js.Json.parseExn("{\"length\":2,\"0\":\"a\",\"1\":\"b\"}"),
    )
    == Ok([|"a", "b"|]),
  );
};
/* Canonical DOM serialization ignores empty style attributes left by CSSOM. */
let rec serialize = node =>
  if (node_type(node) == 3) {
    "T:" ++ content(node);
  } else {
    let attrs: array(Js.Json.t) = cast(get(node, "attributes"));
    let attrs =
      Array.to_list(
        Array.map(
          a => (cast(get(a, "name")), cast(get(a, "value"))),
          attrs,
        ),
      );
    let attrs = List.filter(((k, v)) => k != "style" || v != "", attrs);
    let attrs = List.sort(compare, attrs);
    prop(node, "localName")
    ++ "@"
    ++ namespace(node)
    ++ "["
    ++ String.concat(";", List.map(((k, v)) => k ++ "=" ++ v, attrs))
    ++ "]("
    ++ String.concat(
         ",",
         Array.to_list(Array.map(serialize, children(node))),
       )
    ++ ")";
  };
let randomized = () => {
  let rng = Random.State.make([|2026, 42|]);
  let random = n => Random.State.int(rng, n);
  let handler = Events.on_click(1);
  let rec tree = depth =>
    if (depth == 0 || random(5) == 0) {
      Vdom.text(string_of_int(random(8)));
    } else {
      let kids = List.init(random(4), _ => tree(depth - 1));
      let facts = [
        Attr.data("x", string_of_int(random(4))),
        Attr.none,
        handler,
      ];
      switch (random(6)) {
      | 0 => Html.map(x => x + 1, tree(depth - 1))
      | 1 => Vdom.lazy1(x => x, tree(depth - 1))
      | 2 =>
        Vdom.keyed(
          "div",
          facts,
          List.mapi(
            (i, k) => (string_of_int((i + random(3)) mod 5), k),
            kids,
          ),
        )
      | 3 =>
        Vdom.node(
          ~namespace=Svg.namespace,
          "g",
          [Attr.data("s", "x")],
          kids,
        )
      | 4 => Vdom.node("section", facts, kids)
      | _ => Vdom.node("div", facts, kids)
      };
    };
  let base = Debug.active_listeners();
  for (i in 1 to 500) {
    let old = tree(4);
    let next = tree(4);
    let plan = Diff.diff(old, next);
    check(
      "simulator oracle",
      Naive.equal(
        Patch_sim.apply_to_tree(Naive.render(old), plan),
        Naive.render(next),
      ),
    );
    let dom = rendered(old);
    let result = Patch.apply(dom, plan);
    let fresh = rendered(next);
    equal(
      "random pair " ++ string_of_int(i),
      serialize(result),
      serialize(fresh),
    );
    Render.dispose(result);
    Render.dispose(fresh);
  };
  check(
    "random roots release every listener",
    Debug.active_listeners() == base,
  );
};
let targeted = () => {
  let sent = ref(0);
  let row = (key, tag, label) => (
    key,
    Html.map(
      x => x + 10,
      Vdom.lazy1(
        n => Vdom.node(tag, [Events.on_click(n)], [Vdom.text(label)]),
        1,
      ),
    ),
  );
  let old =
    Vdom.keyed(
      "div",
      [],
      [row("a", "button", "old"), row("b", "span", "b")],
    );
  let next =
    Vdom.keyed(
      "div",
      [],
      [
        row("b", "span", "new b"),
        row("a", "p", "new a"),
        row("c", "em", "c"),
      ],
    );
  let dom = Render.create(n => sent := n, old);
  let retained = children(dom)[1];
  ignore(patch(dom, old, next));
  check(
    "reorder resolves roots after nested thunk redraw",
    children(dom)[0] === retained,
  );
  equal(
    "nested thunk redraw moved",
    prop(children(dom)[1], "localName"),
    "p",
  );
  ignore(emit(children(dom)[1], "click"));
  check("redrawn handler keeps outer tagger", sent^ == 11);
  let empty = Vdom.keyed("div", [], []);
  ignore(patch(dom, next, empty));
  check("reorder removal", Array.length(children(dom)) == 0);
  Render.dispose(dom);
  let nested = n =>
    Vdom.lazy1(n => Vdom.lazy1(n => Vdom.text(string_of_int(n)), n), n);
  let old = nested(1);
  let next = nested(2);
  let dom = rendered(old);
  let changed = patch(dom, old, next);
  check("nested thunk text preserves root", dom === changed);
  equal("nested subplan", content(changed), "2");
  Render.dispose(changed);
  let deep = (leaf, thunk) => {
    let node = ref(leaf);
    for (_i in 1 to 5000) {
      node :=
        (
          if (thunk) {
            Vdom.lazy1(n => n, node^);
          } else {
            Vdom.map(x => x + 1, node^);
          }
        );
    };
    node^;
  };
  List.iter(
    thunk => {
      let old = deep(Vdom.text("old"), thunk);
      let next = deep(Vdom.text("new"), thunk);
      let dom = rendered(old);
      let changed = patch(dom, old, next);
      equal("5000-deep scopes", content(changed), "new");
      Render.dispose(changed);
    },
    [true, false],
  );
};
let batching = () => {
  let view = model =>
    Html.button([Events.on_click(1)], [Html.text(string_of_int(model))]);
  let update = (msg, model) => model + msg;
  let app = App.beginner(~init=0, ~update, ~view, ~node="#app", ());
  let start = requests();
  let patches = Debug.patch_count();
  for (_i in 1 to 100) {
    app.dispatch(1);
  };
  check("100 messages schedule one frame", requests() - start == 1);
  check("before frame no patch", Debug.patch_count() == patches);
  flush();
  check("one patch per frame", Debug.patch_count() - patches == 1);
  equal(
    "model committed before render",
    content(query(document, "#app")),
    "100",
  );
  /* Animator intentionally schedules one spare rAF; it must not patch. */
  check("one spare frame", requests() - start == 2);
  flush();
  check("spare frame does not draw", Debug.patch_count() - patches == 1);
  app.shutdown();
  app.shutdown();
  let app =
    App.element(
      ~init=flags => (flags, Tea_core.Cmd.msg(2)),
      ~update=(n, m) => (m + n, Tea_core.Cmd.none),
      ~view,
      ~subscriptions=_ => Tea_core.Sub.none,
      ~node="#app",
      ~flags=3,
      (),
    );
  check("element init command", app.model() == 5);
  flush();
  flush();
  app.shutdown();
  let original_body = query(document, "body");
  let doc =
    App.document(
      ~init=() => (0, Tea_core.Cmd.none),
      ~update=(n, m) => (m + n, Tea_core.Cmd.none),
      ~view=
        n =>
          {
            App.title: string_of_int(n),
            body: [Html.p([], [Html.text(string_of_int(n))])],
          },
      ~subscriptions=_ => Tea_core.Sub.none,
      ~flags=(),
      (),
    );
  check(
    "document retains body identity",
    query(document, "body") === original_body,
  );
  equal("document title", cast(get(document, "title")), "0");
  doc.dispatch(3);
  flush();
  equal("document updates title", cast(get(document, "title")), "3");
  equal(
    "document body without extra wrapper",
    outer(query(document, "body")),
    "<body><p>3</p></body>",
  );
  doc.shutdown();
  flush();
};
let median = values => {
  Array.sort(Float.compare, values);
  values[Array.length(values) / 2];
};
let performance = () => {
  let row = (i, label) => (
    string_of_int(i),
    Html.li(
      [Attr.data("uid", string_of_int(i)), Events.on_click(i)],
      [Html.text(label)],
    ),
  );
  let rows = Array.init(1000, i => row(i, string_of_int(i)));
  let old = Vdom.keyed("ul", [], Array.to_list(rows));
  let new_rows = Array.copy(rows);
  new_rows[500] = row(500, "changed");
  let next = Vdom.keyed("ul", [], Array.to_list(new_rows));
  let start = Browser_platform.now();
  let dom = rendered(old);
  let mount = Browser_platform.now() -. start;
  let identities = Array.init(1000, i => children(dom)[i]);
  let text_identities = Array.map(n => children(n)[0], identities);
  let listeners = Debug.active_listeners();
  let times =
    Array.init(
      1000,
      i => {
        let (a, b) =
          if (i mod 2 == 0) {
            (old, next);
          } else {
            (next, old);
          };
        let t = Browser_platform.now();
        ignore(patch(dom, a, b));
        Browser_platform.now() -. t;
      },
    );
  Array.iteri(
    (i, n) => {
      check("update retains row identity", children(dom)[i] === n);
      check(
        "update retains text identity",
        children(n)[0] === text_identities[i],
      );
    },
    identities,
  );
  check(
    "1000 patches zero listener growth",
    Debug.active_listeners() == listeners,
  );
  let reversed = Vdom.keyed("ul", [], List.rev(Array.to_list(rows)));
  ignore(patch(dom, old, reversed));
  Array.iteri(
    (i, n) =>
      check("reorder retains identity", children(dom)[999 - i] === n),
    identities,
  );
  let elapsed = median(times);
  Printf.printf(
    "DOM PERF: diff+patch median %.4f ms (1000 samples); mount 1000 rows %.4f ms; zero recreated rows; zero listener growth\n%!",
    elapsed,
    mount,
  );
  /* Report host-dependent timings; enforce structural invariants above.
     See bench/dom and docs/perf.md for performance-budget measurements. */
  Render.dispose(dom);
};
let () = {
  platform();
  styles();
  attributes();
  events();
  randomized();
  targeted();
  batching();
  performance();
  check("all tests release all listeners", Debug.active_listeners() == 0);
  print_endline(
    "DOM tests: all passed (500 randomized pairs, forms/events/scopes/apps/performance)",
  );
};
