open Tea;
open Test_support;
module T = Ui.Tabs;
let entries: array(T.item) = [|
  {
    key: "a",
    label: "Profile",
    disabled: false,
  },
  {
    key: "b",
    label: "Unavailable",
    disabled: true,
  },
  {
    key: "c",
    label: "Privacy",
    disabled: false,
  },
  {
    key: "d",
    label: "Alerts",
    disabled: false,
  },
|];
type model = {
  tabs: T.model,
  items: array(T.item),
  out: list(T.out_msg),
  hooks: int,
  mounted: bool,
};
type msg =
  | Child(T.msg)
  | Items(array(T.item))
  | Raw_items(array(T.item))
  | Set(string)
  | Config(T.direction, T.orientation, T.activation)
  | Mounted(bool)
  | Hook;
let update = (msg, model) =>
  switch (msg) {
  | Child(msg) =>
    let (tabs, cmd, out) = T.update(~items=model.items, msg, model.tabs);
    (
      {
        ...model,
        tabs,
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
    let (tabs, cmd) = T.reconcile(~items, model.tabs);
    (
      {
        ...model,
        items,
        tabs,
      },
      Cmd.map(msg => Child(msg), cmd),
    );
  | Set(key) =>
    let (tabs, cmd) = T.set_selected(~items=model.items, key, model.tabs);
    (
      {
        ...model,
        tabs,
      },
      Cmd.map(msg => Child(msg), cmd),
    );
  | Config(dir, orientation, activation) =>
    let (tabs, cmd) =
      T.configure(
        ~items=model.items,
        ~dir,
        ~orientation,
        ~activation,
        model.tabs,
      );
    (
      {
        ...model,
        tabs,
      },
      Cmd.map(msg => Child(msg), cmd),
    );
  | Mounted(mounted) => (
      {
        ...model,
        mounted,
      },
      Cmd.none,
    )
  | Hook => (
      {
        ...model,
        hooks: model.hooks + 1,
      },
      Cmd.none,
    )
  };
let view = model =>
  T.view(
    ~model=model.tabs,
    ~items=model.items,
    ~to_parent=msg => Child(msg),
    ~panel_rendering=model.mounted ? T.Keep_mounted : T.Selected_only,
    ~panel_focusable=key => !String.equal(key, "c"),
    ~on_key_down=key => String.equal(key, "Delete") ? Some(Hook) : None,
    ~to_view=
      parts =>
        Html.div(
          [],
          [
            Html.h2(parts.label, [Html.text("Settings")]),
            Html.keyed(
              "div",
              parts.root,
              Array.to_list(
                Array.map(
                  (part: T.item_parts(msg)) =>
                    (
                      part.item.key,
                      Html.div(part.tab, [Html.text(part.item.label)]),
                    ),
                  parts.items,
                ),
              ),
            ),
            Html.keyed(
              "div",
              [],
              Array.to_list(
                Array.map(
                  (part: T.item_parts(msg)) =>
                    (
                      part.item.key,
                      Html.div(
                        part.panel,
                        part.render_panel
                          ? [Html.text(part.item.label ++ " content")] : [],
                      ),
                    ),
                  parts.items,
                ),
              ),
            ),
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
              tabs: T.init(~id="t", ~items=entries, ()),
              items: entries,
              out: [],
              hooks: 0,
              mounted: true,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model => Sub.map(msg => Child(msg), T.subscriptions(model.tabs)),
      ~node="#app",
      ~flags=(),
      (),
    );
  let selection = () => T.selected(app.model().tabs);
  let focused = () => T.focused(app.model().tabs);
  let focus_key = k => {
    focus(node("#t-item-" ++ k));
    frame();
  };
  check("default selection", selection() == Some("a"));
  check(
    "tablist semantics",
    attr("#t", "role") == Some("tablist")
    && attr("#t", "aria-labelledby") == Some("t-label")
    && attr("#t", "aria-orientation") == Some("horizontal"),
  );
  List.iter(
    k => {
      check("tab semantics", attr("#t-item-" ++ k, "role") == Some("tab"));
      check(
        "controls resolves",
        attr("#t-item-" ++ k, "aria-controls") == Some("t-panel-item-" ++ k)
        && attr("#t-panel-item-" ++ k, "role") == Some("tabpanel"),
      );
      check(
        "panel labelled by tab",
        attr("#t-panel-item-" ++ k, "aria-labelledby")
        == Some("t-item-" ++ k),
      );
      check(
        "panel visibility",
        get(node("#t-panel-item-" ++ k), "hidden") == !String.equal(k, "a"),
      );
    },
    ["a", "b", "c", "d"],
  );
  check(
    "panel tab order default and opt-out",
    attr("#t-panel-item-a", "tabindex") == Some("0")
    && attr("#t-panel-item-c", "tabindex") == None,
  );
  check(
    "disabled tabs skipped",
    attr("#t-item-b", "aria-disabled") == Some("true")
    && attr("#t-item-b", "tabindex") == Some("-1"),
  );
  focus_key("a");
  List.iter(
    ((source, keyboard, target)) => {
      check(
        "navigation prevents",
        !key("#t-item-" ++ source, "keydown", keyboard),
      );
      frame();
      check("automatic activates", selection() == Some(target));
      check("navigation focuses", has_focus("#t-item-" ++ target));
      check(
        "selected data and panel",
        attr("#t-item-" ++ target, "data-selected") == Some("")
        && get(node("#t-panel-item-" ++ target), "hidden") == false,
      );
    },
    [
      ("a", "ArrowRight", "c"),
      ("c", "ArrowRight", "d"),
      ("d", "ArrowRight", "a"),
      ("a", "ArrowLeft", "d"),
      ("d", "ArrowLeft", "c"),
      ("c", "Home", "a"),
      ("a", "End", "d"),
    ],
  );
  let before = List.length(app.model().out);
  let prior_model = app.model().tabs;
  let prior_snapshot = Js.Json.stringifyAny(prior_model);
  ignore(key("#t-item-d", "keydown", "ArrowRight"));
  ignore(key("#t-item-d", "keydown", "ArrowRight"));
  check(
    "updates never mutate a prior model",
    Js.Json.stringifyAny(prior_model) == prior_snapshot,
  );
  check(
    "batched state",
    selection() == Some("c") && List.length(app.model().out) == before + 2,
  );
  frame();
  check(
    "batched focus does not reselect intermediate",
    has_focus("#t-item-c")
    && selection() == Some("c")
    && List.length(app.model().out) == before
    + 2,
  );
  List.iter(
    k => {
      check("ctrl untouched", key(~ctrl=true, "#t-item-c", "keydown", k));
      check("alt untouched", key(~alt=true, "#t-item-c", "keydown", k));
      check("meta untouched", key(~meta=true, "#t-item-c", "keydown", k));
      check(
        "IME untouched",
        key(~composing=true, "#t-item-c", "keydown", k),
      );
    },
    ["ArrowRight", "Home", "End", " ", "Enter"],
  );
  List.iter(
    k =>
      check("orthogonal and Tab untouched", key("#t-item-c", "keydown", k)),
    ["Tab", "ArrowUp", "ArrowDown"],
  );
  check(
    "Shift Tab untouched",
    key(~shift=true, "#t-item-c", "keydown", "Tab"),
  );
  check(
    "Delete composes without preventing",
    key("#t-item-c", "keydown", "Delete") && app.model().hooks == 1,
  );
  let before = List.length(app.model().out);
  app.dispatch(Set("a"));
  frame();
  check(
    "setter is quiet",
    selection() == Some("a") && List.length(app.model().out) == before,
  );
  app.dispatch(Config(T.Ltr, T.Horizontal, T.Manual));
  frame();
  ignore(key("#t-item-a", "keydown", "ArrowRight"));
  frame();
  check(
    "manual separate focus",
    selection() == Some("a")
    && focused() == Some("c")
    && has_focus("#t-item-c")
    && attr("#t-item-c", "tabindex") == Some("0")
    && attr("#t-item-a", "tabindex") == Some("-1"),
  );
  ignore(key("#t-item-c", "keydown", "Enter"));
  frame();
  check("manual Enter activates", selection() == Some("c"));
  ignore(key("#t-item-c", "keydown", "End"));
  frame();
  check(
    "manual End only focus",
    selection() == Some("c") && focused() == Some("d"),
  );
  ignore(key("#t-item-d", "keydown", " "));
  frame();
  check("manual Space activates", selection() == Some("d"));
  let before = List.length(app.model().out);
  ignore(key("#t-item-d", "keydown", " "));
  ignore(key("#t-item-d", "keydown", "Enter"));
  frame();
  check("activation idempotent", List.length(app.model().out) == before);
  ignore(key("#t-item-d", "keydown", "Home"));
  frame();
  focus(node("#outside"));
  frame();
  check(
    "manual exit restores selected entry",
    attr("#t-item-d", "tabindex") == Some("0")
    && attr("#t-item-a", "tabindex") == Some("-1"),
  );
  List.iter(
    activation => {
      app.dispatch(Config(T.Rtl, T.Horizontal, activation));
      app.dispatch(Set("a"));
      frame();
      focus_key("a");
      ignore(key("#t-item-a", "keydown", "ArrowLeft"));
      frame();
      check("RTL left advances", focused() == Some("c"));
      ignore(key("#t-item-c", "keydown", "ArrowRight"));
      frame();
      check("RTL right previous", focused() == Some("a"));
      app.dispatch(Config(T.Rtl, T.Vertical, activation));
      frame();
      List.iter(
        ((source, keyboard, target)) => {
          ignore(key("#t-item-" ++ source, "keydown", keyboard));
          frame();
          check(
            "vertical navigation",
            focused() == Some(target) && has_focus("#t-item-" ++ target),
          );
          check(
            "activation policy",
            selection() == Some(activation == T.Automatic ? target : "a"),
          );
        },
        [
          ("a", "ArrowDown", "c"),
          ("c", "ArrowDown", "d"),
          ("d", "ArrowDown", "a"),
          ("a", "ArrowUp", "d"),
          ("d", "Home", "a"),
          ("a", "End", "d"),
        ],
      );
      check(
        "vertical Left untouched",
        key("#t-item-d", "keydown", "ArrowLeft"),
      );
      check(
        "vertical Right untouched",
        key("#t-item-d", "keydown", "ArrowRight"),
      );
      check(
        "vertical aria",
        attr("#t", "aria-orientation") == Some("vertical"),
      );
    },
    [T.Automatic, T.Manual],
  );
  app.dispatch(Config(T.Ltr, T.Horizontal, T.Manual));
  frame();
  pointer("#t-item-c");
  click("#t-item-c");
  check(
    "pointer manual activates",
    selection() == Some("c") && has_focus("#t-item-c"),
  );
  let before = List.length(app.model().out);
  click("#t-item-b");
  ignore(key("#t-item-b", "keydown", "Enter"));
  ignore(key("#t-item-b", "keydown", "ArrowRight"));
  frame();
  check(
    "disabled click and keys inert",
    selection() == Some("c") && List.length(app.model().out) == before,
  );
  app.dispatch(Mounted(false));
  frame();
  check(
    "selected content policy",
    get(node("#t-panel-item-a"), "textContent") == ""
    && get(node("#t-panel-item-c"), "textContent") == "Privacy content"
    && attr("#t-item-a", "aria-controls") == Some("t-panel-item-a"),
  );
  app.dispatch(Items([|entries[0], entries[3]|]));
  frame();
  check(
    "removed selection and focus repaired",
    selection() == Some("a")
    && focused() == Some("a")
    && has_focus("#t-item-a")
    && List.length(app.model().out) == before,
  );
  app.dispatch(
    Items([|
      {
        ...entries[0],
        disabled: true,
      },
      entries[3],
    |]),
  );
  frame();
  check(
    "disabled selection repaired",
    selection() == Some("d") && has_focus("#t-item-d"),
  );
  app.dispatch(Items([|entries[3]|]));
  frame();
  ignore(key("#t-item-d", "keydown", "ArrowRight"));
  frame();
  check("single wraps", selection() == Some("d") && focused() == Some("d"));
  app.dispatch(
    Items([|
      {
        ...entries[3],
        disabled: true,
      },
    |]),
  );
  frame();
  check(
    "all disabled",
    selection() == None
    && focused() == None
    && attr("#t-item-d", "tabindex") == Some("-1"),
  );
  app.dispatch(Items([||]));
  frame();
  check("empty safe", selection() == None && focused() == None);
  check("empty focus parked on tablist", has_focus("#t"));
  app.dispatch(Items(entries));
  app.dispatch(Set("invalid"));
  frame();
  check("invalid setter falls back", selection() == Some("a"));
  focus(node("#outside"));
  frame();
  app.dispatch(Config(T.Ltr, T.Horizontal, T.Automatic));
  frame();
  focus_key("c");
  check("automatic external focus activates", selection() == Some("c"));
  app.dispatch(Set("a"));
  frame();
  focus_key("a");
  app.dispatch(Raw_items([|entries[2], entries[3]|]));
  ignore(key("#t-item-c", "keydown", "ArrowRight"));
  frame();
  check(
    "latest collection repair precedes intent",
    selection() == Some("d") && has_focus("#t-item-d"),
  );
  app.dispatch(Items([|entries[3], entries[2]|]));
  frame();
  check(
    "reorder preserves identity",
    selection() == Some("d") && has_focus("#t-item-d"),
  );
  ignore(key(~repeat=true, "#t-item-d", "keydown", "ArrowRight"));
  frame();
  check("repeat navigates reordered items", selection() == Some("c"));
  focus(node("#outside"));
  frame();
  let before = List.length(app.model().out);
  app.dispatch(Items([|entries[0]|]));
  frame();
  check(
    "outside repair quiet and no focus steal",
    selection() == Some("a")
    && has_focus("#outside")
    && List.length(app.model().out) == before,
  );
  let strange: array(T.item) = [|
    {
      key: {js|a b/é|js},
      label: "Odd",
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
  let tabs = query_all(document, "[role=tab]");
  check(
    "safe unique keys",
    attribute(tabs[0], "id") != attribute(tabs[1], "id"),
  );
  app.dispatch(Items(entries));
  app.dispatch(Set("a"));
  frame();
  focus_key("a");
  ignore(key("#t-item-a", "keydown", "ArrowRight"));
  focus(node("#outside"));
  frame();
  check("leaving before draw cancels queued focus", has_focus("#outside"));
  app.dispatch(Set("a"));
  frame();
  focus_key("a");
  ignore(key("#t-item-a", "keydown", "ArrowRight"));
  app.shutdown();
  focus(node("#outside"));
  frame();
  check("shutdown cancels queued focus", has_focus("#outside"));
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("tabs");
  Js.Promise.resolve();
};
