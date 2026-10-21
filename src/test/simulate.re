module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;

let run = (handler, event) => {
  let decoded =
    switch (handler) {
    | V.Normal(decoder) => D.run(decoder, event)
    | V.May_stop_propagation(decoder)
    | V.May_prevent_default(decoder) =>
      D.run(D.map(((msg, _)) => msg, decoder), event)
    | V.Custom(decoder) =>
      D.run(D.map(value => value.V.message, decoder), event)
    };
  switch (decoded) {
  | Ok(msg) => Some(msg)
  | Error(_) => None
  };
};

let rec dispatch: type msg. (V.t(msg), string, D.Value.t) => option(msg) =
        (tree, name, event) =>
  switch (tree) {
  | V.Tagger(tagger) =>
    Option.map(tagger.tagger, dispatch(tagger.child, name, event))
  | V.Thunk(thunk) => dispatch(V.force(thunk), name, event)
  | V.Node(element) => facts(element.facts, name, event)
  | V.Keyed(element) => facts(element.facts, name, event)
  | V.Text(_) => None
  }
and facts: type msg. (list(V.fact(msg)), string, D.Value.t) => option(msg) =
    (facts, name, event) => {
  let handler =
    List.fold_left(
      (found, fact) =>
        switch (fact) {
        | V.Event(key, handler) when key == name => Some(handler)
        | _ => found
        },
      None,
      facts,
    );
  Option.bind(handler, handler => run(handler, event));
};

let event = (tree, ~selector, ~event_name, ~event) =>
  Option.bind(Query.find(tree, ~selector), node =>
    dispatch(node, event_name, event)
  );
