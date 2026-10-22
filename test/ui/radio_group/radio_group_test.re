open Tea;
open Test_support;
module R = Ui.Radio_group;
let entries: array(R.item) = [|
  {
    key: "a",
    label: "Daily",
    disabled: false,
  },
  {
    key: "b",
    label: "Unavailable",
    disabled: true,
  },
  {
    key: "c",
    label: "Weekly",
    disabled: false,
  },
  {
    key: "d",
    label: "Never",
    disabled: false,
  },
|];
type model = {
  group: R.model,
  items: array(R.item),
  out: list(R.out_msg),
  hooks: int,
};
type msg =
  | Child(R.msg)
  | Items(array(R.item))
  | Raw_items(array(R.item))
  | Set(option(string))
  | Config(R.direction, R.orientation, bool, bool)
  | Hook;
let update = (msg, model) =>
  switch (msg) {
  | Child(msg) =>
    let (group, cmd, out) = R.update(~items=model.items, msg, model.group);
    (
      {
        ...model,
        group,
        out: model.out @ out,
      },
      Cmd.map(msg => Child(msg), cmd),
    );
  | Raw_items(items) => (
      {
        ...model,
        items,
      },
      Cmd.none,
    )
  | Items(items) =>
    let (group, cmd) = R.reconcile(~items, model.group);
    (
      {
        ...model,
        items,
        group,
      },
      Cmd.map(msg => Child(msg), cmd),
    );
  | Set(key) =>
    let (group, cmd) = R.set_checked(~items=model.items, key, model.group);
    (
      {
        ...model,
        group,
      },
      Cmd.map(msg => Child(msg), cmd),
    );
  | Config(dir, orientation, disabled, readonly) =>
    let (group, cmd) =
      R.configure(
        ~items=model.items,
        ~dir,
        ~orientation,
        ~disabled,
        ~readonly,
        model.group,
      );
    (
      {
        ...model,
        group,
      },
      Cmd.map(msg => Child(msg), cmd),
    );
  | Hook => (
      {
        ...model,
        hooks: model.hooks + 1,
      },
      Cmd.none,
    )
  };
let view = model =>
  R.view(
    ~model=model.group,
    ~items=model.items,
    ~to_parent=msg => Child(msg),
    ~description=true,
    ~required=true,
    ~invalid=true,
    ~name="schedule",
    ~on_key_down=key => String.equal(key, "Home") ? Some(Hook) : None,
    ~to_view=
      parts =>
        Html.div(
          [],
          [
            Html.h2(parts.label, [Html.text("Schedule")]),
            Html.p(
              parts.description,
              [Html.text("Choose delivery frequency")],
            ),
            Html.keyed(
              "div",
              parts.root,
              Array.to_list(
                Array.map(
                  (part: R.item_parts(msg)) =>
                    (
                      part.item.key,
                      Html.div(part.root, [Html.text(part.item.label)]),
                    ),
                  parts.items,
                ),
              ),
            ),
            Option.value(~default=Html.text(""), parts.hidden_input),
            Html.button([Html.id("outside")], [Html.text("Outside")]),
          ],
        ),
    (),
  );
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              group: R.init(~id="r", ~items=entries, ()),
              items: entries,
              out: [],
              hooks: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model => Sub.map(msg => Child(msg), R.subscriptions(model.group)),
      ~node="#app",
      ~flags=(),
      (),
    );
  let selection = () => R.checked(app.model().group);
  let focus_key = key => {
    focus(node("#r-item-" ++ key));
    frame();
  };
  check(
    "root semantics",
    attr("#r", "role") == Some("radiogroup")
    && attr("#r", "aria-orientation") == None,
  );
  check(
    "root idrefs",
    attr("#r", "aria-labelledby") == Some("r-label")
    && attr("#r-label", "id") == Some("r-label")
    && attr("#r", "aria-describedby") == Some("r-description"),
  );
  check(
    "form validation metadata",
    attr("#r", "aria-required") == Some("true")
    && attr("#r", "aria-invalid") == Some("true")
    && attr("#r", "data-invalid") == Some(""),
  );
  check(
    "unchecked entry",
    selection() == None
    && attr("#r-item-a", "tabindex") == Some("0")
    && attr("#r-item-c", "tabindex") == Some("-1"),
  );
  check(
    "disabled item",
    attr("#r-item-b", "aria-disabled") == Some("true")
    && attr("#r-item-b", "data-disabled") == Some(""),
  );
  check(
    "no hidden value when unchecked",
    Array.length(query_all(document, "input[name=schedule]")) == 0,
  );
  focus_key("a");
  check("Space prevents scroll", !key("#r-item-a", "keydown", " "));
  frame();
  check(
    "Space selects",
    selection() == Some("a")
    && app.model().out == [R.Changed("a")]
    && attr("#r-item-a", "data-checked") == Some(""),
  );
  check(
    "hidden submission",
    get(node("input[name=schedule]"), "value") == "a",
  );
  ignore(key("#r-item-a", "keydown", " "));
  frame();
  check("checked Space no duplicate", List.length(app.model().out) == 1);
  List.iter(
    ((source, keyboard, target)) => {
      check(
        "arrows claimed",
        !key("#r-item-" ++ source, "keydown", keyboard),
      );
      frame();
      check("arrow selects latest", selection() == Some(target));
      check("arrow focuses after render", has_focus("#r-item-" ++ target));
      check(
        "roving checked",
        attr("#r-item-" ++ target, "tabindex") == Some("0"),
      );
    },
    [
      ("a", "ArrowRight", "c"),
      ("c", "ArrowDown", "d"),
      ("d", "ArrowRight", "a"),
      ("a", "ArrowLeft", "d"),
      ("d", "ArrowUp", "c"),
      ("c", "ArrowLeft", "a"),
    ],
  );
  /* Intents are dispatched from the same stale rendered node, without a frame. */
  let before = List.length(app.model().out);
  ignore(key("#r-item-a", "keydown", "ArrowRight"));
  ignore(key("#r-item-a", "keydown", "ArrowRight"));
  check(
    "batched latest selection",
    selection() == Some("d") && List.length(app.model().out) == before + 2,
  );
  frame();
  check(
    "batched focus never rewinds state",
    has_focus("#r-item-d") && selection() == Some("d"),
  );
  List.iter(
    k => {
      check("ctrl untouched", key(~ctrl=true, "#r-item-d", "keydown", k));
      check("alt untouched", key(~alt=true, "#r-item-d", "keydown", k));
      check("meta untouched", key(~meta=true, "#r-item-d", "keydown", k));
      check(
        "composition untouched",
        key(~composing=true, "#r-item-d", "keydown", k),
      );
    },
    ["ArrowRight", "ArrowDown", " "],
  );
  List.iter(
    k => check("non-radio keys untouched", key("#r-item-d", "keydown", k)),
    ["Tab", "Enter", "End"],
  );
  check(
    "Shift Tab untouched",
    key(~shift=true, "#r-item-d", "keydown", "Tab"),
  );
  check(
    "hook no prevention",
    key("#r-item-d", "keydown", "Home") && app.model().hooks == 1,
  );
  app.dispatch(Config(R.Rtl, R.Horizontal, false, false));
  frame();
  ignore(key("#r-item-d", "keydown", "ArrowRight"));
  frame();
  check("RTL right previous", selection() == Some("c"));
  ignore(key("#r-item-c", "keydown", "ArrowLeft"));
  frame();
  check("RTL left next", selection() == Some("d"));
  check(
    "orientation hint only",
    attr("#r", "data-orientation") == Some("horizontal")
    && attr("#r", "dir") == Some("rtl"),
  );
  let before = List.length(app.model().out);
  app.dispatch(Set(Some("c")));
  frame();
  check(
    "setter quiet",
    selection() == Some("c") && List.length(app.model().out) == before,
  );
  app.dispatch(Config(R.Ltr, R.Vertical, false, true));
  frame();
  ignore(key("#r-item-c", "keydown", "ArrowDown"));
  frame();
  check(
    "readonly navigation",
    has_focus("#r-item-d")
    && selection() == Some("c")
    && R.focused(app.model().group) == Some("d"),
  );
  ignore(key("#r-item-d", "keydown", " "));
  click("#r-item-a");
  check(
    "readonly cannot check",
    selection() == Some("c")
    && List.length(app.model().out) == before
    && attr("#r", "aria-readonly") == Some("true"),
  );
  focus(node("#outside"));
  frame();
  check(
    "readonly exit restores checked entry",
    attr("#r-item-c", "tabindex") == Some("0")
    && attr("#r-item-a", "tabindex") == Some("-1"),
  );
  app.dispatch(Config(R.Ltr, R.Vertical, true, false));
  frame();
  focus_key("c");
  ignore(key("#r-item-c", "keydown", "ArrowDown"));
  click("#r-item-a");
  check(
    "disabled group inert but discoverable",
    selection() == Some("c")
    && attr("#r-item-c", "tabindex") == Some("0")
    && attr("#r", "aria-disabled") == Some("true")
    && get(node("input[name=schedule]"), "disabled") == true,
  );
  app.dispatch(Config(R.Ltr, R.Vertical, false, false));
  frame();
  click("#r-item-b");
  ignore(key("#r-item-b", "keydown", "ArrowDown"));
  frame();
  check("disabled radio inert", selection() == Some("c"));
  pointer("#r-item-a");
  click("#r-item-a");
  check(
    "pointer selects and focuses",
    selection() == Some("a") && has_focus("#r-item-a"),
  );
  let before = List.length(app.model().out);
  app.dispatch(Items([|entries[2], entries[3]|]));
  frame();
  check(
    "removed selection cleared and focus repaired",
    selection() == None
    && has_focus("#r-item-c")
    && List.length(app.model().out) == before,
  );
  app.dispatch(Set(Some("d")));
  frame();
  app.dispatch(
    Items([|
      {
        ...entries[3],
        disabled: true,
      },
      entries[2],
    |]),
  );
  frame();
  check(
    "disabled selection cleared",
    selection() == None && has_focus("#r-item-c"),
  );
  app.dispatch(Items([|entries[2]|]));
  frame();
  focus_key("c");
  ignore(key("#r-item-c", "keydown", "ArrowDown"));
  frame();
  check("single wraps", selection() == Some("c") && has_focus("#r-item-c"));
  app.dispatch(
    Items([|
      {
        ...entries[2],
        disabled: true,
      },
    |]),
  );
  frame();
  check(
    "all disabled no tabstop",
    selection() == None && attr("#r-item-c", "tabindex") == Some("-1"),
  );
  app.dispatch(Items([||]));
  frame();
  check(
    "empty safe",
    selection() == None && R.focused(app.model().group) == None,
  );
  check("empty focus parked on group", has_focus("#r"));
  app.dispatch(Items(entries));
  frame();
  app.dispatch(Set(Some("a")));
  frame();
  focus_key("a");
  app.dispatch(Raw_items([|entries[2], entries[3]|]));
  ignore(key("#r-item-c", "keydown", "ArrowRight"));
  frame();
  check(
    "update repairs latest collection before intent",
    selection() == Some("d") && has_focus("#r-item-d"),
  );
  app.dispatch(Items([|entries[3], entries[2]|]));
  frame();
  check(
    "reorder preserves identity",
    selection() == Some("d") && has_focus("#r-item-d"),
  );
  ignore(key(~repeat=true, "#r-item-d", "keydown", "ArrowDown"));
  frame();
  check("repeat uses latest order", selection() == Some("c"));
  focus(node("#outside"));
  frame();
  let before = List.length(app.model().out);
  app.dispatch(Items([|entries[0]|]));
  frame();
  check(
    "repair outside does not steal focus or emit",
    has_focus("#outside")
    && selection() == None
    && List.length(app.model().out) == before,
  );
  let strange: array(R.item) = [|
    {
      key: {js|a b/é|js},
      label: "Odd key",
      disabled: false,
    },
    {
      key: "a_0020b",
      label: "Escape lookalike",
      disabled: false,
    },
  |];
  app.dispatch(Items(strange));
  frame();
  let radios = query_all(document, "[role=radio]");
  check(
    "safe collision-free IDs",
    attribute(radios[0], "id") != attribute(radios[1], "id"),
  );
  app.dispatch(Set(Some("missing")));
  frame();
  check("invalid setter clears", selection() == None);
  app.dispatch(Items(entries));
  app.dispatch(Set(Some("a")));
  frame();
  focus_key("a");
  ignore(key("#r-item-a", "keydown", "ArrowRight"));
  focus(node("#outside"));
  frame();
  check("leaving before draw cancels queued focus", has_focus("#outside"));
  app.dispatch(Set(Some("a")));
  frame();
  focus_key("a");
  ignore(key("#r-item-a", "keydown", "ArrowRight"));
  app.shutdown();
  focus(node("#outside"));
  frame();
  check("shutdown cancels queued focus", has_focus("#outside"));
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("radio_group");
  Js.Promise.resolve();
};
