open Tea;
open Test_support;
module F = Ui.Checkbox;
[@mel.send] external activate: Dom.node => unit = "click";
[@mel.new] [@mel.scope "window"]
external form_data: Dom.node => Js.t({..}) = "FormData";
[@mel.send]
external values: (Js.t({..}), string) => array(string) = "getAll";
type msg =
  | Toggle(bool)
  | Disabled(bool)
  | Mixed(bool)
  | Accept(bool)
  | Describe(bool);
type model = {
  native: bool,
  custom: bool,
  mixed: bool,
  disabled: bool,
  accept: bool,
  description: bool,
  count: int,
};
let update = (msg, m) => (
  switch (msg) {
  | Toggle(native) when m.accept && !m.disabled => {
      ...m,
      native:
        native
          ? F.next_checked(~checked=m.native, ~indeterminate=m.mixed, ())
          : m.native,
      custom:
        native
          ? m.custom
          : F.next_checked(~checked=m.custom, ~indeterminate=m.mixed, ()),
      mixed: false,
      count: m.count + 1,
    }
  | Toggle(_) => m
  | Disabled(disabled) => {
      ...m,
      disabled,
    }
  | Mixed(mixed) => {
      ...m,
      mixed,
    }
  | Accept(accept) => {
      ...m,
      accept,
    }
  | Describe(description) => {
      ...m,
      description,
    }
  },
  Cmd.none,
);
let view = m =>
  Html.form(
    [Html.id("form")],
    List.map(
      native =>
        F.view(
          ~id=native ? "native" : "custom",
          ~native,
          ~checked=native ? m.native : m.custom,
          ~on_change=Toggle(native),
          ~indeterminate=m.mixed,
          ~disabled=m.disabled,
          ~has_description=m.description,
          ~name=native ? "native" : "custom",
          ~value="yes",
          ~to_view=
            p =>
              Html.div(
                [],
                [
                  native
                    ? Html.input(p.control, [])
                    : Html.div(
                        p.control,
                        [Html.span(p.label, [Html.text("Custom")])],
                      ),
                  native
                    ? Html.label(p.label, [Html.text("Native")])
                    : Html.text(""),
                  m.description
                    ? Html.p(p.description, [Html.text("Help")])
                    : Html.text(""),
                  p.hidden_input,
                ],
              ),
          (),
        ),
      [true, false],
    ),
  );
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              native: false,
              custom: false,
              mixed: true,
              disabled: false,
              accept: true,
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
  check("next unchecked", F.next_checked(~checked=false, ()));
  check("next checked", !F.next_checked(~checked=true, ()));
  check(
    "mixed always checks",
    F.next_checked(~checked=true, ~indeterminate=true, ()),
  );
  check("native type", attr("#native-control", "type") == Some("checkbox"));
  check(
    "native mixed property",
    get(node("#native-control"), "indeterminate") == true,
  );
  check(
    "custom role/tabindex/mixed",
    attr("#custom-control", "role") == Some("checkbox")
    && attr("#custom-control", "tabindex") == Some("0")
    && attr("#custom-control", "aria-checked") == Some("mixed"),
  );
  check(
    "native label",
    attr("#native-label", "for") == Some("native-control"),
  );
  check(
    "custom label",
    attr("#custom-control", "aria-labelledby") == Some("custom-label"),
  );
  check(
    "no description ref",
    attr("#custom-control", "aria-describedby") == None,
  );
  check(
    "unchecked custom not submitted",
    Array.length(values(form_data(node("#form")), "custom")) == 0,
  );
  app.dispatch(Accept(false));
  frame();
  activate(node("#native-control"));
  frame();
  check(
    "rejected native activation restores mixed",
    get(node("#native-control"), "indeterminate") == true
    && get(node("#native-control"), "checked") == false
    && app.model().count == 0,
  );
  app.dispatch(Accept(true));
  frame();
  activate(node("#native-control"));
  frame();
  check(
    "native mixed goes checked",
    app.model().native
    && !app.model().mixed
    && get(node("#native-control"), "checked") == true
    && get(node("#native-control"), "indeterminate") == false,
  );
  check(
    "native checked submits value",
    values(form_data(node("#form")), "native") == [|"yes"|],
  );
  app.dispatch(Mixed(true));
  frame();
  check(
    "mixed set again",
    get(node("#native-control"), "indeterminate") == true,
  );
  activate(node("#native-control"));
  frame();
  check(
    "checked plus mixed goes checked",
    app.model().native
    && get(node("#native-control"), "checked") == true
    && get(node("#native-control"), "indeterminate") == false,
  );
  let before = app.model().count;
  check("Space down prevented", !key("#custom-control", "keydown", " "));
  check("Space down no toggle", app.model().count == before);
  check("Space up prevented", !key("#custom-control", "keyup", " "));
  frame();
  check(
    "Space up toggles once",
    app.model().custom && app.model().count == before + 1,
  );
  check(
    "custom true state/hooks",
    attr("#custom-control", "aria-checked") == Some("true")
    && attr("#custom-control", "data-checked") == Some("")
    && attr("#custom-control", "data-indeterminate") == None,
  );
  check(
    "custom form value",
    values(form_data(node("#form")), "custom") == [|"yes"|],
  );
  List.iter(
    ((ctrl, alt, meta, composing)) => {
      check(
        "shortcut down untouched",
        key(
          ~ctrl,
          ~alt,
          ~meta,
          ~composing,
          "#custom-control",
          "keydown",
          " ",
        ),
      );
      check(
        "shortcut up untouched",
        key(~ctrl, ~alt, ~meta, ~composing, "#custom-control", "keyup", " "),
      );
    },
    [
      (true, false, false, false),
      (false, true, false, false),
      (false, false, true, false),
      (false, false, false, true),
    ],
  );
  check("Enter unclaimed", key("#custom-control", "keydown", "Enter"));
  check("Tab unclaimed", key("#custom-control", "keydown", "Tab"));
  check("native Space unclaimed", key("#native-control", "keydown", " "));
  check("unhandled keys no toggle", app.model().count == before + 1);
  let before = app.model().count;
  ignore(key("#custom-control", "keyup", " "));
  ignore(key("#custom-control", "keyup", " "));
  frame();
  check(
    "rapid custom intents compose",
    app.model().custom && app.model().count == before + 2,
  );
  activate(node("#native-control"));
  activate(node("#native-control"));
  frame();
  check(
    "rapid native intents compose",
    app.model().native && app.model().count == before + 4,
  );
  activate(node("#custom-label"));
  frame();
  check("custom label bubbles activation", !app.model().custom);
  app.dispatch(Describe(true));
  frame();
  check(
    "description refs native/custom",
    attr("#native-control", "aria-describedby") == Some("native-description")
    && attr("#custom-control", "aria-describedby")
    == Some("custom-description"),
  );
  app.dispatch(Disabled(true));
  frame();
  check(
    "native disabled",
    get(node("#native-control"), "disabled") == true
    && attr("#native-control", "data-disabled") == Some(""),
  );
  check(
    "custom disabled",
    attr("#custom-control", "aria-disabled") == Some("true")
    && attr("#custom-control", "tabindex") == Some("-1"),
  );
  let before = app.model().count;
  activate(node("#native-control"));
  click("#custom-control");
  ignore(key("#custom-control", "keyup", " "));
  frame();
  check("disabled no toggles", app.model().count == before);
  check(
    "disabled native form omitted",
    Array.length(values(form_data(node("#form")), "native")) == 0,
  );
  app.dispatch(Disabled(false));
  app.dispatch(Describe(false));
  frame();
  check(
    "reenabled hooks/reset refs",
    attr("#custom-control", "data-disabled") == None
    && attr("#custom-control", "aria-disabled") == None
    && attr("#native-control", "aria-describedby") == None,
  );
  focus(node("#custom-control"));
  click("#custom-control");
  check("toggle retains focus", has_focus("#custom-control"));
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no global listeners", listener_count() == 0);
  report("checkbox");
  Js.Promise.resolve();
};
