open Tea;
open Test_support;
module D = Ui.Drag_and_drop;
type msg =
  | Drag(D.msg)
  | Replace(array(D.container))
  | Cancel;
type model = {
  drag: D.model,
  containers: array(D.container),
  outs: list(D.out_msg),
};
let item = (~disabled=false, key): D.item => {
  key,
  label: key,
  disabled,
};
let container = (~disabled=false, key, items): D.container => {
  key,
  label: key,
  disabled,
  items,
};
let initial = [|
  container(
    "left",
    [|item("a"), item("b"), item(~disabled=true, "fixed"), item("c")|],
  ),
  container(~disabled=true, "locked", [||]),
  container("right", [|item("d")|]),
  container("empty", [||]),
|];
let update = (message, model) =>
  switch (message) {
  | Drag(message) =>
    let (drag, command, outs) =
      D.update(~containers=model.containers, message, model.drag);
    (
      {
        ...model,
        drag,
        outs: model.outs @ outs,
      },
      Cmd.map(message => Drag(message), command),
    );
  | Replace(containers) => (
      {
        ...model,
        containers,
      },
      Cmd.none,
    )
  | Cancel =>
    let (drag, command) = D.cancel(model.drag);
    (
      {
        ...model,
        drag,
      },
      Cmd.map(message => Drag(message), command),
    );
  };
let view = model =>
  D.view(
    ~model=model.drag,
    ~containers=model.containers,
    ~to_parent=message => Drag(message),
    ~to_view=
      p =>
        Html.div(
          [],
          [
            Html.p(p.instructions, [Html.text(p.instructions_text)]),
            Html.div(p.live, [Html.text(p.live_text)]),
            Html.div(p.overlay @ [Html.id("overlay")], []),
            Html.div(
              [],
              Array.to_list(
                Array.map(
                  (c: D.container) =>
                    Html.ul(
                      p.container(c.key)
                      @ [Html.data("test-container", c.key)],
                      Array.to_list(
                        Array.map(
                          (item: D.item) =>
                            Html.li(
                              p.item(item.key)
                              @ [Html.data("test-item", item.key)],
                              [
                                Html.button(
                                  p.handle(item.key)
                                  @ [Html.data("test-handle", item.key)],
                                  [Html.text(item.label)],
                                ),
                              ],
                            ),
                          c.items,
                        ),
                      ),
                    ),
                  model.containers,
                ),
              ),
            ),
          ],
        ),
    (),
  );
let mount =
    (
      ~orientation=D.Vertical,
      ~dir=D.Ltr,
      ~touch_delay=0,
      ~labels=D.default_labels,
      ~assertive=false,
      ~drag_from_item=true,
      ~containers=initial,
      (),
    ) =>
  App.element(
    ~init=
      () =>
        (
          {
            drag:
              D.init(
                ~id="sort",
                ~orientation,
                ~dir,
                ~touch_delay,
                ~labels,
                ~assertive,
                ~drag_from_item,
                (),
              ),
            containers,
            outs: [],
          },
          Cmd.none,
        ),
    ~update,
    ~view,
    ~subscriptions=
      model =>
        Sub.map(message => Drag(message), D.subscriptions(model.drag)),
    ~node="#app",
    ~flags=(),
    (),
  );
let handle = key => "[data-test-handle='" ++ key ++ "']";
let item_selector = key => "[data-test-item='" ++ key ++ "']";
let list_selector = key => "[data-test-container='" ++ key ++ "']";
let press = (~shift=false, ~repeat=false, app, key_) => {
  ignore(key(~shift, ~repeat, handle("a"), "keydown", key_));
  ignore(app);
};
let target = (app: Tea_core.Runtime.handle(model, msg), container, index) =>
  check(
    "preview " ++ container ++ string_of_int(index),
    D.preview(app.model().drag)
    == Some({
         D.container,
         index,
       }),
  );
let move_count = (app: Tea_core.Runtime.handle(model, msg)) =>
  List.length(
    List.filter(
      out =>
        switch (out) {
        | D.Moved(_) => true
        | _ => false
        },
      app.model().outs,
    ),
  );
let close = (app: Tea_core.Runtime.handle(model, msg)) => {
  app.shutdown();
  frame();
  check("no managed listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no document/window listeners", listener_count() == 0);
};
let point =
    (~pointer=1, ~touch=false, ~button=0, ~primary=true, selector, name, x, y) => {
  let e =
    pointer_event(
      name,
      {
        "bubbles": true,
        "cancelable": true,
        "clientX": x,
        "clientY": y,
        "pointerId": pointer,
        "isPrimary": primary,
        "pointerType": touch ? "touch" : "mouse",
        "button": button,
      },
    );
  dispatch(node(selector), e);
};
let rect = (selector, left, top, width, height) =>
  put(node(selector), "getBoundingClientRect", () =>
    {
      "left": left,
      "top": top,
      "right": left +. width,
      "bottom": top +. height,
      "x": left,
      "y": top,
      "width": width,
      "height": height,
    }
  );
let layout = () => {
  rect(list_selector("left"), 0., 0., 100., 200.);
  rect(list_selector("locked"), 110., 0., 100., 200.);
  rect(list_selector("right"), 220., 0., 100., 200.);
  rect(list_selector("empty"), 330., 0., 100., 200.);
  List.iteri(
    (i, key) =>
      rect(item_selector(key), 0., float_of_int(i * 40), 100., 40.),
    ["a", "b", "fixed", "c"],
  );
  rect(item_selector("d"), 220., 0., 100., 40.);
};
let completed = {
  let app = mount();
  check(
    "instructions idref resolves",
    attr(handle("a"), "aria-describedby") == Some("sort-instructions")
    && attr("#sort-live", "aria-live") == Some("polite"),
  );
  check(
    "no deprecated ARIA",
    attr(handle("a"), "aria-grabbed") == None
    && attr(list_selector("left"), "aria-dropeffect") == None,
  );
  check(
    "native button semantics",
    attr(handle("a"), "type") == Some("button")
    && attr(handle("fixed"), "data-disabled") == Some(""),
  );
  check(
    "orientation",
    attr(list_selector("left"), "data-orientation") == Some("vertical"),
  );
  check(
    "idle Escape reaches document",
    key_at_document(handle("a"), "Escape") == (true, true),
  );
  check("idle arrows untouched", key(handle("a"), "keydown", "ArrowDown"));
  List.iter(
    k => {
      check("ctrl untouched", key(~ctrl=true, handle("a"), "keydown", k));
      check("alt untouched", key(~alt=true, handle("a"), "keydown", k));
      check("meta untouched", key(~meta=true, handle("a"), "keydown", k));
      check(
        "IME untouched",
        key(~composing=true, handle("a"), "keydown", k),
      );
    },
    [" ", "Enter", "ArrowDown"],
  );
  check("no shortcut lift", D.dragging(app.model().drag) == None);
  focus(node(handle("a")));
  check("Space claimed", !key(handle("a"), "keydown", " "));
  check("lifted synchronously", D.dragging(app.model().drag) == Some("a"));
  /* No draw between these intents: each must use the new submodel. */
  press(app, "ArrowDown");
  press(app, "ArrowDown");
  target(app, "left", 2);
  check("no Moved in preview", move_count(app) == 0);
  frame();
  check(
    "dragging presence",
    attr(item_selector("a"), "data-dragging") == Some(""),
  );
  check(
    "target presence",
    attr(list_selector("left"), "data-over") == Some(""),
  );
  check(
    "announcement",
    D.announcement(app.model().drag) == "a moved to position 3 of 4 in left.",
  );
  press(~repeat=true, app, "Enter");
  check("repeat does not drop", D.dragging(app.model().drag) == Some("a"));
  press(~shift=true, app, "ArrowRight");
  target(app, "right", 1);
  press(~shift=true, app, "ArrowRight");
  target(app, "empty", 0);
  press(~shift=true, app, "ArrowRight");
  target(app, "empty", 0);
  press(~shift=true, app, "ArrowLeft");
  target(app, "right", 0);
  press(~shift=true, app, "ArrowLeft");
  target(app, "left", 0);
  press(app, "ArrowUp");
  target(app, "left", 0);
  press(app, "ArrowDown");
  press(app, "ArrowDown");
  press(app, "ArrowDown");
  press(app, "ArrowDown");
  target(app, "left", 3);
  press(app, "Enter");
  frame();
  check(
    "drop exactly once",
    move_count(app) == 1 && D.dragging(app.model().drag) == None,
  );
  check("drop retains focus", has_focus(handle("a")));
  check("drop announcement", D.announcement(app.model().drag) == "Dropped.");
  check(
    "data cleared",
    attr(item_selector("a"), "data-dragging") == None
    && attr(list_selector("left"), "data-over") == None,
  );
  check(
    "movement source and target",
    List.exists(
      out =>
        switch (out) {
        | D.Moved(m) =>
          m.key == "a"
          && m.from_container == "left"
          && m.from_index == 0
          && m.to_index == 3
        | _ => false
        },
      app.model().outs,
    ),
  );
  press(app, "Enter");
  frame();
  press(app, "ArrowDown");
  check(
    "modified and composing drag Escape reach document",
    key_at_document(~ctrl=true, handle("a"), "Escape") == (true, true)
    && key_at_document(~alt=true, handle("a"), "Escape") == (true, true)
    && key_at_document(~meta=true, handle("a"), "Escape") == (true, true)
    && key_at_document(~composing=true, handle("a"), "Escape")
    == (true, true),
  );
  check(
    "drag Escape prevented and stopped at document",
    key_at_document(handle("a"), "Escape") == (false, false),
  );
  frame();
  check(
    "Escape restores",
    D.dragging(app.model().drag) == None
    && D.announcement(app.model().drag)
    == "Cancelled, a returned to position 1."
    && has_focus(handle("a")),
  );
  press(app, " ");
  frame();
  check("Tab not prevented", key(handle("a"), "keydown", "Tab"));
  frame();
  check("Tab cancels", D.dragging(app.model().drag) == None);
  press(app, "Enter");
  frame();
  app.dispatch(Cancel);
  frame();
  check(
    "programmatic cancel has no output",
    List.length(app.model().outs) == 7,
  );
  check("disabled ignored", key(handle("fixed"), "keydown", " "));
  close(app);
  let horizontal = mount(~orientation=D.Horizontal, ());
  press(horizontal, "Enter");
  press(horizontal, "ArrowRight");
  target(horizontal, "left", 1);
  press(horizontal, "ArrowLeft");
  target(horizontal, "left", 0);
  press(~shift=true, horizontal, "ArrowDown");
  target(horizontal, "right", 0);
  press(~shift=true, horizontal, "ArrowUp");
  target(horizontal, "left", 0);
  close(horizontal);
  let rtl = mount(~orientation=D.Horizontal, ~dir=D.Rtl, ());
  press(rtl, "Enter");
  press(rtl, "ArrowLeft");
  target(rtl, "left", 1);
  press(rtl, "ArrowRight");
  target(rtl, "left", 0);
  close(rtl);
  let cross = mount(~dir=D.Rtl, ());
  press(cross, "Enter");
  press(~shift=true, cross, "ArrowLeft");
  target(cross, "right", 0);
  close(cross);
  let dynamic = mount();
  press(dynamic, "Enter");
  press(dynamic, "ArrowDown");
  press(dynamic, "ArrowDown");
  dynamic.dispatch(Replace([|container("left", [|item("a")|])|]));
  press(dynamic, "ArrowDown");
  target(dynamic, "left", 0);
  dynamic.dispatch(Replace([||]));
  press(dynamic, "Enter");
  frame();
  check(
    "missing dragged key cancels",
    D.dragging(dynamic.model().drag) == None,
  );
  close(dynamic);
  let disabled =
    mount(
      ~containers=[|container("left", [|item(~disabled=true, "a")|])|],
      (),
    );
  press(disabled, "Enter");
  check(
    "all disabled cannot lift",
    D.dragging(disabled.model().drag) == None,
  );
  close(disabled);
  let changing = mount();
  press(changing, "Enter");
  changing.dispatch(
    Replace([|container(~disabled=true, "left", [|item("a")|])|]),
  );
  press(changing, "ArrowDown");
  check(
    "disabled source cancels",
    D.dragging(changing.model().drag) == None,
  );
  close(changing);
  let empty = mount(~containers=[||], ());
  check("empty initial preview", D.preview(empty.model().drag) == None);
  close(empty);
  let single = mount(~containers=[|container("one", [|item("a")|])|], ());
  press(single, "Enter");
  press(single, "ArrowDown");
  press(single, "ArrowUp");
  target(single, "one", 0);
  press(single, "Enter");
  frame();
  check("single item no-op drops once", move_count(single) == 1);
  close(single);
  let removed_target = mount();
  press(removed_target, "Enter");
  press(~shift=true, removed_target, "ArrowRight");
  removed_target.dispatch(Replace([|initial[0]|]));
  press(removed_target, "ArrowDown");
  target(removed_target, "left", 1);
  close(removed_target);
  let special =
    mount(
      ~containers=[|
        container(
          "a_b",
          [|item("a b"), item("a_0020b"), item(""), item({js|日本語|js})|],
        ),
      |],
      (),
    );
  check(
    "key encoding is collision-free",
    attr(handle("a b"), "id") != attr(handle("a_0020b"), "id"),
  );
  check(
    "Unicode and empty key ids exist",
    attr(handle(""), "id") != None
    && attr(handle({js|日本語|js}), "id") != None,
  );
  check(
    "special keys share resolving instructions",
    attr(handle({js|日本語|js}), "aria-describedby")
    == Some("sort-instructions"),
  );
  close(special);
  let pointer_app = mount();
  layout();
  ignore(point(~primary=false, handle("a"), "pointerdown", 10., 10.));
  check(
    "nonprimary ignored",
    listener_count_for("document", "pointermove", true) == 0,
  );
  ignore(point(~button=2, handle("a"), "pointerdown", 10., 10.));
  check(
    "secondary button ignored",
    listener_count_for("document", "pointermove", true) == 0,
  );
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  check(
    "pointer lifecycle installed",
    listener_count_for("document", "pointermove", true) == 1,
  );
  ignore(point(handle("a"), "pointermove", 12., 12.));
  check(
    "below threshold no lift",
    D.dragging(pointer_app.model().drag) == None,
  );
  ignore(point(handle("a"), "pointerup", 12., 12.));
  frame();
  check("click gesture cleanup", listener_count() == 0);
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(~pointer=2, handle("a"), "pointermove", 80., 150.));
  check(
    "other pointer ignored",
    D.dragging(pointer_app.model().drag) == None,
  );
  ignore(point(handle("a"), "pointermove", 10., 150.));
  frame();
  target(pointer_app, "left", 3);
  check(
    "geometry variables",
    attr("#overlay", "style")
    == Some("--tea-drag-x: 0px; --tea-drag-y: 140px;"),
  );
  ignore(point(handle("a"), "pointermove", 225., 160.));
  frame();
  target(pointer_app, "right", 1);
  ignore(point(handle("a"), "pointerup", 225., 160.));
  frame();
  check(
    "pointer drop emits once",
    move_count(pointer_app) == 1 && has_focus(handle("a")),
  );
  check("pointer listeners removed", listener_count() == 0);
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(handle("a"), "pointermove", 500., 500.));
  frame();
  check(
    "outside has no target",
    D.preview(pointer_app.model().drag) == None,
  );
  ignore(point(handle("a"), "pointerup", 500., 500.));
  frame();
  check("outside cancels without move", move_count(pointer_app) == 1);
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(handle("a"), "pointermove", 225., 160.));
  /* Drop before the first measurement/draw must still use the release point. */
  ignore(point(handle("a"), "pointerup", 340., 100.));
  ignore(point(handle("a"), "pointermove", 225., 160.));
  ignore(point(handle("a"), "pointerup", 225., 160.));
  frame();
  check(
    "rapid release target",
    List.exists(
      out =>
        switch (out) {
        | D.Moved(m) => m.to_container == "empty" && m.to_index == 0
        | _ => false
        },
      pointer_app.model().outs,
    ),
  );
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(handle("a"), "pointermove", 10., 100.));
  frame();
  target(pointer_app, "left", 1);
  ignore(point(~pointer=2, handle("a"), "pointercancel", 10., 100.));
  check(
    "foreign pointercancel ignored",
    D.dragging(pointer_app.model().drag) == Some("a"),
  );
  ignore(
    point(~pointer=2, ~primary=false, handle("d"), "pointerdown", 225., 20.),
  );
  check(
    "secondary press cannot clear active click guard",
    listener_count_for("document", "click", true) == 1,
  );
  let click_event =
    event(
      "click",
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  put(click_event, "detail", 1);
  check(
    "drag-generated click synchronously suppressed",
    !dispatch(node(handle("a")), click_event),
  );
  rect(item_selector("fixed"), 0., 40., 100., 40.);
  ignore(
    dispatch(node(handle("a")), event("scroll", { "bubbles": false })),
  );
  frame();
  target(pointer_app, "left", 2);
  ignore(point(handle("a"), "pointercancel", 10., 100.));
  frame();
  check("cancel cleans all globals", listener_count() == 0);
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(handle("a"), "pointermove", 10., 100.));
  press(pointer_app, "Escape");
  check(
    "cancel discards preview but protects unfinished gesture",
    D.dragging(pointer_app.model().drag) == None
    && listener_count_for("document", "click", true) == 1,
  );
  let descendant_clicks = ref(0);
  put(node(handle("a")), "onclick", _ => incr(descendant_clicks));
  ignore(point(handle("a"), "pointerup", 10., 100.));
  let cancelled_click =
    event(
      "click",
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  put(cancelled_click, "detail", 1);
  check(
    "cancelled pointer click suppressed",
    !dispatch(node(handle("a")), cancelled_click) && descendant_clicks^ == 0,
  );
  check("cancel guard consumed synchronously", listener_count() == 0);
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(handle("a"), "pointermove", 10., 100.));
  ignore(point(handle("a"), "pointerup", 10., 100.));
  let ancestor_click =
    event(
      "click",
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  put(ancestor_click, "detail", 1);
  check(
    "retargeted ancestor click suppressed",
    !dispatch(node(list_selector("left")), ancestor_click),
  );
  frame();
  check("drop guard removed", listener_count() == 0);
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(handle("a"), "pointerup", 10., 10.));
  let short_click =
    event(
      "click",
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  put(short_click, "detail", 1);
  check(
    "short click clears old guard without redraw",
    dispatch(node(item_selector("a")), short_click),
  );
  ignore(point(handle("a"), "pointerdown", 10., 10.));
  ignore(point(handle("a"), "pointermove", 10., 100.));
  close(pointer_app);
  let only_handle = mount(~drag_from_item=false, ());
  layout();
  ignore(point(item_selector("a"), "pointerdown", 10., 10.));
  ignore(point(item_selector("a"), "pointermove", 10., 100.));
  check("handle-only policy", D.dragging(only_handle.model().drag) == None);
  close(only_handle);
  let custom =
    mount(
      ~assertive=true,
      ~labels={
        instructions: "Localized instructions",
        picked_up: (_, _, _) => "Lift",
        moved: (_, _, _, _) => "Move",
        dropped: "Drop",
        cancelled: (_, _) => "Cancel",
      },
      (),
    );
  press(custom, "Enter");
  frame();
  check(
    "custom labels",
    D.announcement(custom.model().drag) == "Lift"
    && attr("#sort-live", "aria-live") == Some("assertive"),
  );
  press(custom, "ArrowDown");
  check("custom move", D.announcement(custom.model().drag) == "Move");
  press(custom, "Escape");
  check("custom cancel", D.announcement(custom.model().drag) == "Cancel");
  close(custom);
  let horizontal_pointer = mount(~orientation=D.Horizontal, ~dir=D.Rtl, ());
  layout();
  rect(list_selector("left"), 0., 0., 200., 50.);
  List.iteri(
    (i, key) =>
      rect(item_selector(key), float_of_int(150 - i * 40), 0., 40., 50.),
    ["a", "b", "fixed", "c"],
  );
  ignore(point(handle("a"), "pointerdown", 170., 25.));
  ignore(point(handle("a"), "pointermove", 10., 25.));
  frame();
  target(horizontal_pointer, "left", 3);
  close(horizontal_pointer);
  let touch = mount(~touch_delay=5, ());
  layout();
  ignore(point(~touch=true, handle("a"), "pointerdown", 10., 10.));
  ignore(point(~touch=true, handle("a"), "pointermove", 10., 100.));
  frame();
  check(
    "touch movement before delay cancels",
    D.dragging(touch.model().drag) == None && listener_count() == 0,
  );
  ignore(point(~touch=true, handle("a"), "pointerdown", 10., 10.));
  Js.Promise.then_(
    _ => {
      ignore(point(~touch=true, handle("a"), "pointermove", 10., 100.));
      frame();
      check(
        "touch after long press lifts",
        D.dragging(touch.model().drag) == Some("a"),
      );
      check(
        "touch context menu suppressed after delay",
        !
          dispatch(
            node(handle("a")),
            event(
              "contextmenu",
              {
                "bubbles": true,
                "cancelable": true,
              },
            ),
          ),
      );
      check(
        "unrelated context menu untouched",
        dispatch(
          node(handle("d")),
          event(
            "contextmenu",
            {
              "bubbles": true,
              "cancelable": true,
            },
          ),
        ),
      );
      let unrelated_touch =
        event(
          "touchmove",
          {
            "bubbles": true,
            "cancelable": true,
          },
        );
      put(
        unrelated_touch,
        "touches",
        [|{ "target": node(handle("d")) }|],
      );
      put(
        unrelated_touch,
        "changedTouches",
        [|{ "target": node(handle("d")) }|],
      );
      check(
        "unrelated touch untouched",
        dispatch(node(handle("d")), unrelated_touch),
      );
      check(
        "ready touch prevents pan",
        {
          let touch_event =
            event(
              "touchmove",
              {
                "bubbles": true,
                "cancelable": true,
              },
            );
          put(
            touch_event,
            "touches",
            [|{ "target": node(handle("a")) }|],
          );
          put(
            touch_event,
            "changedTouches",
            [|{ "target": node(handle("a")) }|],
          );
          !dispatch(node(handle("a")), touch_event);
        },
      );
      ignore(point(~touch=true, handle("a"), "pointerup", 10., 100.));
      frame();
      check("touch commits", move_count(touch) == 1);
      close(touch);
      report("drag_and_drop");
      Js.Promise.resolve();
    },
    Js.Promise.make((~resolve, ~reject as _) =>
      ignore(timeout(() => resolve(. true), 15))
    ),
  );
};
