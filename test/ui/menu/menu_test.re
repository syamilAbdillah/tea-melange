open Tea;
open Test_support;
module M = Ui.Menu;
module A = Ui.Animation;
let i = (~disabled=false, key, label, kind): M.item => {
  key,
  label,
  disabled,
  kind,
};
let items = [|
  i(~disabled=true, "disabled", "Disabled", M.Action),
  i("alpha", {js|Álpha|js}, M.Action),
  i("apple", "Apple", M.Action),
  i("sep", "", M.Separator),
  i("check", "Check", M.Checkbox),
  i(
    "group",
    "Options",
    M.Group([|
      i("one", "One", M.Radio("group")),
      i("two", "Two", M.Radio("group")),
    |]),
  ),
  i(
    "share",
    "Share",
    M.Submenu([|
      i("link", "Link", M.Action),
      i("nested", "Nested", M.Submenu([|i("deep", "Deep", M.Action)|])),
    |]),
  ),
  i("share-menu", "Collision probe", M.Action),
  i("group-label", "Label collision probe", M.Action),
  i("a b_/?", "Unsafe key", M.Action),
|];
type msg =
  | Menu(M.msg)
  | Open
  | Close
  | Items(array(M.item))
  | Mount(bool);
type model = {
  menu: M.model,
  items: array(M.item),
  outs: list(M.out_msg),
  mounted: bool,
};
let update = (message, model) =>
  switch (message) {
  | Menu(message) =>
    let (menu, cmd, outs) =
      M.update(~items=model.items, message, model.menu);
    (
      {
        ...model,
        menu,
        outs: model.outs @ outs,
      },
      Cmd.map(msg => Menu(msg), cmd),
    );
  | Open =>
    let (menu, cmd) = M.open_(~items=model.items, model.menu);
    (
      {
        ...model,
        menu,
      },
      Cmd.map(msg => Menu(msg), cmd),
    );
  | Close =>
    let (menu, cmd) = M.close(model.menu);
    (
      {
        ...model,
        menu,
      },
      Cmd.map(msg => Menu(msg), cmd),
    );
  | Items(items) => (
      {
        ...model,
        items,
      },
      Cmd.none,
    )
  | Mount(mounted) => (
      {
        ...model,
        mounted,
      },
      Cmd.none,
    )
  };
let rec entry = (p: M.item_parts(msg)) =>
  Html.div(
    p.attrs,
    switch (p.item.kind) {
    | M.Group(_) => [
        Html.div(p.label, [Html.text(p.item.label)]),
        ...Array.to_list(Array.map(entry, p.children)),
      ]
    | _ => [Html.text(p.item.label)]
    },
  );
let named = (name, facts) =>
  List.length(
    List.filter(
      fact =>
        switch (fact) {
        | Tea_vdom.Vdom.Attribute(key, _)
        | Property(key, _) => String.equal(key, name)
        | _ => false
        },
      facts,
    ),
  );
let view = model =>
  Html.div(
    [],
    [
      model.mounted
        ? M.view(
            ~model=model.menu,
            ~items=model.items,
            ~to_parent=msg => Menu(msg),
            ~to_view=
              parts =>
                Html.div(
                  [],
                  [
                    Html.button(parts.trigger, [Html.text("Actions")]),
                    ...Array.to_list(
                         Array.map(
                           (p: M.panel_parts(msg)) => {
                             check(
                               "exactly one hidden and id fact",
                               named("hidden", p.attrs) == 1
                               && named("id", p.attrs) == 1,
                             );
                             Html.div(
                               p.attrs,
                               Array.to_list(Array.map(entry, p.items)),
                             );
                           },
                           parts.panels,
                         ),
                       ),
                  ],
                ),
            (),
          )
        : Html.div(
            [],
            [Html.button([Html.id("m-button")], [Html.text("Actions")])],
          ),
      Html.button([Html.id("outside")], [Html.text("Outside")]),
    ],
  );
let geometry = () =>
  Array.iter(
    el =>
      put(el, "getBoundingClientRect", () =>
        {
          "x": 100.,
          "y": 100.,
          "width": 180.,
          "height": 40.,
          "top": 100.,
          "bottom": 140.,
          "left": 100.,
          "right": 280.,
        }
      ),
    query_all(document, "[id]"),
  );
let rec settle = n =>
  n == 0
    ? Js.Promise.resolve()
    : {
      frame();
      Js.Promise.then_(_ => settle(n - 1), tick());
    };
let step = (promise, f) =>
  Js.Promise.then_(
    _ => {
      f();
      settle(10);
    },
    promise,
  );
let press = (~shift=false, selector, value) =>
  ignore(key(~shift, selector, "keydown", value));
[@mel.send]
external listen: (Dom.document, string, Dom.event => unit) => unit =
  "addEventListener";
[@mel.send]
external unlisten: (Dom.document, string, Dom.event => unit) => unit =
  "removeEventListener";
/* Observe an ancestor: cancelBubble resets after dispatch. */
let reaches_document = (~ctrl=false, ~composing=false, selector, value) => {
  let reached = ref(false);
  let spy = _ => reached := true;
  listen(document, "keydown", spy);
  ignore(key(~ctrl, ~composing, selector, "keydown", value));
  unlisten(document, "keydown", spy);
  reached^;
};
let completed = {
  let empty = M.init(~id="pure", ());
  let (empty_open, _) = M.open_(~items=[||], empty);
  check(
    "empty opens with no active",
    M.is_open(empty_open) && M.active_key(empty_open) == None,
  );
  let (all_disabled, _) =
    M.open_(~items=[|i(~disabled=true, "d", "Disabled", M.Action)|], empty);
  check(
    "all-disabled remains navigable",
    M.active_key(all_disabled) == Some("d"),
  );
  let domain =
    empty
    |> M.set_selected(Some("x"))
    |> M.set_checked(~key="c", true)
    |> M.set_radio(~group="r", ~key=Some("v"));
  check(
    "setters and accessors",
    M.selected(domain) == Some("x")
    && M.checked(~key="c", domain)
    && M.radio_value(~group="r", domain) == Some("v"),
  );
  check(
    "prior model immutable",
    !M.checked(~key="c", empty) && M.selected(empty) == None,
  );
  check(
    "clear setters",
    !M.checked(~key="c", M.set_checked(~key="c", false, domain))
    && M.radio_value(~group="r", M.set_radio(~group="r", ~key=None, domain))
    == None,
  );
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              menu: M.init(~id="m", ~top_layer=false, ~hover_delay=1., ()),
              items,
              outs: [],
              mounted: true,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=
        model =>
          model.mounted
            ? Sub.map(msg => Menu(msg), M.subscriptions(model.menu))
            : Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  let act = key =>
    check("active " ++ key, M.active_key(app.model().menu) == Some(key));
  let open_ = () => {
    focus(node("#m-button"));
    press("#m-button", "ArrowDown");
  };
  let phase_hidden = () => {
    for (_n in 0 to 7) {
      frame();
    };
    check(
      "DOM hidden exactly Closed",
      get(node("#m-menu"), "hidden")
      == (M.phase(app.model().menu) == A.Closed),
    );
  };
  geometry();
  check(
    "closed native trigger",
    attr("#m-button", "type") == Some("button")
    && attr("#m-button", "aria-haspopup") == Some("menu")
    && attr("#m-button", "aria-expanded") == Some("false"),
  );
  phase_hidden();
  check(
    "closed trigger Escape propagates and is not prevented",
    reaches_document("#m-button", "Escape")
    && key("#m-button", "keydown", "Escape"),
  );
  let run =
    step(
      Js.Promise.resolve(),
      () => {
        open_();
        act("alpha");
        phase_hidden();
      },
    );
  let run =
    step(
      run,
      () => {
        check("menu gets focus", has_focus("#m-menu"));
        check("Opened once", app.model().outs == [M.Opened]);
        check(
          "menu semantics",
          attr("#m-menu", "role") == Some("menu")
          && attr("#m-menu", "aria-labelledby") == Some("m-button"),
        );
        check(
          "label group semantics",
          attr("#m-item-group", "role") == Some("group")
          && attr("#m-label-item-group", "id") == Some("m-label-item-group"),
        );
        let ids =
          Array.map(el => get(el, "id"), query_all(document, "[id]"));
        Array.iteri(
          (idx, id) =>
            Array.iteri(
              (other, value) =>
                check(
                  "unique id namespace",
                  idx == other || !String.equal(id, value),
                ),
              ids,
            ),
          ids,
        );
        press("#m-menu", "ArrowDown");
        press("#m-menu", "ArrowDown");
        act("check");
        press("#m-menu", "Home");
        act("disabled");
        check(
          "disabled still navigable",
          attr("#m-item-disabled", "aria-disabled") == Some("true"),
        );
        let count = List.length(app.model().outs);
        press("#m-menu", "Enter");
        press("#m-menu", " ");
        check(
          "disabled cannot activate",
          List.length(app.model().outs) == count
          && M.is_open(app.model().menu),
        );
        press("#m-menu", "ArrowUp");
        act("a b_/?");
        press("#m-menu", "ArrowDown");
        act("disabled");
        press("#m-menu", "End");
        act("a b_/?");
        press("#m-menu", "Home");
        press("#m-menu", "a");
        act("alpha");
        press("#m-menu", "a");
        act("apple");
        List.iter(
          k => {
            check("ctrl passes", key(~ctrl=true, "#m-menu", "keydown", k));
            check("alt passes", key(~alt=true, "#m-menu", "keydown", k));
            check("meta passes", key(~meta=true, "#m-menu", "keydown", k));
            check(
              "composition passes",
              key(~composing=true, "#m-menu", "keydown", k),
            );
          },
          ["Home", "End", "Enter", " ", "ArrowDown", "Escape"],
        );
        act("apple");
        press("#m-menu", "Enter");
        check(
          "action emits then closes",
          List.rev(app.model().outs) |> List.hd == M.Closed(M.Activation),
        );
        check(
          "selection saved",
          M.selected(app.model().menu) == Some("apple"),
        );
        check("immediate trigger return", has_focus("#m-button"));
        phase_hidden();
      },
    );
  let run =
    step(
      run,
      () => {
        phase_hidden();
        check("anchor listeners released", listener_count() == 0);
        open_();
      },
    );
  let run =
    step(
      run,
      () => {
        press("#m-menu", "c");
        act("check");
        press("#m-menu", " ");
        check(
          "Space checkbox keeps open",
          M.checked(~key="check", app.model().menu)
          && M.is_open(app.model().menu),
        );
        press("#m-menu", "ArrowDown");
        act("one");
        press("#m-menu", " ");
        check(
          "Space radio keeps open",
          M.radio_value(~group="group", app.model().menu) == Some("one")
          && M.is_open(app.model().menu),
        );
        press("#m-menu", "ArrowDown");
        press("#m-menu", "Enter");
        check(
          "Enter radio closes",
          M.radio_value(~group="group", app.model().menu) == Some("two")
          && !M.is_open(app.model().menu),
        );
      },
    );
  let run = step(run, () => {open_()});
  let run =
    step(
      run,
      () => {
        press("#m-menu", "s");
        act("share");
        press("#m-menu", "ArrowRight");
        check(
          "submenu first",
          M.active_key(~submenu="share", app.model().menu) == Some("link"),
        );
      },
    );
  let run =
    step(
      run,
      () => {
        check(
          "submenu focus and expanded",
          has_focus("#m-submenu-item-share")
          && attr("#m-item-share", "aria-expanded") == Some("true"),
        );
        press("#m-submenu-item-share", "ArrowDown");
        press("#m-submenu-item-share", " ");
      },
    );
  let run =
    step(
      run,
      () => {
        check("deep submenu focus", has_focus("#m-submenu-item-nested"));
        check(
          "modified submenu Escape propagates",
          reaches_document(~ctrl=true, "#m-submenu-item-nested", "Escape"),
        );
        check(
          "unknown submenu key propagates",
          reaches_document("#m-submenu-item-nested", "F2"),
        );
        check(
          "claimed submenu Escape stops at menu",
          !reaches_document("#m-submenu-item-nested", "Escape"),
        );
        check(
          "Escape closes only branch",
          M.is_open(app.model().menu) && has_focus("#m-submenu-item-share"),
        );
        check(
          "claimed back arrow still propagates",
          reaches_document("#m-submenu-item-share", "ArrowLeft"),
        );
        check(
          "back returns parent active",
          has_focus("#m-menu")
          && M.active_key(app.model().menu) == Some("share"),
        );
        press("#m-menu", "ArrowRight");
      },
    );
  let run =
    step(
      run,
      () => {
        press("#m-submenu-item-share", "Enter");
        check(
          "nested activation closes tree",
          !M.is_open(app.model().menu)
          && M.selected(app.model().menu) == Some("link"),
        );
      },
    );
  let run = step(run, () => {open_()});
  let run =
    step(
      run,
      () => {
        ignore(emit("#m-item-check", "pointermove"));
        act("check");
        ignore(emit("#m-menu", "pointerleave"));
        check(
          "pointerleave clears active",
          M.active_key(app.model().menu) == None,
        );
        ignore(emit("#m-item-share", "pointermove"));
      },
    );
  let run =
    step(
      run,
      () => {
        check(
          "delayed hover opens",
          attr("#m-item-share", "aria-expanded") == Some("true"),
        );
        ignore(emit("#m-menu", "pointerleave"));
        act("share");
        pointer("#m-submenu-item-share");
        check("submenu inside not outside", M.is_open(app.model().menu));
        focus(node("#outside"));
        pointer("#outside");
        check(
          "outside leaves focus",
          has_focus("#outside") && !M.is_open(app.model().menu),
        );
      },
    );
  let run =
    step(
      run,
      () => {
        check(
          "outside cleanup",
          listener_count() == 0 && has_focus("#outside"),
        );
        open_();
        press("#m-button", "ArrowUp");
        act("a b_/?");
      },
    );
  let run =
    step(
      run,
      () => {
        check("Tab not prevented", key("#m-menu", "keydown", "Tab"));
        check(
          "Tab resets native origin synchronously",
          has_focus("#m-button") && !M.is_open(app.model().menu),
        );
        open_();
        check(
          "Shift Tab not prevented",
          key(~shift=true, "#m-menu", "keydown", "Tab"),
        );
        app.dispatch(Open);
        app.dispatch(Close);
        app.dispatch(Open);
        check("same-frame reverse ends open", M.is_open(app.model().menu));
      },
    );
  let run =
    step(
      run,
      () => {
        check("reopened focus", has_focus("#m-menu"));
        check("arrows still propagate", reaches_document("#m-menu", "Home"));
        check("unhandled key propagates", reaches_document("#m-menu", "F2"));
        check(
          "modified Escape propagates",
          reaches_document(~ctrl=true, "#m-menu", "Escape"),
        );
        check(
          "composing Escape propagates",
          reaches_document(~composing=true, "#m-menu", "Escape"),
        );
        check(
          "root Escape is claimed",
          !reaches_document("#m-menu", "Escape"),
        );
        check(
          "root Escape closes and focuses trigger",
          !M.is_open(app.model().menu) && has_focus("#m-button"),
        );
        frame();
        check(
          "closed trigger Escape propagates",
          reaches_document("#m-button", "Escape"),
        );
        app.dispatch(Open);
        frame();
        check(
          "open trigger Escape is claimed",
          !reaches_document("#m-button", "Escape"),
        );
        app.dispatch(Open);
        frame();
        app.dispatch(
          Items([|
            i("start", "Start", M.Action),
            i("accent", {js|Éxport|js}, M.Action),
          |]),
        );
        press("#m-menu", "Home");
        check("ASCII search is claimed", !key("#m-menu", "keydown", "e"));
        act("accent");
        app.dispatch(
          Items([|
            i("start", "Start", M.Action),
            i("export", "Export", M.Action),
            i("astral", {js|😀 Smile|js}, M.Action),
            i("cafe", {js|Café|js}, M.Action),
          |]),
        );
        press("#m-menu", "Home");
        check(
          "accented key is claimed",
          !key("#m-menu", "keydown", {js|é|js}),
        );
        act("export");
        press("#m-menu", "Home");
        check(
          "astral key is claimed",
          !key("#m-menu", "keydown", {js|😀|js}),
        );
        act("astral");
        press("#m-menu", "Home");
        press("#m-menu", "c");
        act("cafe");
        press("#m-menu", "a");
        press("#m-menu", "f");
        press("#m-menu", {js|é|js});
        act("cafe");
        app.dispatch(Items([|i("replacement", "Replacement", M.Action)|]));
        press("#m-menu", "ArrowDown");
        act("replacement");
      },
    );
  let run =
    step(
      run,
      () => {
        check(
          "repair idref exists",
          attr("#m-menu", "aria-activedescendant")
          == attr("#m-item-replacement", "id"),
        );
        app.dispatch(Items([||]));
        press("#m-menu", "Home");
        check("empty repairs", M.active_key(app.model().menu) == None);
        app.dispatch(Close);
        app.dispatch(Open);
        app.dispatch(Close);
      },
    );
  let run =
    step(
      run,
      () => {
        check(
          "rapid final close cannot focus leaving menu",
          has_focus("#m-button")
          && M.phase(app.model().menu) == A.Closed
          && listener_count() == 0,
        );
        app.dispatch(Items(items));
        app.dispatch(Open);
      },
    );
  let run =
    step(
      run,
      () => {
        app.dispatch(Mount(false));
        frame();
        check("unmount restores trigger", has_focus("#m-button"));
        check("unmount releases sources", listener_count() == 0);
        app.dispatch(Mount(true));
        frame();
        geometry();
        app.dispatch(Close);
        app.dispatch(Open);
      },
    );
  let run =
    step(
      run,
      () => {
        check("remount and reversal focus", has_focus("#m-menu"));
        app.dispatch(Close);
      },
    );
  let run =
    step(
      run,
      () => {
        put(node("#m-menu"), "getAnimations", () =>
          [|
            {
              "finished":
                Js.Promise.make((~resolve as _, ~reject as _) => ()),
            },
          |]
        );
        app.dispatch(Open);
      },
    );
  let run =
    step(
      run,
      () => {
        check(
          "pending animation still entering",
          M.phase(app.model().menu) == A.Entering,
        );
        ignore(emit("#m-item-share", "pointermove"));
        app.shutdown();
        frame();
        check("shutdown restores focus", has_focus("#m-button"));
        check(
          "zero root/global listeners",
          Tea_dom.Debug.active_listeners() == 0 && listener_count() == 0,
        );
      },
    );
  Js.Promise.then_(
    _ => {
      report("menu");
      Js.Promise.resolve();
    },
    run,
  );
};
