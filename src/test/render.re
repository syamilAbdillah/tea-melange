module V = Tea_vdom.Vdom;

let escape = value => {
  let buffer = Buffer.create(String.length(value));
  String.iter(
    c =>
      switch (c) {
      | '&' => Buffer.add_string(buffer, "&amp;")
      | '<' => Buffer.add_string(buffer, "&lt;")
      | '>' => Buffer.add_string(buffer, "&gt;")
      | '"' => Buffer.add_string(buffer, "&quot;")
      | '\'' => Buffer.add_string(buffer, "&#39;")
      | _ => Buffer.add_char(buffer, c)
      },
    value,
  );
  Buffer.contents(buffer);
};

let property_name = name =>
  switch (name) {
  | "readOnly" => "readonly"
  | "autoFocus" => "autofocus"
  | "className" => "class"
  | "htmlFor" => "for"
  | "tabIndex" => "tabindex"
  | name => name
  };

let attributes = facts => {
  let rec set = (name, value, entries) =>
    switch (entries) {
    | [] => [(name, value)]
    | [(key, _) as entry, ...rest] =>
      key == name
        ? [(name, value), ...rest] : [entry, ...set(name, value, rest)]
    };
  let (entries, styles) =
    List.fold_left(
      ((entries, styles), fact) =>
        switch (fact) {
        | V.Attribute("", _) => (entries, styles)
        | V.Attribute("style", value) => (
            set("style", None, entries),
            styles @ [value],
          )
        | V.Attribute(name, value)
        | V.Attribute_ns(_, name, value) => (
            set(name, Some(value), entries),
            styles,
          )
        | V.Property(name, value) =>
          let value =
            switch (value) {
            | V.String(value) => Some(value)
            | V.Float(value) => Some(Printf.sprintf("%.17g", value))
            | V.Bool(value) => value ? Some("") : None
            };
          (set(property_name(name), value, entries), styles);
        | V.Style(key, value) => (
            set("style", None, entries),
            styles @ [key ++ ":" ++ value ++ ";"],
          )
        | V.Event(_, _) => (entries, styles)
        },
      ([], []),
      facts,
    );
  let entries =
    styles == []
      ? entries
      : set(
          "style",
          Some(
            String.concat(
              "",
              List.map(
                s => String.ends_with(~suffix=";", s) ? s : s ++ ";",
                styles,
              ),
            ),
          ),
          entries,
        );
  String.concat(
    "",
    List.filter_map(
      ((name, value)) =>
        Option.map(
          value => " " ++ name ++ "=\"" ++ escape(value) ++ "\"",
          value,
        ),
      entries,
    ),
  );
};

let void_tags = [
  "area",
  "base",
  "br",
  "col",
  "embed",
  "hr",
  "img",
  "input",
  "link",
  "meta",
  "param",
  "source",
  "track",
  "wbr",
];

let to_string = tree => {
  let buffer = Buffer.create(256);
  let rec render: type msg. V.t(msg) => unit = tree =>
    switch (tree) {
    | V.Text(text) => Buffer.add_string(buffer, escape(text))
    | V.Thunk(thunk) => render(V.force(thunk))
    | V.Tagger(tagger) => render(tagger.child)
    | V.Node(element) =>
      element_(element.tag, element.namespace, element.facts, () =>
        Array.iter(render, element.kids)
      )
    | V.Keyed(element) =>
      element_(element.tag, element.namespace, element.facts, () =>
        Array.iter(((_, child)) => render(child), element.kids)
      )
    }
  and element_
      : type msg.
          (string, option(string), list(V.fact(msg)), unit => unit) => unit =
      (tag, namespace, facts, children) => {
    Buffer.add_string(buffer, "<" ++ tag ++ attributes(facts) ++ ">");
    if (!(namespace == None && List.mem(tag, void_tags))) {
      children();
      Buffer.add_string(buffer, "</" ++ tag ++ ">");
    };
  };
  render(tree);
  Buffer.contents(buffer);
};
