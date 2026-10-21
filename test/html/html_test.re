module Html = Tea_html.Html;
module Attr = Tea_html.Attr;
module Svg = Tea_html.Svg;
module Events = Tea_html.Events;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Render = Tea_test.Render;
module Query = Tea_test.Query;
module Simulate = Tea_test.Simulate;

type msg =
  | Increment
  | Decrement
  | Set(int);

let update = (msg, model) =>
  switch (msg) {
  | Increment => model + 1
  | Decrement => model - 1
  | Set(n) => n
  };

let view = model =>
  Html.div(
    [],
    [
      Html.button([Html.on_click(Decrement)], [Html.text("−")]),
      Html.span([], [Html.text(string_of_int(model))]),
      Html.button([Html.on_click(Increment)], [Html.text("+")]),
    ],
  );

let equal = (label, expected, actual) =>
  if (expected != actual) {
    failwith(label ++ ": expected " ++ expected ++ ", got " ++ actual);
  };
let render = Render.to_string;
let null = D.Value.Null;
let event = (tree, selector, event_name, event) =>
  Simulate.event(tree, ~selector, ~event_name, ~event);
let target = (key, value) =>
  D.Value.Object([("target", D.Value.Object([(key, value)]))]);

let () = {
  equal(
    "counter",
    "<div><button>−</button><span>0</span><button>+</button></div>",
    render(view(0)),
  );
  assert(
    event(view(0), Query.Tag("button"), "click", null) == Some(Decrement),
  );
  assert(update(Increment, 0) == 1);
  assert(update(Decrement, 1) == 0);
  assert(update(Set(5), 0) == 5);
  let buttons = Query.find_all(view(0), ~selector=Query.Tag("button"));
  assert(List.length(buttons) == 2);
  assert(
    event(List.nth(buttons, 1), Query.Tag("button"), "click", null)
    == Some(Increment),
  );
};

type input_msg =
  | SetText(string)
  | Check(bool)
  | Submit
  | Key(string);
let () = {
  let input =
    Html.input(
      [
        Attr.id("entry"),
        Html.on_input(value => SetText(value)),
        Events.on_check(value => Check(value)),
        Events.on_key_down(key => key == "Enter" ? Some(Key(key)) : None),
      ],
      [],
    );
  assert(
    event(
      input,
      Query.Id("entry"),
      "input",
      target("value", D.Value.String("x")),
    )
    == Some(SetText("x")),
  );
  assert(
    event(
      Html.map(value => Some(value), input),
      Query.Id("entry"),
      "input",
      target("value", D.Value.String("mapped")),
    )
    == Some(Some(SetText("mapped"))),
  );
  assert(
    event(
      input,
      Query.Id("entry"),
      "input",
      target("value", D.Value.Bool(true)),
    )
    == None,
  );
  assert(event(input, Query.Id("entry"), "input", null) == None);
  assert(
    event(
      input,
      Query.Id("entry"),
      "change",
      target("checked", D.Value.Bool(true)),
    )
    == Some(Check(true)),
  );
  assert(
    event(
      input,
      Query.Tag("input"),
      "keydown",
      D.Value.Object([("key", D.Value.String("Enter"))]),
    )
    == Some(Key("Enter")),
  );
  assert(
    event(
      input,
      Query.Tag("input"),
      "keydown",
      D.Value.Object([("key", D.Value.String("Escape"))]),
    )
    == None,
  );
  assert(event(input, Query.Tag("button"), "input", null) == None);
  assert(event(input, Query.Tag("input"), "click", null) == None);
  assert(
    event(
      Html.form([Events.on_submit(Submit)], []),
      Query.Tag("form"),
      "submit",
      null,
    )
    == Some(Submit),
  );
  switch (Events.on_submit(Submit)) {
  | V.Event("submit", V.May_prevent_default(decoder)) =>
    assert(D.run(decoder, null) == Ok((Submit, true)))
  | _ => assert(false)
  };
  let changed =
    Html.select([Events.on_change(value => SetText(value))], []);
  assert(
    event(
      changed,
      Query.Tag("select"),
      "change",
      target("value", D.Value.String("selected")),
    )
    == Some(SetText("selected")),
  );
};

let () = {
  let boolean_properties = [
    ("checked", Attr.checked),
    ("selected", Attr.selected),
    ("disabled", Attr.disabled),
    ("hidden", Attr.hidden),
    ("readOnly", Attr.readonly),
    ("required", Attr.required),
    ("autoFocus", Attr.autofocus),
    ("multiple", Attr.multiple),
  ];
  List.iter(
    ((name, make)) => {
      assert(make(true) == V.Property(name, V.Bool(true)));
      assert(make(false) == V.Property(name, V.Bool(false)));
      let html_name =
        switch (name) {
        | "readOnly" => "readonly"
        | "autoFocus" => "autofocus"
        | name => name
        };
      equal(
        "boolean true " ++ name,
        "<input " ++ html_name ++ "=\"\">",
        render(Html.input([make(true)], [])),
      );
      equal(
        "boolean false " ++ name,
        "<input>",
        render(Html.input([make(false)], [])),
      );
    },
    boolean_properties,
  );
  assert(Attr.value("x") == V.Property("value", V.String("x")));
  let string_attributes = [
    ("class", Attr.class_),
    ("id", Attr.id),
    ("href", Attr.href),
    ("src", Attr.src),
    ("alt", Attr.alt),
    ("type", Attr.type_),
    ("placeholder", Attr.placeholder),
    ("name", Attr.name),
    ("for", Attr.for_),
    ("title", Attr.title),
    ("role", Attr.role),
  ];
  List.iter(
    ((name, make)) => assert(make("x") == V.Attribute(name, "x")),
    string_attributes,
  );
  List.iter(
    ((name, make)) => assert(make(12) == V.Attribute(name, "12")),
    [
      ("width", Attr.width),
      ("height", Attr.height),
      ("tabindex", Attr.tabindex),
    ],
  );
  assert(Attr.style("color", "red") == V.Style("color", "red"));
  assert(
    Attr.classList([("a", true), ("b", false), ("c", true)])
    == V.Attribute("class", "a c"),
  );
  assert(Attr.data("test", "x") == V.Attribute("data-test", "x"));
  assert(
    Attr.aria("hidden", "false") == V.Attribute("aria-hidden", "false"),
  );
  assert(Attr.attribute("custom", "x") == V.Attribute("custom", "x"));
  assert(
    Attr.property("custom", V.Float(2.5))
    == V.Property("custom", V.Float(2.5)),
  );
  assert(Tea_html.Attributes.id("x") == Attr.id("x"));
  switch (Html.div([Attr.none, Attr.id("x"), Attr.none], [])) {
  | V.Node({ facts, _ }) => assert(facts == [V.Attribute("id", "x")])
  | _ => assert(false)
  };
  equal(
    "booleans",
    "<input value=\"x\" checked=\"\" disabled=\"\" readonly=\"\" autofocus=\"\" multiple=\"\">",
    render(
      Html.input(
        [
          Attr.value("x"),
          Attr.checked(true),
          Attr.selected(false),
          Attr.disabled(true),
          Attr.hidden(false),
          Attr.readonly(true),
          Attr.required(false),
          Attr.autofocus(true),
          Attr.multiple(true),
        ],
        [],
      ),
    ),
  );
  equal(
    "selected",
    "<option selected=\"\">yes</option>",
    render(Html.option([Attr.selected(true)], [Html.text("yes")])),
  );
  equal(
    "cleared property",
    "<input>",
    render(
      Html.input([Attr.attribute("checked", ""), Attr.checked(false)], []),
    ),
  );
};

let () = {
  let shape =
    Svg.svg(
      [Attr.none, Svg.Attr.viewBox("0 0 10 10")],
      [
        Svg.use([Svg.Attr.xlink_href("#shape")], []),
        Svg.circle(
          [Svg.Attr.cx("5"), Svg.Attr.r("2"), Svg.Attr.fill("red")],
          [],
        ),
      ],
    );
  switch (shape) {
  | V.Node({ namespace, facts, kids, _ }) =>
    assert(namespace == Some("http://www.w3.org/2000/svg"));
    assert(facts == [V.Attribute("viewBox", "0 0 10 10")]);
    switch (kids[0]) {
    | V.Node({ namespace, facts, _ }) =>
      assert(namespace == Some(Svg.namespace));
      assert(
        facts
        == [
             V.Attribute_ns(
               "http://www.w3.org/1999/xlink",
               "xlink:href",
               "#shape",
             ),
           ],
      );
    | _ => assert(false)
    };
  | _ => assert(false)
  };
  equal(
    "svg",
    "<svg viewBox=\"0 0 10 10\"><use xlink:href=\"#shape\"></use><circle cx=\"5\" r=\"2\" fill=\"red\"></circle></svg>",
    render(shape),
  );
  List.iter(
    make =>
      switch (make("1")) {
      | V.Attribute(_, "1") => ()
      | _ => assert(false)
      },
    [
      Svg.Attr.x,
      Svg.Attr.y,
      Svg.Attr.width,
      Svg.Attr.height,
      Svg.Attr.d,
      Svg.Attr.stroke,
      Svg.Attr.stroke_width,
      Svg.Attr.transform,
      Svg.Attr.points,
    ],
  );
};

let () = {
  equal(
    "escaping",
    "<p title=\"&lt;&amp;&quot;&#39;&gt;\">&lt;&amp;&quot;&#39;&gt;</p>",
    render(Html.p([Attr.title("<&\"'>")], [Html.text("<&\"'>")])),
  );
  equal(
    "voids",
    "<div><br><hr><img src=\"x\"></div>",
    render(
      Html.div(
        [],
        [
          Html.br([], []),
          Html.hr([], []),
          Html.img([Attr.src("x")], [Html.text("ignored")]),
        ],
      ),
    ),
  );
  equal(
    "style order",
    "<div style=\"color:red;padding:2px;color:blue;\"></div>",
    render(
      Html.div(
        [
          Attr.style("color", "red"),
          Attr.style("padding", "2px"),
          Attr.style("color", "blue"),
        ],
        [],
      ),
    ),
  );
  equal(
    "raw style merged",
    "<div style=\"display:block;color:red;\"></div>",
    render(
      Html.div(
        [
          Attr.attribute("style", "display:block"),
          Attr.style("color", "red"),
        ],
        [],
      ),
    ),
  );
  equal(
    "duplicate attr",
    "<div id=\"last\" class=\"a\"></div>",
    render(
      Html.div([Attr.id("first"), Attr.class_("a"), Attr.id("last")], []),
    ),
  );
  equal(
    "property aliases",
    "<div class=\"x\" for=\"entry\" tabindex=\"2\"></div>",
    render(
      Html.div(
        [
          Attr.property("className", V.String("x")),
          Attr.property("htmlFor", V.String("entry")),
          Attr.property("tabIndex", V.Float(2.)),
        ],
        [],
      ),
    ),
  );
  equal(
    "raw sentinel",
    "<div></div>",
    render(V.node("div", [Attr.none], [])),
  );
};

type wrapped =
  | Wrapped(msg);
type outer =
  | Outer(wrapped);
let () = {
  let calls = ref(0);
  let child =
    Html.lazy1(
      value => {
        incr(calls);
        Html.button(
          [Attr.class_("target\tactive"), Html.on_click(value)],
          [Html.text("go")],
        );
      },
      Increment,
    );
  let tree =
    Html.map(
      value => Outer(value),
      Html.map(
        value => Wrapped(value),
        Html.Keyed.ul([Attr.none, Attr.id("list")], [("stable", child)]),
      ),
    );
  equal(
    "keyed/lazy/map",
    "<ul id=\"list\"><button class=\"target\tactive\">go</button></ul>",
    render(tree),
  );
  ignore(render(tree));
  assert(calls^ == 1);
  assert(
    event(tree, Query.Class("target"), "click", null)
    == Some(Outer(Wrapped(Increment))),
  );
  assert(event(tree, Query.Text("go"), "click", null) == None); /* Root ul is first text match. */
  assert(Query.text_content(tree) == "go");
  assert(
    List.length(Query.find_all(tree, ~selector=Query.Text("go"))) == 2,
  );
  assert(
    List.length(Query.find_all(tree, ~selector=Query.Id("list"))) == 1,
  );
  assert(Query.find(tree, ~selector=Query.Class("missing")) == None);
  equal(
    "lazy2",
    "ab",
    render(Html.lazy2((a, b) => Html.text(a ++ b), "a", "b")),
  );
  equal(
    "lazy3",
    "abc",
    render(Html.lazy3((a, b, c) => Html.text(a ++ b ++ c), "a", "b", "c")),
  );
  equal(
    "keyed helpers",
    "<div><ol></ol></div>",
    render(
      Html.Keyed.div([Attr.none], [("ol", Tea_html.Keyed.ol([], []))]),
    ),
  );
};

let () = {
  let handlers = [
    ("click", Events.on_click(Increment)),
    ("dblclick", Events.on_double_click(Increment)),
    ("focus", Events.on_focus(Increment)),
    ("blur", Events.on_blur(Increment)),
    ("mousedown", Events.on_mouse_down(Increment)),
    ("mouseup", Events.on_mouse_up(Increment)),
    ("mouseenter", Events.on_mouse_enter(Increment)),
    ("mouseleave", Events.on_mouse_leave(Increment)),
    ("mousemove", Events.on_mouse_move(Increment)),
    ("mouseover", Events.on_mouse_over(Increment)),
    ("mouseout", Events.on_mouse_out(Increment)),
    (
      "prevent",
      Events.prevent_default_on("prevent", D.succeed((Increment, true))),
    ),
    (
      "stop",
      Events.stop_propagation_on("stop", D.succeed((Increment, true))),
    ),
    (
      "custom",
      Events.custom(
        "custom",
        D.succeed({
          V.message: Increment,
          stop_propagation: true,
          prevent_default: false,
        }),
      ),
    ),
    ("raw", Events.on("raw", V.Normal(D.succeed(Increment)))),
  ];
  let tree =
    Html.map(
      value => Wrapped(value),
      Html.button(List.map(((_, handler)) => handler, handlers), []),
    );
  List.iter(
    ((name, _)) =>
      assert(
        event(tree, Query.Tag("button"), name, null)
        == Some(Wrapped(Increment)),
      ),
    handlers,
  );
  equal("events omitted", "<button></button>", render(tree));
  assert(
    event(
      Html.button(
        [Events.on_click(Increment), Events.on_click(Decrement)],
        [],
      ),
      Query.Tag("button"),
      "click",
      null,
    )
    == Some(Decrement),
  );
  assert(
    event(
      Html.button([Events.custom("click", D.fail("ignored"))], []),
      Query.Tag("button"),
      "click",
      null,
    )
    == None,
  );
  assert(
    event(
      Html.button(
        [Events.stop_propagation_on("click", D.fail("ignored"))],
        [],
      ),
      Query.Tag("button"),
      "click",
      null,
    )
    == None,
  );
};
