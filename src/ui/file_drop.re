module Html = Tea_html.Html;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module B = {
  type raw;
  external document: Dom.document = "document";
  [@mel.get_index] external get: ('a, string) => raw;
  external cast: 'a => 'b = "%identity";
  [@mel.send]
  external find: (Dom.document, string) => Js.Nullable.t(Dom.node) =
    "getElementById";
  [@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
  [@mel.send]
  external listen: (Dom.document, string, raw => unit) => unit =
    "addEventListener";
  [@mel.send]
  external unlisten: (Dom.document, string, raw => unit) => unit =
    "removeEventListener";
  [@mel.send] external prevent: raw => unit = "preventDefault";
  [@mel.send] external click: Dom.node => unit = "click";
  [@mel.set] external clear: (Dom.node, string) => unit = "value";
  let present = x => !Js.testAny(x);
  let within = (id, event) => {
    let target = get(event, "target");
    switch (Js.Nullable.toOption(find(document, id))) {
    | Some(root)
        when
          present(target) && Js.typeof(get(target, "nodeType")) == "number" =>
      contains(root, cast(target))
    | _ => false
    };
  };
};
type handle;
type file = {
  handle,
  name: string,
  size: float,
  mime: string,
  last_modified: float,
};
type rejection =
  | Not_accepted
  | Too_large
  | Too_many;
type capture =
  | User
  | Environment;
type out_msg =
  | Accepted(list(file))
  | Rejected(list((file, rejection)));
type model = {
  id: string,
  depth: int,
  disabled: bool,
  multiple: bool,
  accept: list(string),
  capture: option(capture),
  max_size: option(float),
  max_count: option(int),
};
type msg =
  | Enter
  | Leave
  | Over
  | Receive(list(file));
type parts('parent) = {
  root: list(Html.attr('parent)),
  button: list(Html.attr('parent)),
  input: list(Html.attr('parent)),
  dragging_over: bool,
};
let init =
    (
      ~id,
      ~disabled=false,
      ~multiple=false,
      ~accept=[],
      ~capture=?,
      ~max_size=?,
      ~max_count=?,
      (),
    ) => {
  id,
  depth: 0,
  disabled,
  multiple,
  accept,
  capture,
  max_size: Option.map(x => max(0., x), max_size),
  max_count: Option.map(x => max(0, x), max_count),
};
let dragging_over = model => model.depth > 0;
let disabled = model => model.disabled;
let set_disabled = (disabled, model) => (
  {
    ...model,
    disabled,
    depth: 0,
  },
  Cmd.none,
);
let reset_dragging = model => (
  {
    ...model,
    depth: 0,
  },
  Cmd.none,
);
let tokens = model =>
  List.filter(
    x => !String.equal(x, ""),
    List.map(
      x => String.lowercase_ascii(String.trim(x)),
      List.concat_map(String.split_on_char(','), model.accept),
    ),
  );
let starts = (s, prefix) =>
  String.length(s) >= String.length(prefix)
  && String.equal(String.sub(s, 0, String.length(prefix)), prefix);
let ends = (s, suffix) =>
  String.length(s) >= String.length(suffix)
  && String.equal(
       String.sub(
         s,
         String.length(s) - String.length(suffix),
         String.length(suffix),
       ),
       suffix,
     );
let mime_token = token =>
  !String.equal(token, "")
  && String.for_all(
       c =>
         switch (c) {
         | 'a' .. 'z'
         | '0' .. '9'
         | '!'
         | '#'
         | '$'
         | '%'
         | '&'
         | '\''
         | '+'
         | '-'
         | '.'
         | '^'
         | '_'
         | '`'
         | '|'
         | '~' => true
         | _ => false
         },
       token,
     );
let accepts = (tokens, file) => {
  let name = String.lowercase_ascii(file.name);
  let mime = String.lowercase_ascii(file.mime);
  tokens == []
  || List.exists(
       token =>
         if (starts(token, ".")) {
           String.length(token) > 1 && ends(name, token);
         } else {
           switch (
             String.split_on_char('/', token),
             String.split_on_char('/', mime),
           ) {
           | ([major, minor], [file_major, file_minor]) =>
             mime_token(major)
             && mime_token(file_minor)
             && String.equal(major, file_major)
             && (
               String.equal(minor, "*")
               || mime_token(minor)
               && String.equal(minor, file_minor)
             )
           | _ => false
           };
         },
       tokens,
     );
};
let update = (msg, model) =>
  if (model.disabled) {
    (model, Cmd.none, []);
  } else {
    switch (msg) {
    | Enter => (
        {
          ...model,
          depth: model.depth + 1,
        },
        Cmd.none,
        [],
      )
    | Leave => (
        {
          ...model,
          depth: max(0, model.depth - 1),
        },
        Cmd.none,
        [],
      )
    | Over => (model, Cmd.none, [])
    | Receive(files) =>
      let limit =
        model.multiple
          ? Option.value(~default=max_int, model.max_count)
          : min(1, Option.value(~default=1, model.max_count));
      let rules = tokens(model);
      let rec collect = (files, count, accepted, rejected) =>
        switch (files) {
        | [] => (List.rev(accepted), List.rev(rejected))
        | [file, ...rest] =>
          let reason =
            if (!accepts(rules, file)) {
              Some(Not_accepted);
            } else if (Option.fold(
                         ~none=false,
                         ~some=limit => file.size > limit,
                         model.max_size,
                       )) {
              Some(Too_large);
            } else if (count >= limit) {
              Some(Too_many);
            } else {
              None;
            };
          switch (reason) {
          | Some(reason) =>
            collect(rest, count, accepted, [(file, reason), ...rejected])
          | None => collect(rest, count + 1, [file, ...accepted], rejected)
          };
        };
      let (accepted, rejected) = collect(files, 0, [], []);
      (
        {
          ...model,
          depth: 0,
        },
        Cmd.none,
        (accepted == [] ? [] : [Accepted(accepted)])
        @ (rejected == [] ? [] : [Rejected(rejected)]),
      );
    };
  };
let file_decoder =
  Tea_core.Decode.(
    map2(
      (a, b) => (a, b),
      field("name", string),
      map3(
        (size, mime, modified) => (size, mime, modified),
        field("size", float),
        field("type", string),
        field("lastModified", float),
      ),
    )
  );
let files = raw => {
  let n = B.get(raw, "length");
  if (!B.present(raw) || Js.typeof(n) != "number") {
    [];
  } else {
    let rec loop = (i, acc) =>
      if (i < 0) {
        acc;
      } else {
        let value = B.get(raw, string_of_int(i));
        switch (Tea_dom.Dom_decode.run(file_decoder, B.cast(value))) {
        | Ok((name, (size, mime, last_modified))) =>
          loop(
            i - 1,
            [
              {
                handle: B.cast(value),
                name,
                size,
                mime,
                last_modified,
              },
              ...acc,
            ],
          )
        | Error(_) => loop(i - 1, acc)
        };
      };
    loop((B.cast(n): int) - 1, []);
  };
};
let has_files = event => {
  let transfer = B.get(event, "dataTransfer");
  if (!B.present(transfer)) {
    false;
  } else {
    switch (
      Tea_dom.Dom_decode.run(
        Tea_core.Decode.field(
          "types",
          Tea_core.Decode.list(Tea_core.Decode.string),
        ),
        B.cast(transfer),
      )
    ) {
    | Ok(types) => List.exists(String.equal("Files"), types)
    | Error(_) => false
    };
  };
};
let route = (model, name, event) => {
  let part = Ui_attr.part_id(model.id);
  if (model.disabled) {
    None;
  } else if (String.equal(name, "click") && B.within(part("button"), event)) {
    Option.iter(
      input => B.click(input),
      Js.Nullable.toOption(B.find(B.document, part("input"))),
    );
    None;
  } else if (String.equal(name, "change") && B.within(part("input"), event)) {
    let target = B.get(event, "target");
    let selected = B.get(target, "files");
    let snapshot = B.present(selected) ? files(selected) : [];
    B.clear(B.cast(target), "");
    Some(Receive(snapshot));
  } else if (B.within(part("root"), event) && has_files(event)) {
    switch (name) {
    | "dragenter" =>
      B.prevent(event);
      Some(Enter);
    | "dragover" =>
      B.prevent(event);
      Some(Over);
    | "dragleave" => Some(Leave)
    | "drop" =>
      B.prevent(event);
      let selected = B.get(B.get(event, "dataTransfer"), "files");
      Some(Receive(B.present(selected) ? files(selected) : []));
    | _ => None
    };
  } else {
    None;
  };
};
let subscriptions = model =>
  Sub.batch(
    List.map(
      name =>
        Sub.filter_map(
          event => route(model, name, event),
          Sub.make(
            ~home="File_drop.events." ++ name,
            ~key=model.id,
            ~tagger=x => x,
            ~start=
              dispatch => {
                /* Delegation survives replacement of parts. Filtered's newest tagger
                   reads current state; file snapshots and picker activation happen in
                   the native event stack, not in a deferred DOM command. */
                B.listen(B.document, name, dispatch);
                () => B.unlisten(B.document, name, dispatch);
              },
          ),
        ),
      ["click", "change", "dragenter", "dragover", "dragleave", "drop"],
    ),
  );
let view = (~model, ~to_parent, ~to_view, ()) => {
  /* Events are delegated by subscriptions; attribute bundles carry no stale
     state closures. to_parent is deliberately used only for the type boundary. */
  let map = List.map(Tea_vdom.Vdom.map_fact(to_parent));
  let part = Ui_attr.part_id(model.id);
  to_view({
    root:
      map([
        Html.id(part("root")),
        Ui_attr.data_flag("dragging-over", dragging_over(model)),
        Ui_attr.data_flag("disabled", model.disabled),
      ]),
    button:
      map([
        Html.id(part("button")),
        Html.type_("button"),
        Html.disabled(model.disabled),
        Ui_attr.data_flag("disabled", model.disabled),
        Html.aria("controls", part("input")),
      ]),
    input:
      map([
        Html.id(part("input")),
        Html.type_("file"),
        Html.hidden(true),
        Html.tabindex(-1),
        Html.disabled(model.disabled),
        Html.multiple(model.multiple),
        Html.attribute("accept", String.concat(",", model.accept)),
        switch (model.capture) {
        | None => Html.none
        | Some(User) => Html.attribute("capture", "user")
        | Some(Environment) => Html.attribute("capture", "environment")
        },
      ]),
    dragging_over: dragging_over(model),
  });
};
