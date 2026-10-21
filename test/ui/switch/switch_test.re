open Tea;
open Test_support;
module F = Ui.Switch_;
[@mel.send] external activate: Dom.node => unit = "click";
[@mel.new] [@mel.scope "window"]
external form_data: Dom.node => Js.t({..}) = "FormData";
[@mel.send]
external values: (Js.t({..}), string) => array(string) = "getAll";
type msg =
  | Toggle
  | Disabled(bool)
  | Describe(bool)
  | Checked(bool);
type model = {
  checked: bool,
  disabled: bool,
  description: bool,
  count: int,
};
let update = (msg, m) => (
  switch (msg) {
  | Toggle => {
      ...m,
      checked: !m.checked,
      count: m.count + 1,
    }
  | Disabled(disabled) => {
      ...m,
      disabled,
    }
  | Describe(description) => {
      ...m,
      description,
    }
  | Checked(checked) => {
      ...m,
      checked,
    }
  },
  Cmd.none,
);
let view = m =>
  Html.form(
    [Html.id("form")],
    [
      F.view(
        ~id="switch",
        ~checked=m.checked,
        ~on_change=Toggle,
        ~disabled=m.disabled,
        ~has_description=m.description,
        ~name="notifications",
        ~value="enabled",
        ~to_view=
          p =>
            Html.div(
              [],
              [
                Html.label(p.label, [Html.text("Notifications")]),
                Html.button(p.root, [Html.text(m.checked ? "On" : "Off")]),
                p.hidden_input,
                m.description
                  ? Html.p(p.description, [Html.text("Help")])
                  : Html.text(""),
              ],
            ),
        (),
      ),
      F.view(
        ~id="unnamed",
        ~checked=m.checked,
        ~on_change=Toggle,
        ~to_view=
          p =>
            Html.div(
              [],
              [
                Html.label(p.label, [Html.text("Unnamed")]),
                Html.button(p.root, []),
                p.hidden_input,
              ],
            ),
        (),
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
              checked: false,
              disabled: false,
              description: false,
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
    "native button switch",
    get(node("#switch-control"), "tagName") == "BUTTON"
    && attr("#switch-control", "type") == Some("button")
    && attr("#switch-control", "role") == Some("switch"),
  );
  check(
    "off state",
    attr("#switch-control", "aria-checked") == Some("false")
    && attr("#switch-control", "data-checked") == None,
  );
  check(
    "label idref",
    attr("#switch-control", "aria-labelledby") == Some("switch-label")
    && attr("#switch-label", "for") == Some("switch-control"),
  );
  check(
    "no description idref",
    attr("#switch-control", "aria-describedby") == None,
  );
  check(
    "off not successful",
    Array.length(values(form_data(node("#form")), "notifications")) == 0,
  );
  check(
    "no unnamed input",
    Array.length(query_all(document, "#unnamed-control ~ input")) == 0,
  );
  focus(node("#switch-control"));
  activate(node("#switch-control"));
  frame();
  check(
    "click on",
    app.model().checked
    && attr("#switch-control", "aria-checked") == Some("true")
    && attr("#switch-control", "data-checked") == Some(""),
  );
  check(
    "on successful",
    values(form_data(node("#form")), "notifications") == [|"enabled"|],
  );
  check("retains focus", has_focus("#switch-control"));
  activate(node("#switch-control"));
  activate(node("#switch-control"));
  frame();
  check(
    "rapid intents compose",
    app.model().checked && app.model().count == 3,
  );
  let before = app.model().count;
  List.iter(
    key_ => {
      check(
        "native keydown unclaimed",
        key("#switch-control", "keydown", key_),
      );
      check("native keyup unclaimed", key("#switch-control", "keyup", key_));
    },
    [" ", "Enter", "Tab", "Escape", "ArrowDown"],
  );
  check(
    "native key events no synthetic double activation",
    app.model().count == before,
  );
  activate(node("#switch-label"));
  frame();
  check(
    "native label forwards exactly once",
    !app.model().checked && app.model().count == before + 1,
  );
  app.dispatch(Describe(true));
  frame();
  check(
    "description linked",
    attr("#switch-control", "aria-describedby") == Some("switch-description"),
  );
  app.dispatch(Checked(true));
  frame();
  check(
    "programmatic checked no event",
    app.model().count == before
    + 1
    && attr("#switch-control", "aria-checked") == Some("true"),
  );
  app.dispatch(Disabled(true));
  frame();
  check(
    "disabled semantics/hooks",
    get(node("#switch-control"), "disabled") == true
    && attr("#switch-control", "data-disabled") == Some(""),
  );
  activate(node("#switch-control"));
  activate(node("#switch-label"));
  click("#switch-control");
  check(
    "disabled blocks native and synthetic clicks",
    app.model().count == before + 1,
  );
  check(
    "disabled form omitted",
    Array.length(values(form_data(node("#form")), "notifications")) == 0,
  );
  app.dispatch(Disabled(false));
  app.dispatch(Describe(false));
  frame();
  check(
    "reset disabled/ref",
    get(node("#switch-control"), "disabled") == false
    && attr("#switch-control", "data-disabled") == None
    && attr("#switch-control", "aria-describedby") == None,
  );
  activate(node("#switch-control"));
  frame();
  check("reenabled toggles", !app.model().checked);
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("switch");
  Js.Promise.resolve();
};
