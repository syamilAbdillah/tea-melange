open Tea;
open Test_support;
module C = Ui.Combobox;
module V = Tea_vdom.Vdom;
let items: array(C.item) = [|
  {
    key: "a /",
    label: {js|Áland|js},
    disabled: false,
  },
  {
    key: "b",
    label: "Blocked",
    disabled: true,
  },
  {
    key: "c",
    label: "Canada",
    disabled: false,
  },
  {
    key: "d",
    label: "Denmark",
    disabled: false,
  },
|];
type msg =
  | Progress(C.msg)
  | Replace(array(C.item))
  | Text(string)
  | Select(list(string))
  | Open
  | Close
  | Loading(bool)
  | Disable(bool)
  | Readonly(bool)
  | Unmount
  | Hook;
type model = {
  combo: C.model,
  items: array(C.item),
  outs: list(C.out_msg),
  loading: bool,
  mounted: bool,
  hooks: int,
};
let settle = () =>
  for (_i in 1 to 12) {
    flush();
  };
let start =
    (
      ~multiple=false,
      ~mode=C.Restricted(C.Revert),
      ~autocomplete=C.List,
      ~wrap=true,
      ~open_on_type=true,
      ~rendered_keys=?,
      ~localized=false,
      (),
    ) =>
  App.element(
    ~init=
      () =>
        (
          {
            combo:
              C.init(
                ~id="test",
                ~multiple,
                ~mode,
                ~autocomplete,
                ~wrap,
                ~open_on_type,
                ~top_layer=false,
                (),
              ),
            items,
            outs: [],
            loading: false,
            mounted: true,
            hooks: 0,
          },
          Cmd.none,
        ),
    ~update=
      (msg, model) => {
        let (combo, command, outs) =
          switch (msg) {
          | Progress(msg) =>
            C.update(
              ~items=model.items,
              ~label_for_key=
                key =>
                  Option.map(
                    (i: C.item) => i.label,
                    Array.find_opt((i: C.item) => i.key == key, items),
                  ),
              msg,
              model.combo,
            )
          | Text(text) => (C.set_text(text, model.combo), Cmd.none, [])
          | Select(keys) => (
              C.set_selected(keys, model.combo),
              Cmd.none,
              [],
            )
          | Open =>
            let (m, c) = C.open_(model.combo);
            (m, c, []);
          | Close =>
            let (m, c) = C.close(model.combo);
            (m, c, []);
          | Disable(value) =>
            let (m, c) = C.set_disabled(value, model.combo);
            (m, c, []);
          | Readonly(value) => (
              C.set_readonly(value, model.combo),
              Cmd.none,
              [],
            )
          | _ => (model.combo, Cmd.none, [])
          };
        let model = {
          ...model,
          combo,
          outs: model.outs @ outs,
        };
        let model =
          switch (msg) {
          | Replace(items) => {
              ...model,
              items,
            }
          | Loading(loading) => {
              ...model,
              loading,
            }
          | Unmount => {
              ...model,
              mounted: false,
            }
          | Hook => {
              ...model,
              hooks: model.hooks + 1,
            }
          | _ => model
          };
        (
          {
            ...model,
            combo:
              C.refresh(
                ~items=model.items,
                ~loading=model.loading,
                ~results_label=
                  n =>
                    localized
                      ? string_of_int(n) ++ " matches"
                      : string_of_int(n) ++ " results available",
                ~empty_label=localized ? "Nothing found" : "No results",
                ~loading_label=localized ? "Searching" : "Loading",
                model.combo,
              ),
          },
          Cmd.map(m => Progress(m), command),
        );
      },
    ~subscriptions=
      model =>
        model.mounted
          ? Sub.map(m => Progress(m), C.subscriptions(model.combo))
          : Sub.none,
    ~view=
      model =>
        Html.div(
          [],
          [
            model.mounted
              ? C.view(
                  ~model=model.combo,
                  ~items=model.items,
                  ~rendered_keys?,
                  ~to_parent=m => Progress(m),
                  ~has_description=true,
                  ~has_error=true,
                  ~invalid=true,
                  ~on_key_down=key => key == "F2" ? Some(Hook) : None,
                  ~to_view=
                    p => {
                      check(
                        "one hidden fact",
                        List.length(
                          List.filter(
                            f =>
                              switch (f) {
                              | V.Property("hidden", _) => true
                              | _ => false
                              },
                            p.popup,
                          ),
                        )
                        == 1,
                      );
                      check(
                        "one id fact",
                        List.length(
                          List.filter(
                            f =>
                              switch (f) {
                              | V.Attribute("id", _) => true
                              | _ => false
                              },
                            p.popup,
                          ),
                        )
                        == 1,
                      );
                      Html.div(
                        [],
                        [
                          Html.label(p.label, [Html.text("Country")]),
                          Html.div(
                            p.wrapper,
                            [
                              Html.input(p.control, []),
                              Html.button(p.toggle, [Html.text("Show")]),
                            ],
                          ),
                          Html.p(
                            p.description,
                            [Html.text("Choose a country")],
                          ),
                          Html.p(p.error, [Html.text("Required")]),
                          Html.div(
                            p.popup,
                            Array.to_list(
                              Array.map(
                                (i: C.item) =>
                                  Html.div(
                                    p.option_(i.key),
                                    [Html.text(i.label)],
                                  ),
                                Array.of_list(
                                  List.filter(
                                    (i: C.item) =>
                                      Option.fold(
                                        ~none=true,
                                        ~some=
                                          keys =>
                                            List.exists(
                                              String.equal(i.key),
                                              keys,
                                            ),
                                        rendered_keys,
                                      ),
                                    Array.to_list(model.items),
                                  ),
                                ),
                              ),
                            ),
                          ),
                          Html.div(
                            p.loading @ [Html.id("loading")],
                            [Html.text("Loading")],
                          ),
                          Html.div(
                            p.empty @ [Html.id("empty")],
                            [Html.text("No results")],
                          ),
                          Html.div(p.live, [Html.text(p.announcement)]),
                          Html.div(
                            [],
                            List.map(
                              key =>
                                Html.span(
                                  p.chip(key),
                                  [
                                    Html.text(key),
                                    Html.button(
                                      p.remove(~key, ~label=key),
                                      [Html.text("Remove")],
                                    ),
                                  ],
                                ),
                              p.selected,
                            ),
                          ),
                        ],
                      );
                    },
                  (),
                )
              : Html.text(""),
            Html.button([Html.id("outside")], [Html.text("Outside")]),
          ],
        ),
    ~node="#app",
    ~flags=(),
    (),
  );
let stop = app => {
  app.Tea_core.Runtime.shutdown();
  settle();
  check("root listeners disposed", Tea_dom.Debug.active_listeners() == 0);
  check("global listeners disposed", listener_count() == 0);
};
let send = (app, msg) => {
  app.Tea_core.Runtime.dispatch(msg);
  settle();
};
let press = key_ => {
  let allowed = key("#test-control", "keydown", key_);
  settle();
  allowed;
};
let input = value => {
  put(node("#test-control"), "value", value);
  ignore(emit("#test-control", "input"));
};
let escape_reaches_document = () => {
  let (accepted, reached) = key_at_document("#test-control", "Escape");
  check("Escape prevention matches its claim", accepted == reached);
  settle();
  reached;
};
let active = app =>
  C.active_key(~items=app.Tea_core.Runtime.model().items, app.model().combo);
let completed = {
  check("filter empty", C.filter(~query="", ~label=x => x, [||]) == [||]);
  check(
    "filter accents case and decomposed",
    C.filter(
      ~query="al",
      ~label=x => x,
      ~mode=C.Starts_with,
      [|{js|Áland|js}, {js|álpha|js}, "CANADA"|],
    )
    == [|{js|Áland|js}, {js|álpha|js}|],
  );
  check(
    "filter folds letters without canonical decomposition",
    C.filter(
      ~query="ss",
      ~label=x => x,
      [|{js|Straße|js}, {js|Øresund|js}, "Strasse"|],
    )
    == [|{js|Straße|js}, "Strasse"|]
    && C.filter(
         ~query="o",
         ~label=x => x,
         ~mode=C.Starts_with,
         [|{js|Øresund|js}|],
       )
    == [|{js|Øresund|js}|],
  );
  check(
    "contains default",
    C.filter(~query="AN", ~label=x => x, [|"Canada", "Japan", "UK"|])
    == [|"Canada", "Japan"|],
  );
  check(
    "empty matches all",
    C.filter(~query="", ~label=x => x, [|"a", "b"|]) == [|"a", "b"|],
  );
  let app = start();
  check(
    "closed semantics",
    attr("#test-control", "role") == Some("combobox")
    && attr("#test-control", "aria-expanded") == Some("false")
    && attr("#test-control", "aria-controls") == None
    && attr("#test-control", "aria-autocomplete") == Some("list")
    && get(node("#test-popup"), "hidden"),
  );
  check(
    "input convention",
    attr("#test-control", "aria-describedby")
    == Some("test-description test-error")
    && attr("#test-control", "aria-errormessage") == Some("test-error")
    && attr("#test-label", "for") == Some("test-control"),
  );
  check(
    "toggle semantics",
    attr("#test-toggle", "tabindex") == Some("-1")
    && attr("#test-toggle", "aria-label") == Some("Show suggestions"),
  );
  focus(node("#test-control"));
  check("unclaimed empty Escape bubbles", escape_reaches_document());
  check("Down claimed", !press("ArrowDown"));
  check("first enabled", active(app) == Some("a /"));
  let first_id = Option.get(attr("#test-control", "aria-activedescendant"));
  check(
    "safe resolving option id",
    attr("#" ++ first_id, "role") == Some("option"),
  );
  check(
    "rendered expanded hidden",
    !get(node("#test-popup"), "hidden")
    && attr("#test-control", "aria-controls") == Some("test-popup"),
  );
  check(
    "disabled option",
    attr("#test-item-b", "aria-disabled") == Some("true"),
  );
  check("focus stays input", has_focus("#test-control"));
  /* Intent, not next-index: three events without any intervening draw. */
  ignore(key("#test-control", "keydown", "ArrowDown"));
  ignore(key("#test-control", "keydown", "ArrowDown"));
  settle();
  check("same-frame navigation skips disabled", active(app) == Some("d"));
  ignore(press("ArrowDown"));
  check("wrap", active(app) == Some("a /"));
  ignore(press("ArrowUp"));
  check("reverse wrap", active(app) == Some("d"));
  List.iter(
    k => check("native caret unprevented", press(k)),
    ["Home", "End", "ArrowLeft", "ArrowRight"],
  );
  check("caret no active movement", active(app) == Some("d"));
  check(
    "modified and composing Escape reach document",
    key_at_document(~ctrl=true, "#test-control", "Escape") == (true, true)
    && key_at_document(~alt=true, "#test-control", "Escape") == (true, true)
    && key_at_document(~meta=true, "#test-control", "Escape") == (true, true)
    && key_at_document(~composing=true, "#test-control", "Escape")
    == (true, true),
  );
  check("claimed open Escape stops at document", !escape_reaches_document());
  send(app, Text("abc"));
  check(
    "closed clearing Escape stops at document",
    !escape_reaches_document(),
  );
  check(
    "clear emits query",
    List.mem(C.Query_changed(""), app.model().outs),
  );
  check("empty Escape now bubbles", escape_reaches_document());
  ignore(key(~alt=true, "#test-control", "keydown", "ArrowDown"));
  settle();
  check(
    "alt open no active",
    C.is_open(app.model().combo) && active(app) == None,
  );
  ignore(key(~alt=true, "#test-control", "keydown", "ArrowUp"));
  settle();
  check("alt closes", !C.is_open(app.model().combo));
  ignore(press("ArrowUp"));
  check("closed up last", active(app) == Some("d"));
  check("select prevents submit", !press("Enter"));
  check(
    "selected value",
    C.selected(app.model().combo) == ["d"]
    && C.text(app.model().combo) == "Denmark",
  );
  input("unmatched");
  ignore(emit("#test-control", "blur"));
  check("restricted revert", C.text(app.model().combo) == "Denmark");
  send(app, Replace([||]));
  input("different");
  pointer("#outside");
  settle();
  check(
    "revert from full label cache",
    C.text(app.model().combo) == "Denmark",
  );
  send(app, Replace(items));
  send(app, Open);
  settle();
  check(
    "announcement",
    get(node("#test-live"), "textContent") == "4 results available",
  );
  send(app, Loading(true));
  check(
    "loading",
    !get(node("#loading"), "hidden")
    && get(node("#test-live"), "textContent") == "Loading",
  );
  send(app, Loading(false));
  send(app, Replace([||]));
  check(
    "empty",
    !get(node("#empty"), "hidden")
    && get(node("#test-live"), "textContent") == "No results",
  );
  send(app, Replace(items));
  ignore(press("ArrowDown"));
  send(app, Replace([|items[2]|]));
  check("removed active repaired", active(app) == None);
  send(app, Replace(items));
  send(app, Readonly(true));
  send(app, Close);
  send(app, Text("readonly"));
  check("readonly closed Escape bubbles", escape_reaches_document());
  input("cannot edit");
  check("readonly input rejected", C.text(app.model().combo) == "readonly");
  send(app, Readonly(false));
  send(app, Disable(true));
  check("disabled native", get(node("#test-control"), "disabled"));
  ignore(press("ArrowDown"));
  check("disabled cannot open", !C.is_open(app.model().combo));
  send(app, Disable(false));
  send(app, Text(""));
  check("hook parent type", press("F2") && app.model().hooks == 1);
  List.iter(
    k => {
      check(
        "ctrl unclaimed",
        key(~ctrl=true, "#test-control", "keydown", k),
      );
      check(
        "IME unclaimed",
        key(~composing=true, "#test-control", "keydown", k),
      );
    },
    ["ArrowDown", "Enter", "Escape", "Backspace"],
  );
  settle();
  check("IME key no selection", !C.is_open(app.model().combo));
  let before = List.length(app.model().outs);
  ignore(emit("#test-control", "compositionstart"));
  input("ca");
  check("no composition query", List.length(app.model().outs) == before);
  ignore(emit("#test-control", "compositionend"));
  check(
    "one committed IME query",
    List.mem(C.Query_changed("ca"), app.model().outs),
  );
  ignore(press("ArrowDown"));
  pointer("#test-item-c");
  check(
    "option pointerdown keeps default focus",
    has_focus("#test-control"),
  );
  ignore(emit("#test-item-c", "pointermove"));
  check("pointer active", active(app) == Some("c"));
  click("#test-item-c");
  check(
    "pointer selects",
    C.selected(app.model().combo) == ["c"] && has_focus("#test-control"),
  );
  click("#test-toggle");
  check("toggle opens", C.is_open(app.model().combo));
  click("#test-toggle");
  check("toggle closes", !C.is_open(app.model().combo));
  click("#test-toggle");
  focus(node("#outside"));
  pointer("#outside");
  settle();
  check("outside never moves focus", has_focus("#outside"));
  send(app, Open);
  send(app, Unmount);
  stop(app);
  let app = start(~wrap=false, ());
  ignore(press("ArrowUp"));
  ignore(press("ArrowDown"));
  check("no wrap boundary", active(app) == Some("d"));
  send(
    app,
    Replace([|
      {
        key: "x",
        label: "X",
        disabled: true,
      },
    |]),
  );
  ignore(press("ArrowDown"));
  check("all disabled", active(app) == None);
  send(app, Replace([||]));
  ignore(press("ArrowUp"));
  check("empty nav", active(app) == None);
  stop(app);
  let app = start(~mode=C.Restricted(C.Clear), ());
  send(app, Text("invalid"));
  send(app, Select(["c"]));
  ignore(emit("#test-control", "blur"));
  check(
    "restricted clear",
    C.text(app.model().combo) == "" && C.selected(app.model().combo) == [],
  );
  stop(app);
  let app = start(~mode=C.Free_form, ~multiple=true, ());
  focus(node("#test-control"));
  ignore(press("ArrowDown"));
  ignore(press("Enter"));
  check(
    "multiple retains popup and clears query",
    C.is_open(app.model().combo)
    && C.text(app.model().combo) == ""
    && C.selected(app.model().combo) == ["a /"],
  );
  ignore(press("ArrowDown"));
  ignore(press("Enter"));
  check(
    "chips insertion order",
    C.selected(app.model().combo) == ["a /", "c"],
  );
  check(
    "Backspace removes last",
    !press("Backspace") && C.selected(app.model().combo) == ["a /"],
  );
  click("button[aria-label='Remove a /']");
  check(
    "chip button removal",
    C.selected(app.model().combo) == [] && has_focus("#test-control"),
  );
  send(app, Select(["c", "c", "d"]));
  check("setter unique", C.selected(app.model().combo) == ["c", "d"]);
  input("custom tag");
  ignore(press("Enter"));
  check(
    "free form submitted",
    List.mem(C.Submitted("custom tag"), app.model().outs),
  );
  check("closed Enter allows form", press("Enter"));
  input("another");
  check("Tab unprevented", press("Tab"));
  check(
    "Tab closes no select",
    !C.is_open(app.model().combo)
    && C.selected(app.model().combo) == ["c", "d"],
  );
  stop(app);
  let app = start(~autocomplete=C.Both, ());
  focus(node("#test-control"));
  input("Can");
  settle();
  check(
    "inline label and range",
    C.text(app.model().combo) == "Canada"
    && C.query(app.model().combo) == "Can"
    && get(node("#test-control"), "selectionStart") == 3
    && get(node("#test-control"), "selectionEnd") == 6,
  );
  check(
    "Backspace removes suffix only",
    !press("Backspace") && C.text(app.model().combo) == "Can",
  );
  input("Cana");
  settle();
  ignore(press("Enter"));
  check(
    "inline commit emits full query",
    C.selected(app.model().combo) == ["c"]
    && List.mem(C.Query_changed("Canada"), app.model().outs),
  );
  stop(app);
  let app =
    start(~autocomplete=C.None_, ~open_on_type=false, ~rendered_keys=[], ());
  input("Can");
  check(
    "none and explicit opening",
    attr("#test-control", "aria-autocomplete") == Some("none")
    && !C.is_open(app.model().combo),
  );
  ignore(press("ArrowDown"));
  check(
    "virtual unrendered no idref",
    attr("#test-control", "aria-activedescendant") == None,
  );
  stop(app);
  check(
    "initial single selection canonical",
    C.selected(C.init(~id="initial", ~selected=["c", "d"], ())) == ["c"],
  );
  check(
    "initial multiple selection canonical",
    C.selected(
      C.init(~id="initial", ~multiple=true, ~selected=["c", "c", "d"], ()),
    )
    == ["c", "d"],
  );
  check(
    "filter unmatched",
    C.filter(~query="zzz", ~label=(i: C.item) => i.label, items) == [||],
  );
  check(
    "filter keeps disabled suggestions",
    C.filter(~query="blo", ~label=(i: C.item) => i.label, items)
    == [|items[1]|],
  );
  let app = start(~localized=true, ());
  send(app, Open);
  check(
    "custom result label",
    get(node("#test-live"), "textContent") == "4 matches",
  );
  send(app, Loading(true));
  check(
    "custom loading label",
    get(node("#test-live"), "textContent") == "Searching",
  );
  send(app, Replace([||]));
  send(app, Loading(false));
  check(
    "custom empty label",
    get(node("#test-live"), "textContent") == "Nothing found",
  );
  stop(app);
  let app = start();
  send(app, Select(["c", "d", "c"]));
  check(
    "single setter keeps first",
    C.selected(app.model().combo) == ["c"],
  );
  send(app, Open);
  ignore(emit("#test-item-b", "pointermove"));
  click("#test-item-b");
  check(
    "disabled option never active or selected",
    active(app) == None && C.selected(app.model().combo) == ["c"],
  );
  let scrolled = ref(0);
  put(node("#test-item-c"), "scrollIntoView", _ => incr(scrolled));
  ignore(press("ArrowDown"));
  ignore(press("ArrowDown"));
  check("active scroll after render", scrolled^ == 1);
  app.dispatch(Replace([||]));
  app.dispatch(Replace([|items[3]|]));
  settle();
  check(
    "announcement debounces final results",
    get(node("#test-live"), "textContent") == "1 results available",
  );
  stop(app);
  let app = start();
  let unfinished = Js.Promise.make((~resolve as _, ~reject as _) => ());
  put(node("#test-popup"), "getAnimations", _ =>
    [|{ "finished": unfinished }|]
  );
  send(app, Open);
  check(
    "animation wait remains entering",
    C.phase(app.model().combo) == Ui.Animation.Entering,
  );
  send(app, Close);
  check(
    "leave retains hidden=false and positioning",
    C.phase(app.model().combo) == Ui.Animation.Leaving
    && !get(node("#test-popup"), "hidden")
    && listener_count() > 0,
  );
  send(app, Open);
  check(
    "reversing leave retains resources",
    C.phase(app.model().combo) == Ui.Animation.Entering,
  );
  stop(app);
  let app = start();
  app.dispatch(Open);
  app.shutdown();
  settle();
  check(
    "shutdown before opening draw",
    listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
  );
  /* Close and reverse before a render; the latest intent wins and owns resources. */
  let app = start();
  app.dispatch(Open);
  app.dispatch(Close);
  app.dispatch(Open);
  settle();
  check(
    "rapid reversal open",
    C.is_open(app.model().combo) && !get(node("#test-popup"), "hidden"),
  );
  app.dispatch(Close);
  settle();
  after_tick(() => {
    settle();
    check(
      "final closed hidden",
      C.phase(app.model().combo) == Ui.Animation.Closed
      && get(node("#test-popup"), "hidden"),
    );
    check("closed cleanup before shutdown", listener_count() == 0);
    stop(app);
    report("combobox");
  });
};
