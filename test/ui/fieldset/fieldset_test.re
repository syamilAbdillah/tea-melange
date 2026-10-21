open Tea;
open Test_support;
module F = Ui.Fieldset;
type msg =
  | Disable(bool)
  | Invalid(bool)
  | Describe(bool)
  | Toggle;
type model = {
  disabled: bool,
  invalid: bool,
  description: bool,
  checked: bool,
};
let update = (msg, m) => (
  switch (msg) {
  | Disable(disabled) => {
      ...m,
      disabled,
    }
  | Invalid(invalid) => {
      ...m,
      invalid,
    }
  | Describe(description) => {
      ...m,
      description,
    }
  | Toggle => {
      ...m,
      checked: !m.checked,
    }
  },
  Cmd.none,
);
let view = m =>
  F.view(
    ~id="group",
    ~disabled=m.disabled,
    ~invalid=m.invalid,
    ~has_description=m.description,
    ~to_view=
      p =>
        Html.fieldset(
          p.root,
          [
            Html.legend(
              p.legend,
              [
                Html.text("Preferences"),
                Html.input([Html.id("legend-input")], []),
              ],
            ),
            Html.input([Html.id("native")], []),
            Ui.Checkbox.view(
              ~id="custom",
              ~native=false,
              ~checked=m.checked,
              ~disabled=p.disabled,
              ~on_change=Toggle,
              ~to_view=
                c =>
                  Html.div(
                    [],
                    [
                      Html.div(
                        c.control,
                        [Html.span(c.label, [Html.text("Custom")])],
                      ),
                      c.hidden_input,
                    ],
                  ),
              (),
            ),
            m.description
              ? Html.p(p.description, [Html.text("Description")])
              : Html.text(""),
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
              disabled: false,
              invalid: false,
              description: false,
              checked: false,
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
    "native fieldset",
    get(node("#group-root"), "tagName") == "FIELDSET",
  );
  check("legend", get(node("#group-legend"), "tagName") == "LEGEND");
  check(
    "no dangling description",
    attr("#group-root", "aria-describedby") == None,
  );
  app.dispatch(Describe(true));
  frame();
  check(
    "description points to present part",
    attr("#group-root", "aria-describedby") == Some("group-description")
    && get(node("#group-description"), "textContent") == "Description",
  );
  click("#custom-control");
  check("child enabled", app.model().checked);
  app.dispatch(Disable(true));
  app.dispatch(Invalid(true));
  frame();
  check(
    "fieldset native disabled",
    get(node("#group-root"), "disabled") == true,
  );
  check(
    "inherited disabled native selector",
    query_all(document, "#native:disabled")->Array.length == 1,
  );
  check(
    "legend exception native selector",
    query_all(document, "#legend-input:disabled")->Array.length == 0,
  );
  check(
    "custom disabled passed through parts",
    attr("#custom-control", "aria-disabled") == Some("true")
    && attr("#custom-control", "tabindex") == Some("-1"),
  );
  check(
    "presence hooks",
    attr("#group-root", "data-disabled") == Some("")
    && attr("#group-root", "data-invalid") == Some(""),
  );
  click("#custom-control");
  ignore(key("#custom-control", "keyup", " "));
  frame();
  check("disabled custom never changes", app.model().checked);
  check("Tab untouched", key("#group-root", "keydown", "Tab"));
  app.dispatch(Describe(false));
  app.dispatch(Disable(false));
  app.dispatch(Invalid(false));
  frame();
  check(
    "cleared refs/hooks",
    attr("#group-root", "aria-describedby") == None
    && attr("#group-root", "data-disabled") == None
    && attr("#group-root", "data-invalid") == None,
  );
  click("#custom-control");
  check("reenabled child", !app.model().checked);
  app.shutdown();
  frame();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no globals", listener_count() == 0);
  report("fieldset");
  Js.Promise.resolve();
};
