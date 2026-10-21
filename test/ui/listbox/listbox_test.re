open Tea;
open Test_support;
module L = Ui.Listbox;
[@mel.scope "JSON"] external snapshot: 'a => string = "stringify";
let all: array(L.item(int)) =
  Array.init(
    15,
    i => {
      let labels = [|
        "Apple",
        "Apricot",
        "Banana",
        "Black currant",
        "Blueberry",
        "Cherry",
        "Date",
        "Elderberry",
        "Fig",
        "Grape",
        "Kiwi",
        "Lemon",
        "Mango",
        "Orange",
        "Pear",
      |];
      {
        L.key: string_of_int(i),
        label: labels[i],
        disabled: i == 1,
        value: i,
      };
    },
  );
type msg =
  | Box(L.msg)
  | Items(array(L.item(int)))
  | Config(bool, bool, bool, bool)
  | Set(list(string))
  | Open(bool)
  | Close(bool)
  | Unmount;
type model = {
  mounted: bool,
  box: L.model,
  items: array(L.item(int)),
  changes: list(L.selection(int)),
  opened: int,
  closed: int,
};
let update = (message, model) =>
  switch (message) {
  | Unmount => (
      {
        ...model,
        mounted: false,
      },
      Cmd.none,
    )
  | Box(message) =>
    let (box, command, outs) =
      L.update(~items=model.items, message, model.box);
    let model =
      List.fold_left(
        (m, out) =>
          switch (out) {
          | L.Changed(change) => {
              ...m,
              changes: m.changes @ [change],
            }
          | L.Opened => {
              ...m,
              opened: m.opened + 1,
            }
          | L.Closed => {
              ...m,
              closed: m.closed + 1,
            }
          },
        {
          ...model,
          box,
        },
        outs,
      );
    (model, Cmd.map(message => Box(message), command));
  | Items(items) =>
    let (box, command) = L.reconcile(~items, model.box);
    (
      {
        ...model,
        box,
        items,
      },
      Cmd.map(message => Box(message), command),
    );
  | Config(disabled, readonly, invalid, wrap) =>
    let (box, command) =
      L.configure(~disabled, ~readonly, ~invalid, ~wrap, model.box);
    (
      {
        ...model,
        box,
      },
      Cmd.map(message => Box(message), command),
    );
  | Set(keys) => (
      {
        ...model,
        box: L.set_selected(keys, model.box),
      },
      Cmd.none,
    )
  | Open(last) =>
    let (box, command) = L.open_(~items=model.items, ~last, model.box);
    (
      {
        ...model,
        box,
      },
      Cmd.map(message => Box(message), command),
    );
  | Close(return_focus) =>
    let (box, command) = L.close(~return_focus, model.box);
    (
      {
        ...model,
        box,
      },
      Cmd.map(message => Box(message), command),
    );
  };
let fact_count = (name, facts) =>
  List.length(
    List.filter(
      fact =>
        switch (fact) {
        | Tea_vdom.Vdom.Property(key, _)
        | Tea_vdom.Vdom.Attribute(key, _)
        | Tea_vdom.Vdom.Event(key, _) => String.equal(name, key)
        | _ => false
        },
      facts,
    ),
  );
let view = model =>
  !model.mounted
    ? Html.div(
        [],
        [
          Html.span([Html.id("test-label")], [Html.text("Fruit")]),
          Html.button(
            [Html.id("test-button"), Html.type_("button")],
            [Html.text("Unmounted")],
          ),
        ],
      )
    : L.view(
        ~model=model.box,
        ~items=model.items,
        ~to_parent=m => Box(m),
        ~name="fruit",
        ~form_value=item => string_of_int(item.value),
        ~to_view=
          p => {
            check("one floating id", fact_count("id", p.listbox) == 1);
            check("one hidden fact", fact_count("hidden", p.listbox) == 1);
            check(
              "one button keydown",
              fact_count("keydown", p.button) == 1,
            );
            Html.div(
              [],
              [
                Html.span(p.label, [Html.text("Fruit")]),
                Html.button(p.button, [Html.text(p.display_value)]),
                Html.div(
                  p.listbox,
                  [
                    Html.div(
                      p.group("g"),
                      [Html.div(p.group_label("g"), [Html.text("Group")])]
                      @ Array.to_list(
                          Array.map(
                            (item: L.item(int)) =>
                              Html.div(
                                p.option_(item.key),
                                [Html.text(item.label)],
                              ),
                            model.items,
                          ),
                        ),
                    ),
                    Html.div(
                      p.empty @ [Html.id("empty")],
                      [Html.text("No options")],
                    ),
                  ],
                ),
                Html.button(
                  [Html.id("outside"), Html.type_("button")],
                  [Html.text("Outside")],
                ),
                Html.p(
                  [Html.id("text")],
                  [Html.text("Nonfocusable outside")],
                ),
              ]
              @ p.form_inputs,
            );
          },
        (),
      );
let rec settle = n =>
  n == 0
    ? Js.Promise.resolve()
    : {
      frame();
      Js.Promise.then_(_ => settle(n - 1), tick());
    };
let press = key_ => ignore(key("#test-listbox", "keydown", key_));
let active = (app: Tea_core.Runtime.handle(model, msg), expected) =>
  check(
    "active " ++ expected,
    L.active_key(app.model().box) == Some(expected),
  );
let hidden = expected =>
  check(
    "hidden property",
    get(node("#test-listbox"), "hidden") == expected,
  );
let mount = multiple => {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              mounted: true,
              box: L.init(~id="test", ~multiple, ~top_layer=false, ()),
              items: all,
              changes: [],
              opened: 0,
              closed: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        m =>
          m.mounted
            ? Sub.map(msg => Box(msg), L.subscriptions(m.box)) : Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  /* jsdom has no layout. Supply geometry, not a positioning/focus polyfill. */
  put(node("#test-button"), "getBoundingClientRect", _ =>
    {
      "x": 10.,
      "y": 10.,
      "width": 200.,
      "height": 30.,
      "top": 10.,
      "right": 210.,
      "bottom": 40.,
      "left": 10.,
    }
  );
  put(node("#test-listbox"), "getBoundingClientRect", _ =>
    {
      "x": 10.,
      "y": 44.,
      "width": 200.,
      "height": 200.,
      "top": 44.,
      "right": 210.,
      "bottom": 244.,
      "left": 10.,
    }
  );
  app;
};
let completed = {
  check(
    "single setter deduplicates and truncates",
    L.selected_keys(
      L.set_selected(["a", "a", "b"], L.init(~id="pure", ())),
    )
    == ["a"],
  );
  check(
    "multiple setter is a set",
    L.selected_keys(
      L.set_selected(
        ["a", "a", "b"],
        L.init(~id="pure", ~multiple=true, ()),
      ),
    )
    == ["a", "b"],
  );
  let app = mount(false);
  let initial = app.model().box;
  let initial_snapshot = snapshot(initial);
  let steps = [
    () => {
      hidden(true);
      check(
        "closed expanded",
        attr("#test-button", "aria-expanded") == Some("false"),
      );
      check(
        "compound accessible name",
        attr("#test-button", "aria-labelledby")
        == Some("test-label test-button"),
      );
      check(
        "controls resolves",
        attr("#test-button", "aria-controls") == Some("test-listbox"),
      );
      focus(node("#test-button"));
      check(
        "open key prevented",
        !key("#test-button", "keydown", "ArrowDown"),
      );
      check("intent opens immediately", L.is_open(app.model().box));
    },
    () => {
      check(
        "effects cannot mutate old models",
        String.equal(snapshot(initial), initial_snapshot),
      );
      hidden(false);
      active(app, "0");
      check("list receives focus", has_focus("#test-listbox"));
      check("one user Opened intent", app.model().opened == 1);
      check(
        "active descendant",
        attr("#test-listbox", "aria-activedescendant") == Some("test-item-0"),
      );
      check(
        "group labelled",
        attr("[role=group]", "aria-labelledby") == Some("test-group-g"),
      );
      check(
        "disabled option",
        attr("#test-item-1", "aria-disabled") == Some("true"),
      );
      check(
        "data presence",
        attr("#test-item-0", "data-active") == Some(""),
      );
      press("ArrowDown");
      press("ArrowDown");
      active(app, "3");
      check(
        "browsing never commits",
        L.selected_keys(app.model().box) == [],
      );
    },
    () => {
      press("Home");
      active(app, "0");
      press("ArrowUp");
      active(app, "0");
      press("End");
      active(app, "14");
      press("ArrowDown");
      active(app, "14");
      press("PageUp");
      active(app, "4");
      press("PageDown");
      active(app, "14");
      press("Home");
      press("ArrowDown");
      press("Enter");
    },
    () => {
      hidden(true);
      check("single value", L.selected_keys(app.model().box) == ["2"]);
      check(
        "changed out values",
        List.hd(app.model().changes).values == [2],
      );
      check("single focus return", has_focus("#test-button"));
      check(
        "form input serialized",
        get(node("input[name=fruit]"), "value") == "2",
      );
      check("closed out", app.model().closed == 1);
      ignore(key("#test-button", "keydown", "ArrowUp"));
    },
    () => {
      active(app, "2");
      press("End");
      press("Escape");
    },
    () => {
      check(
        "Escape retains selection",
        L.selected_keys(app.model().box) == ["2"],
      );
      check("Escape focus", has_focus("#test-button"));
      hidden(true);
      ignore(key("#test-button", "keydown", "b"));
    },
    () => {
      active(app, "3");
      press("b");
      active(app, "4");
      press("Home");
      press("b");
      press("l");
      active(app, "3");
      press(" ");
      active(app, "3");
      check("Space continues prefix", L.is_open(app.model().box));
      press("c");
      active(app, "5");
    },
    () => {
      List.iter(
        k => {
          check(
            "ctrl untouched",
            key(~ctrl=true, "#test-listbox", "keydown", k),
          );
          check(
            "alt untouched",
            key(~alt=true, "#test-listbox", "keydown", k),
          );
          check(
            "meta untouched",
            key(~meta=true, "#test-listbox", "keydown", k),
          );
          check(
            "IME untouched",
            key(~composing=true, "#test-listbox", "keydown", k),
          );
        },
        ["Enter", " ", "ArrowDown", "Home", "End", "a"],
      );
      check("Tab not prevented", key("#test-listbox", "keydown", "Tab"));
      focus(node("#outside"));
    },
    () => {
      hidden(true);
      check(
        "Tab did not select",
        L.selected_keys(app.model().box) == ["2"],
      );
      check("Tab no focus theft", has_focus("#outside"));
      click("#test-button");
    },
    () => {
      pointer("#outside");
      focus(node("#outside"));
    },
    () => {
      hidden(true);
      check("outside focus preserved", has_focus("#outside"));
      focus(node("#test-button"));
      click("#test-button");
    },
    () => {
      pointer("#text");
    },
    () => {
      hidden(true);
      check(
        "outside nonfocusable does not restore trigger",
        !has_focus("#test-button"),
      );
      focus(node("#test-button"));
      click("#test-button");
    },
    () => {
      check(
        "option pointerdown prevented",
        !
          dispatch(
            node("#test-item-5"),
            pointer_event(
              "pointerdown",
              {
                "bubbles": true,
                "cancelable": true,
              },
            ),
          ),
      );
      check("pointerdown retains list focus", has_focus("#test-listbox"));
      ignore(emit("#test-item-5", "pointermove"));
      active(app, "5");
      click("#test-item-1");
      check("disabled cannot select", L.is_open(app.model().box));
      click("#test-item-5");
    },
    () => {
      check("pointer selects", L.selected_keys(app.model().box) == ["5"]);
      check("pointer returns trigger focus", has_focus("#test-button"));
      app.dispatch(Config(false, true, true, false));
      click("#test-button");
    },
    () => {
      press("End");
      press("Enter");
      press("Home");
      press(" ");
      check("readonly open", L.is_open(app.model().box));
      check(
        "readonly retains selection",
        L.selected_keys(app.model().box) == ["5"],
      );
      check(
        "readonly aria",
        attr("#test-listbox", "aria-readonly") == Some("true"),
      );
      check(
        "invalid aria",
        attr("#test-listbox", "aria-invalid") == Some("true"),
      );
      app.dispatch(Config(true, false, false, false));
    },
    () => {
      hidden(true);
      check(
        "disabled trigger",
        get(node("#test-button"), "disabled") == true,
      );
      check(
        "disabled input excluded",
        get(node("input[name=fruit]"), "disabled") == true,
      );
      click("#test-button");
      check("disabled stays closed", !L.is_open(app.model().box));
      app.dispatch(Config(false, false, false, true));
      app.dispatch(Set([]));
      app.dispatch(Open(true));
    },
    () => {
      active(app, "14");
      press("ArrowDown");
      active(app, "0");
      press("ArrowUp");
      active(app, "14");
      app.dispatch(Items(Array.sub(all, 0, 8)));
    },
    () => {
      active(app, "7");
      app.dispatch(Items([||]));
    },
    () => {
      check("empty repaired", L.active_key(app.model().box) == None);
      check(
        "empty no active idref",
        attr("#test-listbox", "aria-activedescendant") == None,
      );
      check("empty state visible", get(node("#empty"), "hidden") == false);
      press("Enter");
      check("empty Enter no-op", L.is_open(app.model().box));
      app.dispatch(Items(all));
      app.dispatch(Set(["missing"]));
    },
    () => {
      active(app, "0");
      check(
        "missing selection retained",
        L.selected_keys(app.model().box) == ["missing"],
      );
      check(
        "missing no form value",
        Array.length(query_all(document, "input[name=fruit]")) == 0,
      );
      app.dispatch(Close(true));
    },
    () => {
      hidden(true);
      /* No draw between click intents: the final state, not stale view, wins. */
      List.iter(
        _ =>
          ignore(
            dispatch(
              node("#test-button"),
              event(
                "click",
                {
                  "bubbles": true,
                  "cancelable": true,
                },
              ),
            ),
          ),
        [0, 1, 2],
      );
      check("three rapid clicks open", L.is_open(app.model().box));
    },
    () => {
      check("rapid final focus", has_focus("#test-listbox"));
      app.dispatch(Close(true));
      app.dispatch(Open(false));
    },
    () => {
      check("Leaving reversed", L.is_open(app.model().box));
      hidden(false);
      check("reopened popup focus", has_focus("#test-listbox"));
      app.dispatch(
        Items(
          Array.map(
            item =>
              {
                ...item,
                L.disabled: true,
              },
            all,
          ),
        ),
      );
    },
    () => {
      check(
        "all disabled has no active option",
        L.active_key(app.model().box) == None,
      );
      List.iter(
        press,
        [
          "ArrowUp",
          "ArrowDown",
          "Home",
          "End",
          "PageUp",
          "PageDown",
          "a",
          "Enter",
        ],
      );
      check(
        "all disabled cannot commit",
        L.selected_keys(app.model().box) == ["missing"],
      );
      check("all disabled stays open", L.is_open(app.model().box));
      app.dispatch(
        Items([|
          {
            L.key: "a b",
            label: "Accent Éclair",
            disabled: false,
            value: 99,
          },
        |]),
      );
    },
    () => {
      active(app, "a b");
      check(
        "key-derived escaped descendant",
        attr("#test-listbox", "aria-activedescendant")
        == Some("test-item-a_0020b"),
      );
      press("ArrowDown");
      press("ArrowUp");
      active(app, "a b");
      app.dispatch(
        Items([|
          {
            L.key: "a b",
            label: "Accent Éclair",
            disabled: true,
            value: 99,
          },
          {
            L.key: "a_0020b",
            label: "Other",
            disabled: false,
            value: 100,
          },
        |]),
      );
    },
    () => {
      active(app, "a_0020b");
      check(
        "escaped keys do not collide",
        attr("#test-listbox", "aria-activedescendant")
        == Some("test-item-a_005f0020b"),
      );
      app.dispatch(Close(true));
      app.dispatch(Close(true));
    },
    () => {
      check(
        "idempotent close preserves queued focus",
        has_focus("#test-button"),
      );
      app.dispatch(Open(false));
    },
    () => {
      app.dispatch(Unmount);
    },
    () => {
      check(
        "unmount removes popup",
        Array.length(query_all(document, "#test-listbox")) == 0,
      );
      check("unmount restores connected trigger", has_focus("#test-button"));
      check("unmount root listeners", Tea_dom.Debug.active_listeners() == 0);
      check("unmount document listeners", listener_count() == 0);
      app.shutdown();
    },
    () => {
      check("shutdown root listeners", Tea_dom.Debug.active_listeners() == 0);
      check("shutdown document listeners", listener_count() == 0);
    },
  ];
  let single =
    List.fold_left(
      (promise, step) =>
        Js.Promise.then_(
          _ => {
            step();
            settle(8);
          },
          promise,
        ),
      settle(8),
      steps,
    );
  Js.Promise.then_(
    _ => {
      let app = mount(true);
      let replacement = ref(None);
      let steps = [
        () => {
          focus(node("#test-button"));
          click("#test-button");
        },
        () => {
          check(
            "multi semantics",
            attr("#test-listbox", "aria-multiselectable") == Some("true"),
          );
          press(" ");
          press("ArrowDown");
          press("Enter");
          check("multi toggle stays open", L.is_open(app.model().box));
          check(
            "multi selected set",
            L.selected_keys(app.model().box) == ["0", "2"],
          );
        },
        () => {
          check(
            "two inputs",
            Array.length(query_all(document, "input[name=fruit]")) == 2,
          );
          press("Enter");
          check(
            "toggle deselects",
            L.selected_keys(app.model().box) == ["0"],
          );
          press("Escape");
        },
        () => {
          check("multi Escape focus", has_focus("#test-button"));
          app.dispatch(Open(false));
        },
        () => {
          active(app, "0");
          check(
            "shift Tab never prevented",
            key(~shift=true, "#test-listbox", "keydown", "Tab"),
          );
          focus(node("#outside"));
        },
        () => {
          check("shift Tab preserves focus", has_focus("#outside"));
          app.dispatch(Open(false));
        },
        () => {
          let never: Js.Promise.t(unit) =
            Js.Promise.make((~resolve as _, ~reject as _) => ());
          put(node("#test-listbox"), "getAnimations", _ =>
            [|{ "finished": never }|]
          );
          press("Escape");
        },
        () => {
          check(
            "in-flight leaving wait retained",
            L.phase(app.model().box) == Ui.Animation.Leaving,
          );
          put(node("#test-listbox"), "getAnimations", _ => [||]);
          app.dispatch(Open(false));
        },
        () => {
          check(
            "in-flight leave reversal",
            L.phase(app.model().box) == Ui.Animation.Open,
          );
          check("reversal focuses popup", has_focus("#test-listbox"));
          let never: Js.Promise.t(unit) =
            Js.Promise.make((~resolve as _, ~reject as _) => ());
          put(node("#test-listbox"), "getAnimations", _ =>
            [|{ "finished": never }|]
          );
          press("Escape");
        },
        () => {
          check(
            "shutdown during unresolved wait",
            L.phase(app.model().box) == Ui.Animation.Leaving,
          );
          app.shutdown();
        },
        () => {
          check(
            "multi shutdown listeners",
            listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
          );
          /* An abandoned queued focus must not attach to a fresh runtime
             reusing the same id and numeric generation in this frame. */
          let old = mount(false);
          old.dispatch(Open(false));
          old.dispatch(Close(true));
          old.shutdown();
          let fresh = mount(false);
          fresh.dispatch(Open(false));
          fresh.dispatch(Close(false));
          focus(node("#outside"));
          replacement := Some(fresh);
        },
        () => {
          check(
            "shutdown focus cannot cross runtime identity",
            has_focus("#outside"),
          );
          Option.iter(app => app.Tea_core.Runtime.shutdown(), replacement^);
        },
        () => {
          check(
            "replacement cleanup",
            listener_count() == 0 && Tea_dom.Debug.active_listeners() == 0,
          );
          report("listbox");
        },
      ];
      List.fold_left(
        (promise, step) =>
          Js.Promise.then_(
            _ => {
              step();
              settle(8);
            },
            promise,
          ),
        settle(8),
        steps,
      );
    },
    single,
  );
};
