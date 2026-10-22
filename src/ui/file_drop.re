module Html = Tea_html.Html;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
external cast: 'a => 'b = "%identity";
let present = value => !Js.testAny(value);
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
         if (String.starts_with(~prefix=".", token)) {
           String.length(token) > 1 && String.ends_with(~suffix=token, name);
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
/* A FileList is array-like, not an array: read it by index. */
let files = raw => {
  let n = Ui_dom.get(raw, "length");
  if (!present(raw) || Js.typeof(n) != "number") {
    [];
  } else {
    let rec loop = (i, acc) =>
      if (i < 0) {
        acc;
      } else {
        let value = Ui_dom.get(raw, string_of_int(i));
        switch (Tea_dom.Dom_decode.run(file_decoder, cast(value))) {
        | Ok((name, (size, mime, last_modified))) =>
          loop(
            i - 1,
            [
              {
                handle: cast(value),
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
    loop((cast(n): int) - 1, []);
  };
};
let has_files = event => {
  let transfer = Ui_dom.get(event, "dataTransfer");
  if (!present(transfer)) {
    false;
  } else {
    switch (
      Tea_dom.Dom_decode.run(
        Tea_core.Decode.field(
          "types",
          Tea_core.Decode.list(Tea_core.Decode.string),
        ),
        cast(transfer),
      )
    ) {
    | Ok(types) => List.exists(String.equal("Files"), types)
    | Error(_) => false
    };
  };
};
let route = (model, name, event) => {
  let part = Ui_attr.part_id(model.id);
  let within = id => Ui_dom.within(part(id), Ui_dom.target(event));
  if (model.disabled) {
    None;
  } else if (String.equal(name, "click") && within("button")) {
    Option.iter(Ui_dom.click, Ui_dom.find(part("input")));
    None;
  } else if (String.equal(name, "change") && within("input")) {
    let target = Ui_dom.get(event, "target");
    let selected = Ui_dom.get(target, "files");
    let snapshot = present(selected) ? files(selected) : [];
    Ui_dom.set_value(cast(target), "");
    Some(Receive(snapshot));
  } else if (within("root") && has_files(event)) {
    switch (name) {
    | "dragenter" =>
      Ui_dom.prevent(event);
      Some(Enter);
    | "dragover" =>
      Ui_dom.prevent(event);
      Some(Over);
    | "dragleave" => Some(Leave)
    | "drop" =>
      Ui_dom.prevent(event);
      let selected = Ui_dom.get(Ui_dom.get(event, "dataTransfer"), "files");
      Some(Receive(present(selected) ? files(selected) : []));
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
                /* Picker activation and file snapshots must happen inside
                   the native event, not a deferred command. */
                Ui_dom.on(
                  Ui_dom.document,
                  name,
                  dispatch,
                )
              },
          ),
        ),
      ["click", "change", "dragenter", "dragover", "dragleave", "drop"],
    ),
  );
let view = (~model, ~to_parent, ~to_view, ()) => {
  /* Events are handled by subscriptions; to_parent only fixes the type. */
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
