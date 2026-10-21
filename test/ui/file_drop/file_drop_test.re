open Tea;
open Test_support;
module F = Ui.File_drop;
[@mel.new] [@mel.scope "window"]
external make_file: (array(string), string, Js.t({..})) => Js.Json.t =
  "File";
[@mel.scope "Object"]
external define: ('a, string, Js.t({..})) => unit = "defineProperty";
external cast: 'a => 'b = "%identity";
type msg =
  | Drop(F.msg)
  | Replace(F.model)
  | Disable(bool)
  | Mounted(bool)
  | Reset;
type model = {
  drop: F.model,
  outcomes: list(F.out_msg),
  mounted: bool,
};
let update = (msg, model) =>
  switch (msg) {
  | Drop(msg) =>
    let (drop, cmd, outcomes) = F.update(msg, model.drop);
    (
      {
        ...model,
        drop,
        outcomes: model.outcomes @ outcomes,
      },
      Cmd.map(x => Drop(x), cmd),
    );
  | Replace(drop) => (
      {
        ...model,
        drop,
        outcomes: [],
      },
      Cmd.none,
    )
  | Disable(value) =>
    let (drop, cmd) = F.set_disabled(value, model.drop);
    (
      {
        ...model,
        drop,
      },
      Cmd.map(x => Drop(x), cmd),
    );
  | Reset =>
    let (drop, cmd) = F.reset_dragging(model.drop);
    (
      {
        ...model,
        drop,
      },
      Cmd.map(x => Drop(x), cmd),
    );
  | Mounted(mounted) => (
      {
        ...model,
        mounted,
      },
      Cmd.none,
    )
  };
let view = model =>
  Html.div(
    [],
    [
      Html.button([Html.id("outside")], [Html.text("Outside")]),
      model.mounted
        ? F.view(
            ~model=model.drop,
            ~to_parent=x => Drop(x),
            ~to_view=
              parts =>
                Html.div(
                  parts.root,
                  [
                    Html.span(
                      [Html.id("child")],
                      [
                        Html.span(
                          [Html.id("grandchild")],
                          [Html.text("Drop here")],
                        ),
                      ],
                    ),
                    Html.button(parts.button, [Html.text("Choose files")]),
                    Html.input(parts.input, []),
                  ],
                ),
            (),
          )
        : Html.text("Unmounted"),
    ],
  );
let file = (~mime="text/plain", ~modified=1700000000123., name, content) =>
  make_file(
    [|content|],
    name,
    {
      "type": mime,
      "lastModified": modified,
    },
  );
let drag = (~types=[|"Files"|], ~flush=true, ~files=[||], selector, name) => {
  let e =
    event(
      name,
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  put(
    e,
    "dataTransfer",
    {
      "types": types,
      "files": files,
    },
  );
  let result = dispatch(node(selector), e);
  if (flush) {
    frame();
  };
  result;
};
let choose = files => {
  define(
    node("#drop-input"),
    "files",
    {
      "value": files,
      "configurable": true,
    },
  );
  ignore(emit("#drop-input", "change"));
};
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              drop: F.init(~id="drop", ()),
              outcomes: [],
              mounted: true,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          model.mounted
            ? Sub.map(x => Drop(x), F.subscriptions(model.drop)) : Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  let replace = model => {
    app.dispatch(Replace(model));
    frame();
  };
  let outcomes = () => app.model().outcomes;
  let dragging = () => F.dragging_over(app.model().drop);
  check(
    "native button semantics",
    attr("#drop-button", "type") == Some("button")
    && attr("#drop-button", "role") == None,
  );
  check(
    "valid controls idref",
    attr("#drop-button", "aria-controls") == Some("drop-input"),
  );
  check(
    "hidden native input",
    get(node("#drop-input"), "hidden")
    && attr("#drop-input", "type") == Some("file")
    && get(node("#drop-input"), "multiple") == false,
  );
  check(
    "no initial dragging",
    !dragging() && attr("#drop-root", "data-dragging-over") == None,
  );
  check("one listener per delegated event", listener_count() == 6);
  check(
    "outside file drag untouched",
    drag("#outside", "dragover") && !dragging(),
  );
  List.iter(
    name =>
      check(
        "text-only drag untouched",
        drag(~types=[|"text/plain"|], "#child", name),
      ),
    ["dragenter", "dragover", "dragleave", "drop"],
  );
  check("non-file drag never changes state", !dragging() && outcomes() == []);
  check("missing dataTransfer untouched", emit("#child", "dragover"));
  check(
    "file enter prevents",
    !drag(~flush=false, "#drop-root", "dragenter"),
  );
  check("nested enter prevents", !drag(~flush=false, "#child", "dragenter"));
  check(
    "nested grandchild",
    !drag(~flush=false, "#grandchild", "dragenter"),
  );
  check(
    "leave keeps browser default",
    drag(~flush=false, "#grandchild", "dragleave"),
  );
  frame();
  check(
    "batched depth uses latest model",
    dragging() && attr("#drop-root", "data-dragging-over") == Some(""),
  );
  ignore(drag("#child", "dragleave"));
  check("nested leave keeps root active", dragging());
  check("dragover permits file drop", !drag("#child", "dragover"));
  ignore(drag("#drop-root", "dragleave"));
  ignore(drag("#drop-root", "dragleave"));
  check(
    "extra leaves clamp zero",
    !dragging() && attr("#drop-root", "data-dragging-over") == None,
  );
  ignore(drag("#child", "dragenter"));
  app.dispatch(Reset);
  frame();
  check("programmatic reset", !dragging() && outcomes() == []);
  let a = file("notes.TXT", "abc");
  let b = file(~mime="image/png", "photo.png", "png");
  check(
    "drop prevented even without preceding enter",
    !drag(~files=[|a|], "#child", "drop"),
  );
  switch (outcomes()) {
  | [F.Accepted([f])] =>
    check(
      "File metadata and opaque handle preserved",
      f.name == "notes.TXT"
      && f.size == 3.
      && f.mime == "text/plain"
      && f.last_modified == 1700000000123.
      && cast(f.handle) === a,
    )
  | _ => check("accepted output", false)
  };
  replace(
    F.init(
      ~id="drop",
      ~multiple=true,
      ~accept=[" image/*, .tXt ", "", "application/pdf"],
      ~max_size=3.,
      ~max_count=2,
      (),
    ),
  );
  check(
    "native options",
    get(node("#drop-input"), "multiple")
    && attr("#drop-input", "accept")
    == Some(" image/*, .tXt ,,application/pdf"),
  );
  let wrong = file(~mime="application/json", "wrong.json", "{}");
  let big = file("big.txt", "1234");
  let extra = file(~mime="application/pdf", "extra.pdf", "pdf");
  ignore(drag("#child", "dragenter"));
  ignore(drag("#grandchild", "dragenter"));
  check(
    "mixed drop prevented",
    !drag(~files=[|wrong, a, big, b, extra|], "#child", "drop"),
  );
  check(
    "drop resets all nesting",
    !dragging() && attr("#drop-root", "data-dragging-over") == None,
  );
  switch (outcomes()) {
  | [
      F.Accepted([a, b]),
      F.Rejected([
        (wrong, F.Not_accepted),
        (big, F.Too_large),
        (extra, F.Too_many),
      ]),
    ] =>
    check(
      "ordered partition with invalid files not consuming quota",
      a.name == "notes.TXT"
      && b.name == "photo.png"
      && wrong.name == "wrong.json"
      && big.name == "big.txt"
      && extra.name == "extra.pdf",
    )
  | _ => check("mixed partition", false)
  };
  replace(
    F.init(
      ~id="drop",
      ~multiple=true,
      ~accept=[".tar.gz", "TEXT/PLAIN"],
      (),
    ),
  );
  choose([|
    file(~mime="", "ARCHIVE.TAR.GZ", "zip"),
    file(~mime="text/plain", "no-extension", "x"),
    b,
  |]);
  switch (outcomes()) {
  | [F.Accepted([_, _]), F.Rejected([(_, F.Not_accepted)])] => ()
  | _ => check("extension suffix/exact MIME/unknown MIME", false)
  };
  replace(F.init(~id="drop", ~max_count=20, ()));
  choose([|a, b|]);
  switch (outcomes()) {
  | [F.Accepted([_]), F.Rejected([(_, F.Too_many)])] => ()
  | _ => check("single-file hard cap", false)
  };
  check("change clears value", get(node("#drop-input"), "value") == "");
  choose([|a|]);
  check(
    "repeat file selection emits new batch",
    List.length(outcomes()) == 3,
  );
  choose([||]);
  check("cancel/empty selection no outcome", List.length(outcomes()) == 3);
  replace(
    F.init(~id="drop", ~multiple=true, ~max_count=-1, ~max_size=-1., ()),
  );
  choose([|a, file("empty", "")|]);
  switch (outcomes()) {
  | [F.Rejected([(_, F.Too_large), (_, F.Too_many)])] => ()
  | _ => check("negative limits clamp and deterministic precedence", false)
  };
  replace(F.init(~id="drop", ~accept=["image/*"], ()));
  choose([|
    file(~mime="", "photo.png", "x"),
    file(~mime="image/", "broken", "x"),
  |]);
  switch (outcomes()) {
  | [F.Rejected([(_, F.Not_accepted), (_, F.Not_accepted)])] => ()
  | _ => check("wildcard requires actual MIME subtype", false)
  };
  replace(F.init(~id="drop", ~capture=F.User, ()));
  check("front capture", attr("#drop-input", "capture") == Some("user"));
  replace(F.init(~id="drop", ~capture=F.Environment, ()));
  check(
    "rear capture",
    attr("#drop-input", "capture") == Some("environment"),
  );
  replace(F.init(~id="drop", ()));
  check("capture omitted", attr("#drop-input", "capture") == None);
  let large = file("large", "x");
  define(large, "size", { "value": 3000000000. });
  choose([|large|]);
  switch (outcomes()) {
  | [F.Accepted([f])] =>
    check("large size not truncated", f.size == 3000000000.)
  | _ => check("large File", false)
  };
  let input = node("#drop-input");
  let original_click = get(input, "click");
  let opened = ref(0);
  put(input, "click", () => incr(opened));
  focus(node("#drop-button"));
  check("button focus", has_focus("#drop-button"));
  let e =
    event(
      "click",
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  ignore(dispatch(node("#drop-button"), e));
  check(
    "picker opens synchronously before render",
    opened^ == 1 && has_focus("#drop-button"),
  );
  check(
    "native keys untouched",
    key("#drop-button", "keydown", "Enter")
    && key("#drop-button", "keydown", " ")
    && key("#drop-button", "keyup", " ")
    && key("#drop-button", "keydown", "Tab")
    && key(~ctrl=true, "#drop-button", "keydown", "Enter")
    && key(~composing=true, "#drop-button", "keydown", "Enter"),
  );
  check("jsdom native keys not falsely synthesized", opened^ == 1);
  ignore(drag("#child", "dragenter"));
  app.dispatch(Disable(true));
  frame();
  check("disable clears drag", !dragging() && F.disabled(app.model().drop));
  check(
    "native disabled state",
    get(node("#drop-button"), "disabled")
    && get(node("#drop-input"), "disabled")
    && attr("#drop-root", "data-disabled") == Some(""),
  );
  let before = List.length(outcomes());
  click("#drop-button");
  choose([|a|]);
  List.iter(
    name =>
      check(
        "disabled file events untouched",
        drag(~files=[|a|], "#child", name),
      ),
    ["dragenter", "dragover", "drop", "dragleave"],
  );
  check(
    "disabled no picker or outcomes",
    opened^ == 1 && List.length(outcomes()) == before && !dragging(),
  );
  app.dispatch(Disable(false));
  frame();
  click("#drop-button");
  check(
    "latest subscription state re-enables synchronously",
    opened^ == 2
    && listener_count() == 6
    && attr("#drop-root", "data-disabled") == None,
  );
  put(input, "click", original_click);
  let paste =
    event(
      "paste",
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  put(paste, "clipboardData", { "files": [|a|] });
  check(
    "paste explicitly excluded",
    dispatch(node("#drop-root"), paste) && List.length(outcomes()) == before,
  );
  app.dispatch(Mounted(false));
  frame();
  check("unmount removes delegated listeners", listener_count() == 0);
  app.dispatch(Mounted(true));
  frame();
  check("remount installs once", listener_count() == 6);
  choose([|a|]);
  check(
    "replacement parts still routed",
    List.length(outcomes()) == before + 1,
  );
  app.shutdown();
  frame();
  check(
    "shutdown removes listeners",
    listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
  );
  report("file_drop");
  Js.Promise.resolve();
};
