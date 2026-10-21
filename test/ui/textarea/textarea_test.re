open Tea;
open Test_support;
module F = Ui.Textarea;
type msg =
  | Input(string)
  | Change(string)
  | Blur
  | Flags(bool)
  | Presence(bool, bool)
  | Readonly(bool)
  | Value(string);
type model = {
  value: string,
  changed: string,
  inputs: int,
  blurs: int,
  flags: bool,
  description: bool,
  error: bool,
  readonly: bool,
};
let update = (msg, m) => (
  switch (msg) {
  | Input(value) => {
      ...m,
      value,
      inputs: m.inputs + 1,
    }
  | Change(changed) => {
      ...m,
      changed,
    }
  | Blur => {
      ...m,
      blurs: m.blurs + 1,
    }
  | Flags(flags) => {
      ...m,
      flags,
    }
  | Presence(description, error) => {
      ...m,
      description,
      error,
    }
  | Readonly(readonly) => {
      ...m,
      readonly,
    }
  | Value(value) => {
      ...m,
      value,
    }
  },
  Cmd.none,
);
let view = m =>
  F.view(
    ~id="field",
    ~value=m.value,
    ~on_input=value => Input(value),
    ~on_change=value => Change(value),
    ~on_blur=Blur,
    ~invalid=m.flags,
    ~disabled=m.flags,
    ~required=m.flags,
    ~readonly=m.readonly,
    ~has_description=m.description,
    ~has_error=m.error,
    ~name="message",
    ~rows=5,
    ~to_view=
      p => {
        let minimal =
          F.parts(
            ~id="minimal",
            ~value="",
            ~on_input=value => Input(value),
            (),
          );
        let clamped =
          F.parts(
            ~id="clamped",
            ~value="",
            ~on_input=value => Input(value),
            ~rows=0,
            (),
          );
        Html.div(
          [],
          [
            Html.label(minimal.label, [Html.text("Minimal")]),
            Html.textarea(minimal.control, []),
            Html.p(minimal.description, [Html.text("Hidden description")]),
            Html.p(minimal.error, [Html.text("Hidden error")]),
            Html.textarea(clamped.control, []),
            Html.label(p.label, [Html.text("Message")]),
            Html.textarea(p.control, []),
            m.description
              ? Html.p(p.description, [Html.text("Help")]) : Html.text(""),
            m.error
              ? Html.p(p.error, [Html.text("Error")]) : Html.text(""),
          ],
        );
      },
    (),
  );
let completed = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              value: "initial",
              changed: "",
              inputs: 0,
              blurs: 0,
              flags: false,
              description: false,
              error: false,
              readonly: false,
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
    "native element",
    get(node("#field-control"), "tagName") == "TEXTAREA",
  );
  check(
    "label points at control",
    attr("#field-label", "for") == Some("field-control"),
  );
  check(
    "initial controlled value",
    get(node("#field-control"), "value") == "initial",
  );
  check("name", attr("#field-control", "name") == Some("message"));
  check("omitted name", attr("#minimal-control", "name") == None);
  check(
    "absent parts hidden",
    get(node("#minimal-description"), "hidden") == true
    && get(node("#minimal-error"), "hidden") == true,
  );
  check(
    "minimal enabled writable optional",
    get(node("#minimal-control"), "disabled") == false
    && get(node("#minimal-control"), "readOnly") == false
    && get(node("#minimal-control"), "required") == false,
  );
  check(
    "default rows/clamped rows",
    attr("#minimal-control", "rows") == Some("3")
    && attr("#clamped-control", "rows") == Some("1"),
  );
  ignore(emit("#minimal-control", "change"));
  ignore(emit("#minimal-control", "blur"));
  check(
    "absent hooks dispatch nothing",
    app.model().inputs == 0
    && app.model().blurs == 0
    && app.model().changed == "",
  );
  check("rows", attr("#field-control", "rows") == Some("5"));
  check(
    "no dangling initial refs",
    attr("#field-control", "aria-describedby") == None
    && attr("#field-control", "aria-errormessage") == None,
  );
  focus(node("#field-control"));
  check("focusable", has_focus("#field-control"));
  List.iter(
    ((description, error)) => {
      app.dispatch(Presence(description, error));
      frame();
      let expected =
        (description ? "field-description" : "")
        ++ (description && error ? " " : "")
        ++ (error ? "field-error" : "");
      check(
        "presence ids",
        attr("#field-control", "aria-describedby")
        == (String.equal(expected, "") ? None : Some(expected)),
      );
      check(
        "no errormessage while valid",
        attr("#field-control", "aria-errormessage") == None,
      );
      check("presence preserves focus", has_focus("#field-control"));
    },
    [(true, false), (false, true), (true, true), (false, false)],
  );
  put(node("#field-control"), "value", "typed");
  ignore(emit("#field-control", "input"));
  check(
    "input actual value",
    app.model().value == "typed" && app.model().inputs == 1,
  );
  put(node("#field-control"), "value", "committed");
  ignore(emit("#field-control", "change"));
  check("change hook actual value", app.model().changed == "committed");
  check(
    "rejected change restores controlled value",
    get(node("#field-control"), "value") == "typed",
  );
  ignore(emit("#field-control", "blur"));
  check("blur hook", app.model().blurs == 1);
  List.iter(
    value => {
      put(node("#field-control"), "value", value);
      ignore(
        dispatch(
          node("#field-control"),
          event("input", { "bubbles": true }),
        ),
      );
    },
    ["one", "two"],
  );
  frame();
  check(
    "rapid actual events",
    app.model().value == "two" && app.model().inputs == 3,
  );
  check("Tab unclaimed", key("#field-control", "keydown", "Tab"));
  check(
    "modified edit unclaimed",
    key(~ctrl=true, "#field-control", "keydown", "a"),
  );
  check(
    "IME unclaimed",
    key(~composing=true, "#field-control", "keydown", "Enter"),
  );
  app.dispatch(Flags(true));
  app.dispatch(Presence(true, true));
  frame();
  check(
    "invalid aria/hooks",
    attr("#field-control", "aria-invalid") == Some("true")
    && attr("#field-control", "data-invalid") == Some("")
    && attr("#field-control", "aria-errormessage") == Some("field-error"),
  );
  check(
    "required/disabled native",
    get(node("#field-control"), "required") == true
    && get(node("#field-control"), "disabled") == true
    && attr("#field-control", "data-disabled") == Some(""),
  );
  let before = app.model().inputs;
  put(node("#field-control"), "value", "ignored");
  ignore(emit("#field-control", "input"));
  ignore(emit("#field-control", "change"));
  check(
    "disabled blocks synthetic events",
    app.model().inputs == before && app.model().changed == "committed",
  );
  app.dispatch(Presence(false, false));
  frame();
  check(
    "invalid absent error has no refs",
    attr("#field-control", "aria-errormessage") == None
    && attr("#field-control", "aria-describedby") == None,
  );
  app.dispatch(Flags(false));
  app.dispatch(Readonly(true));
  frame();
  check(
    "readonly native/hook",
    get(node("#field-control"), "readOnly") == true
    && attr("#field-control", "data-readonly") == Some(""),
  );
  put(node("#field-control"), "value", "ignored");
  ignore(emit("#field-control", "input"));
  ignore(emit("#field-control", "change"));
  check(
    "readonly rejects input/change",
    app.model().inputs == before && app.model().changed == "committed",
  );
  focus(node("#field-control"));
  check("readonly retains focusability", has_focus("#field-control"));
  app.dispatch(Readonly(false));
  app.dispatch(Value("programmatic"));
  frame();
  check(
    "programmatic value no event",
    get(node("#field-control"), "value") == "programmatic"
    && app.model().inputs == before,
  );
  check(
    "states cleared",
    attr("#field-control", "data-invalid") == None
    && attr("#field-control", "aria-invalid") == None
    && attr("#field-control", "data-disabled") == None
    && attr("#field-control", "data-readonly") == None
    && get(node("#field-control"), "required") == false,
  );
  app.shutdown();
  frame();
  check("shutdown no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("shutdown no globals", listener_count() == 0);
  report("textarea");
  Js.Promise.resolve();
};
