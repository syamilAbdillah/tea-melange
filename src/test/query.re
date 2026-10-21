module V = Tea_vdom.Vdom;
type selector =
  | Tag(string)
  | Id(string)
  | Class(string)
  | Text(string);

let rec text_content: type msg. V.t(msg) => string = tree =>
  switch (tree) {
  | V.Text(value) => value
  | V.Thunk(thunk) => text_content(V.force(thunk))
  | V.Tagger(tagger) => text_content(tagger.child)
  | V.Node(element) =>
    String.concat("", Array.to_list(Array.map(text_content, element.kids)))
  | V.Keyed(element) =>
    String.concat(
      "",
      Array.to_list(
        Array.map(((_, child)) => text_content(child), element.kids),
      ),
    )
  };

let fact_value = (name, facts) =>
  List.fold_left(
    (value, fact) =>
      switch (fact) {
      | V.Attribute(key, value) when key == name => Some(value)
      | V.Property(key, V.String(value))
          when key == name || name == "class" && key == "className" =>
        Some(value)
      | _ => value
      },
    None,
    facts,
  );

let classes = value =>
  String.split_on_char(
    ' ',
    String.map(
      c =>
        switch (c) {
        | '\t'
        | '\n'
        | '\r'
        | '\012' => ' '
        | _ => c
        },
      value,
    ),
  );

let matches = (selector, tree, tag, facts) =>
  switch (selector) {
  | Tag(name) => tag == name
  | Id(value) => fact_value("id", facts) == Some(value)
  | Class(value) =>
    switch (fact_value("class", facts)) {
    | None => false
    | Some(names) => value != "" && List.mem(value, classes(names))
    }
  | Text(value) => text_content(tree) == value
  };

let rec find_all
        : type msg. (V.t(msg), ~selector: selector) => list(V.t(msg)) =
        (tree, ~selector) =>
  switch (tree) {
  | V.Text(_) => []
  | V.Thunk(thunk) => find_all(V.force(thunk), ~selector)
  | V.Tagger(tagger) =>
    List.map(V.map(tagger.tagger), find_all(tagger.child, ~selector))
  | V.Node(element) =>
    let children =
      List.concat_map(
        child => find_all(child, ~selector),
        Array.to_list(element.kids),
      );
    matches(selector, tree, element.tag, element.facts)
      ? [tree, ...children] : children;
  | V.Keyed(element) =>
    let children =
      List.concat_map(
        ((_, child)) => find_all(child, ~selector),
        Array.to_list(element.kids),
      );
    matches(selector, tree, element.tag, element.facts)
      ? [tree, ...children] : children;
  };
let find = (tree, ~selector) => List.nth_opt(find_all(tree, ~selector), 0);
