open Tea;
open Test_support;
module B = Ui.Button;
type msg =
  | Press
  | Toggle
  | Disabled(bool)
  | Hook;
type model = {
  count: int,
  pressed: bool,
  disabled: bool,
  hooks: int,
};
let update = (msg, model) => (
  switch (msg) {
  | Press => {
      ...model,
      count: model.count + 1,
    }
  | Toggle => {
      ...model,
      pressed: !model.pressed,
    }
  | Disabled(disabled) => {
      ...model,
      disabled,
    }
  | Hook => {
      ...model,
      hooks: model.hooks + 1,
    }
  },
  Cmd.none,
);
let view = model =>
  Html.div(
    [],
    [
      B.button(
        ~id="native",
        ~on_press=Press,
        ~disabled=model.disabled,
        [Html.text("Native")],
      ),
      B.button(
        ~id="custom",
        ~as_="div",
        ~on_press=Press,
        ~disabled=model.disabled,
        ~on_key_down=key => key == "Home" ? Some(Hook) : None,
        ~on_key_up=key => key == "End" ? Some(Hook) : None,
        [Html.text("Custom")],
      ),
      B.button(
        ~id="toggle",
        ~on_press=Toggle,
        ~pressed=model.pressed,
        [Html.text("Mute")],
      ),
      B.button(
        ~id="focusable",
        ~on_press=Press,
        ~disabled=true,
        ~focusable_when_disabled=true,
        [Html.text("Focusable disabled")],
      ),
      B.button(
        ~id="custom-focusable",
        ~as_="span",
        ~on_press=Press,
        ~disabled=true,
        ~focusable_when_disabled=true,
        [Html.text("Custom focusable disabled")],
      ),
      B.button(~id="submit", ~button_type=B.Submit, ~on_press=Press, []),
      B.button(~id="reset", ~button_type=B.Reset, ~on_press=Press, []),
      B.view(
        ~id="callback",
        ~on_press=Press,
        ~to_view=parts => Html.button(parts.root, [Html.text("Callback")]),
        (),
      ),
      Html.div(
        B.parts(~id="parts", ~native=false, ~on_press=Press, ()).root
        @ [Html.class_("example")],
        [Html.text("Parts")],
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
              count: 0,
              pressed: false,
              disabled: false,
              hooks: 0,
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
    "native default",
    attr("#native", "type") == Some("button")
    && attr("#native", "role") == None
    && attr("#native", "aria-pressed") == None,
  );
  check(
    "form types",
    attr("#submit", "type") == Some("submit")
    && attr("#reset", "type") == Some("reset"),
  );
  check(
    "neutral semantics",
    attr("#custom", "role") == Some("button")
    && attr("#custom", "tabindex") == Some("0"),
  );
  check(
    "parts rendering",
    attr("#parts", "role") == Some("button")
    && attr("#parts", "class") == Some("example"),
  );
  check(
    "toggle unpressed",
    attr("#toggle", "aria-pressed") == Some("false")
    && attr("#toggle", "data-pressed") == None,
  );
  check("enabled hooks absent", attr("#native", "data-disabled") == None);
  check(
    "enabled native click keeps default",
    emit("#native", "click") && app.model().count == 1,
  );
  check(
    "custom pointer click",
    !emit("#custom", "click") && app.model().count == 2,
  );
  pointer("#custom");
  check("pointerdown alone does not activate", app.model().count == 2);
  focus(node("#custom"));
  check("custom focus", has_focus("#custom"));
  check(
    "Enter activates on down",
    !key("#custom", "keydown", "Enter") && app.model().count == 3,
  );
  check(
    "Enter up ignored",
    key("#custom", "keyup", "Enter") && app.model().count == 3,
  );
  check(
    "Space down prevents without message",
    !key("#custom", "keydown", " ") && app.model().count == 3,
  );
  check(
    "Space up activates",
    !key("#custom", "keyup", " ") && app.model().count == 4,
  );
  List.iter(
    name => {
      List.iter(
        key_ => {
          check(
            "ctrl shortcut untouched",
            key(~ctrl=true, "#custom", name, key_),
          );
          check(
            "alt shortcut untouched",
            key(~alt=true, "#custom", name, key_),
          );
          check(
            "meta shortcut untouched",
            key(~meta=true, "#custom", name, key_),
          );
          check(
            "IME untouched",
            key(~composing=true, "#custom", name, key_),
          );
        },
        ["Enter", " ", "Home", "End"],
      );
      check("Tab untouched", key("#custom", name, "Tab"));
      check("Shift Tab untouched", key(~shift=true, "#custom", name, "Tab"));
      check("arrows untouched", key("#custom", name, "ArrowDown"));
    },
    ["keydown", "keyup"],
  );
  check(
    "shortcuts never activate",
    app.model().count == 4 && app.model().hooks == 0,
  );
  check(
    "hook does not prevent default",
    key("#custom", "keydown", "Home") && app.model().hooks == 1,
  );
  check(
    "keyup hook",
    key("#custom", "keyup", "End") && app.model().hooks == 2,
  );
  check(
    "native keyboard left to browser",
    key("#native", "keydown", "Enter")
    && key("#native", "keydown", " ")
    && app.model().count == 4,
  );
  click("#toggle");
  check(
    "toggle pressed",
    attr("#toggle", "aria-pressed") == Some("true")
    && attr("#toggle", "data-pressed") == Some(""),
  );
  /* Two intents before one render must update twice, not close over old state. */
  ignore(
    dispatch(
      node("#toggle"),
      event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
        },
      ),
    ),
  );
  ignore(
    dispatch(
      node("#toggle"),
      event(
        "click",
        {
          "bubbles": true,
          "cancelable": true,
        },
      ),
    ),
  );
  frame();
  check(
    "batched intents use latest state",
    app.model().pressed && attr("#toggle", "aria-pressed") == Some("true"),
  );
  click("#toggle");
  check("toggle resets hook", attr("#toggle", "data-pressed") == None);
  app.dispatch(Disabled(true));
  frame();
  check(
    "native disabled property",
    get(node("#native"), "disabled") == true
    && attr("#native", "data-disabled") == Some(""),
  );
  check(
    "neutral disabled",
    attr("#custom", "aria-disabled") == Some("true")
    && attr("#custom", "tabindex") == Some("-1"),
  );
  check(
    "focusable native uses ARIA not disabled",
    get(node("#focusable"), "disabled") == false
    && attr("#focusable", "aria-disabled") == Some("true")
    && attr("#focusable", "tabindex") == Some("0"),
  );
  check(
    "focusable neutral",
    attr("#custom-focusable", "tabindex") == Some("0"),
  );
  let before = app.model().count;
  List.iter(
    selector => {
      check("disabled click prevents", !emit(selector, "click"));
      check("disabled Enter down", !key(selector, "keydown", "Enter"));
      check("disabled Space down", !key(selector, "keydown", " "));
      check("disabled Space up", !key(selector, "keyup", " "));
    },
    ["#native", "#custom", "#focusable", "#custom-focusable"],
  );
  check("disabled never activates", app.model().count == before);
  focus(node("#focusable"));
  check("focusable disabled native focus", has_focus("#focusable"));
  focus(node("#custom-focusable"));
  check("focusable disabled neutral focus", has_focus("#custom-focusable"));
  app.dispatch(Disabled(false));
  frame();
  check(
    "disabled reset",
    get(node("#native"), "disabled") == false
    && attr("#native", "data-disabled") == None
    && attr("#custom", "aria-disabled") == None
    && attr("#custom", "tabindex") == Some("0"),
  );
  let before = app.model().count;
  click("#callback");
  check(
    "callback view native defaults and events",
    attr("#callback", "type") == Some("button")
    && app.model().count == before
    + 1,
  );
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("button");
  Js.Promise.resolve();
};
