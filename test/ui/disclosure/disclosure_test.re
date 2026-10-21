open Tea;
open Test_support;
module D = Ui.Disclosure;
type model = {
  open_: bool,
  keep: bool,
  active: option(string),
  count: int,
};
type msg =
  | Toggle
  | Keep(bool)
  | Accordion(string);
let update = (msg, model) => (
  switch (msg) {
  | Toggle => {
      ...model,
      open_: !model.open_,
      count: model.count + 1,
    }
  | Keep(keep) => {
      ...model,
      keep,
    }
  | Accordion(key) => {
      ...model,
      active:
        switch (model.active) {
        | Some(active) when String.equal(active, key) => None
        | _ => Some(key)
        },
    }
  },
  Cmd.none,
);
let disclosure = (~id, ~open_, ~on_toggle, ~keep_mounted=true, label) =>
  D.view(
    ~id,
    ~open_,
    ~on_toggle,
    ~keep_mounted,
    ~to_view=
      p =>
        Html.div(
          [],
          [
            Html.h2([], [Html.button(p.button, [Html.text(label)])]),
            p.mounted
              ? Html.div(
                  p.panel,
                  [
                    Html.a(
                      [Html.href("#content"), Html.id(id ++ "-link")],
                      [Html.text("Content")],
                    ),
                  ],
                )
              : Html.text(""),
          ],
        ),
    (),
  );
let view = model =>
  Html.div(
    [],
    [
      disclosure(
        ~id="d",
        ~open_=model.open_,
        ~on_toggle=Toggle,
        ~keep_mounted=model.keep,
        "Details",
      ),
      disclosure(
        ~id="a",
        ~open_=model.active == Some("a"),
        ~on_toggle=Accordion("a"),
        "First",
      ),
      disclosure(
        ~id="b",
        ~open_=model.active == Some("b"),
        ~on_toggle=Accordion("b"),
        "Second",
      ),
    ],
  );
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              open_: false,
              keep: true,
              active: None,
              count: 0,
            },
            Cmd.none,
          ),
      ~update,
      ~view,
      ~subscriptions=_ => Sub.none,
      ~node="#app",
      ~flags=(),
      (),
    );
  check(
    "native button",
    get(node("#d-button"), "tagName") == "BUTTON"
    && attr("#d-button", "type") == Some("button"),
  );
  check(
    "collapsed",
    attr("#d-button", "aria-expanded") == Some("false")
    && attr("#d-button", "aria-controls") == Some("d-panel")
    && get(node("#d-panel"), "hidden") == true,
  );
  List.iter(
    selector =>
      check(
        "closed data",
        attr(selector, "data-closed") == Some("")
        && attr(selector, "data-open") == None,
      ),
    ["#d-button", "#d-panel"],
  );
  focus(node("#d-button"));
  pointer("#d-button");
  click("#d-button");
  check(
    "open and focus retained",
    app.model().open_
    && has_focus("#d-button")
    && attr("#d-button", "aria-expanded") == Some("true")
    && get(node("#d-panel"), "hidden") == false,
  );
  List.iter(
    selector =>
      check(
        "open data",
        attr(selector, "data-open") == Some("")
        && attr(selector, "data-closed") == None,
      ),
    ["#d-button", "#d-panel"],
  );
  List.iter(
    key_ =>
      check(
        "native key defaults retained",
        key("#d-button", "keydown", key_) && key("#d-button", "keyup", key_),
      ),
    ["Enter", " ", "Tab", "ArrowDown", "Home", "End"],
  );
  check("no synthetic keyboard click in jsdom", app.model().count == 1);
  List.iter(
    key_ => {
      check("ctrl unchanged", key(~ctrl=true, "#d-button", "keydown", key_));
      check("alt unchanged", key(~alt=true, "#d-button", "keydown", key_));
      check("meta unchanged", key(~meta=true, "#d-button", "keydown", key_));
      check(
        "IME unchanged",
        key(~composing=true, "#d-button", "keydown", key_),
      );
    },
    ["Enter", " ", "Tab"],
  );
  let event_ = () =>
    event(
      "click",
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  ignore(dispatch(node("#d-button"), event_()));
  ignore(dispatch(node("#d-button"), event_()));
  frame();
  check(
    "rapid toggles use latest parent",
    app.model().open_
    && app.model().count == 3
    && attr("#d-button", "aria-expanded") == Some("true"),
  );
  app.dispatch(Keep(false));
  frame();
  check(
    "open always mounted",
    Array.length(query_all(document, "#d-panel")) == 1,
  );
  click("#d-button");
  check(
    "unmounted no dangling controls",
    Array.length(query_all(document, "#d-panel")) == 0
    && attr("#d-button", "aria-controls") == None
    && attr("#d-button", "aria-expanded") == Some("false"),
  );
  check("button survives unmount", has_focus("#d-button"));
  click("#d-button");
  check(
    "remount restores controls",
    attr("#d-button", "aria-controls") == Some("d-panel")
    && get(node("#d-panel"), "hidden") == false,
  );
  click("#d-button");
  app.dispatch(Keep(true));
  frame();
  check("closed remounted hidden", get(node("#d-panel"), "hidden") == true);
  click("#a-button");
  click("#b-button");
  check(
    "accordion exclusive",
    attr("#a-button", "aria-expanded") == Some("false")
    && attr("#b-button", "aria-expanded") == Some("true")
    && get(node("#a-panel"), "hidden") == true,
  );
  click("#b-button");
  check("accordion permits all collapsed", app.model().active == None);
  check(
    "heading composition",
    Array.length(query_all(document, "h2 > button")) == 3,
  );
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("disclosure");
  Js.Promise.resolve();
};
