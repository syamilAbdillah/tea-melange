open Tea;
open Test_support;
module P = Ui.Date_picker;
module C = Ui.Calendar;
module V = Tea_vdom.Vdom;
let date = iso => Option.value(~default=C.Date.first, C.Date.of_iso(iso));
let today = date("2026-03-05");
let settle = () =>
  for (_i in 1 to 18) {
    flush();
  };
type msg =
  | Progress(P.msg)
  | Open
  | Close
  | Set(option(C.date))
  | Modes(bool, bool)
  | Limits(C.constraints)
  | Remove;
type model = {
  picker: P.model,
  outs: list(P.out_msg),
  disabled: bool,
  readonly: bool,
  limits: C.constraints,
  removed: bool,
};
let no_duplicates = facts => {
  let names =
    List.filter_map(
      fact =>
        switch (fact) {
        | V.Attribute("", _) => None
        | V.Attribute(name, _)
        | V.Property(name, _)
        | V.Event(name, _) => Some(name)
        | _ => None
        },
      facts,
    );
  check(
    "no conflicting owned facts",
    List.length(names) == List.length(List.sort_uniq(String.compare, names)),
  );
};
let render = (model, ~unmount, ~labels, ~required) =>
  P.view(
    ~model=model.picker,
    ~constraints=model.limits,
    ~disabled=model.disabled,
    ~readonly=model.readonly,
    ~has_description=true,
    ~labels,
    ~required,
    ~name="visit",
    ~today,
    ~to_parent=message => Progress(message),
    ~to_view=
      p => {
        List.iter(
          no_duplicates,
          [
            p.group,
            p.label,
            p.input,
            p.trigger,
            p.dialog,
            p.start_boundary,
            p.end_boundary,
            p.instructions,
            p.confirm_button,
            p.cancel_button,
            p.close_button,
          ],
        );
        Html.div(
          p.group,
          [
            Html.label(p.label, [Html.text("Visit date")]),
            Html.input(p.input, []),
            Html.button(p.trigger, [Html.text(labels.P.choose)]),
            unmount && !p.rendered
              ? Html.text("")
              : Html.div(
                  p.dialog,
                  [
                    Html.span(p.start_boundary, []),
                    Html.div(
                      p.calendar.root,
                      [
                        Html.h2(
                          p.calendar.heading,
                          [Html.text(p.calendar.heading_text)],
                        ),
                        Html.button(
                          p.calendar.previous_year,
                          [Html.text("Previous year")],
                        ),
                        Html.button(
                          p.calendar.previous_month,
                          [Html.text("Previous month")],
                        ),
                        Html.button(
                          p.calendar.next_month,
                          [Html.text("Next month")],
                        ),
                        Html.button(
                          p.calendar.next_year,
                          [Html.text("Next year")],
                        ),
                        Html.table(
                          p.calendar.grid,
                          [
                            Html.thead(
                              [],
                              [
                                Html.tr(
                                  [],
                                  Array.to_list(
                                    Array.map(
                                      (day: C.weekday(msg)) =>
                                        Html.th(
                                          day.attrs,
                                          [Html.text(day.text)],
                                        ),
                                      p.calendar.weekdays,
                                    ),
                                  ),
                                ),
                              ],
                            ),
                            Html.tbody(
                              [],
                              Array.to_list(
                                Array.map(
                                  week =>
                                    Html.tr(
                                      [],
                                      Array.to_list(
                                        Array.map(
                                          (cell: C.cell(msg)) =>
                                            Html.td(
                                              cell.attrs,
                                              [Html.text(cell.text)],
                                            ),
                                          week,
                                        ),
                                      ),
                                    ),
                                  p.calendar.weeks,
                                ),
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                    p.confirm
                      ? Html.button(
                          p.confirm_button,
                          [Html.text(labels.P.confirm)],
                        )
                      : Html.text(""),
                    Html.button(
                      p.cancel_button,
                      [Html.text(labels.P.cancel)],
                    ),
                    Html.button(
                      p.close_button,
                      [Html.text(labels.P.close)],
                    ),
                    Html.p(
                      p.instructions,
                      [Html.text(p.instructions_text)],
                    ),
                    Html.span(p.end_boundary, []),
                  ],
                ),
            Html.p(p.description, [Html.text("YYYY-MM-DD")]),
            Html.p(p.error, [Html.text(p.error_text)]),
            Html.input(p.hidden_input, []),
          ],
        );
      },
    (),
  );
let start =
    (
      ~confirm=false,
      ~selected=?,
      ~focus=today,
      ~format=C.Date.to_iso,
      ~parse=C.Date.of_iso,
      ~dir=C.Ltr,
      ~disabled=false,
      ~readonly=false,
      ~limits=C.unconstrained,
      ~unmount=false,
      ~top_layer=false,
      ~labels=P.english,
      ~required=false,
      (),
    ) =>
  App.element(
    ~init=
      () =>
        (
          {
            picker:
              P.init(
                ~id="test",
                ~focus,
                ~selected?,
                ~confirm,
                ~format,
                ~parse,
                ~dir,
                ~top_layer,
                (),
              ),
            outs: [],
            disabled,
            readonly,
            limits,
            removed: false,
          },
          Cmd.none,
        ),
    ~update=
      (message, model) => {
        let (picker, command, outs) =
          switch (message) {
          | Progress(message) =>
            P.update(
              ~constraints=model.limits,
              ~disabled=model.disabled,
              ~readonly=model.readonly,
              message,
              model.picker,
            )
          | Open =>
            let (picker, command) =
              P.open_(
                ~constraints=model.limits,
                ~disabled=model.disabled,
                model.picker,
              );
            (picker, command, []);
          | Close =>
            let (picker, command) = P.close(model.picker);
            (picker, command, []);
          | Set(value) => (P.set_value(value, model.picker), Cmd.none, [])
          | Modes(_, _)
          | Limits(_)
          | Remove => (model.picker, Cmd.none, [])
          };
        (
          {
            picker,
            outs: model.outs @ outs,
            disabled:
              switch (message) {
              | Modes(value, _) => value
              | _ => model.disabled
              },
            readonly:
              switch (message) {
              | Modes(_, value) => value
              | _ => model.readonly
              },
            limits:
              switch (message) {
              | Limits(value) => value
              | _ => model.limits
              },
            removed: model.removed || message == Remove,
          },
          Cmd.map(message => Progress(message), command),
        );
      },
    ~view=
      model =>
        Html.div(
          [],
          [
            model.removed
              ? Html.text("") : render(model, ~unmount, ~labels, ~required),
            Html.button([Html.id("outside")], [Html.text("Outside")]),
            Html.h1([Html.id("nonfocusable")], [Html.text("Dates")]),
          ],
        ),
    ~subscriptions=
      model =>
        model.removed
          ? Sub.none
          : Sub.map(
              message => Progress(message),
              P.subscriptions(model.picker),
            ),
    ~node="#app",
    ~flags=(),
    (),
  );
let layout = () =>
  put(node("#test-group"), "getBoundingClientRect", () =>
    {
      "x": 50.,
      "y": 50.,
      "top": 50.,
      "left": 50.,
      "right": 250.,
      "bottom": 90.,
      "width": 200.,
      "height": 40.,
    }
  );
let stop = app => {
  app.Tea_core.Runtime.shutdown();
  settle();
  check("no root listeners", Tea_dom.Debug.active_listeners() == 0);
  check("no document/window listeners", listener_count() == 0);
};
let input = "#test-input-control";
let trigger = "#test-trigger";
let active_cell = "#test-panel [role=gridcell][tabindex='0']";
let focus_date = (app, expected) => {
  check(
    "roving date",
    C.Date.to_iso(P.focus_date(app.Tea_core.Runtime.model().picker))
    == expected,
  );
  check("DOM follows roving date", has_focus(active_cell));
};
let edit = text => {
  put(node(input), "value", text);
  ignore(emit(input, "input"));
};
let enter = () => {
  check("Enter input is claimed", !key(input, "keydown", "Enter"));
  settle();
};
let value = app =>
  Option.map(C.Date.to_iso, P.value(app.Tea_core.Runtime.model().picker));
let latest = app => List.hd(List.rev(app.Tea_core.Runtime.model().outs));
let idrefs = () =>
  Array.iter(
    node =>
      List.iter(
        name =>
          Option.iter(
            value =>
              List.iter(
                id =>
                  check(
                    "idref resolves",
                    !Js.testAny(query(document, "[id='" ++ id ++ "']")),
                  ),
                String.split_on_char(' ', value),
              ),
            Js.Nullable.toOption(attribute(node, name)),
          ),
        [
          "for",
          "aria-controls",
          "aria-labelledby",
          "aria-describedby",
          "aria-errormessage",
        ],
      ),
    query_all(
      document,
      "[for],[aria-controls],[aria-labelledby],[aria-describedby],[aria-errormessage]",
    ),
  );
[@mel.send]
external listen: (Dom.document, string, Dom.event => unit) => unit =
  "addEventListener";
[@mel.send]
external unlisten: (Dom.document, string, Dom.event => unit) => unit =
  "removeEventListener";
let escape = selector => {
  let reached = ref(false);
  let listener = _ => reached := true;
  listen(document, "keydown", listener);
  let accepted = key(selector, "keydown", "Escape");
  unlisten(document, "keydown", listener);
  check(
    "claimed Escape prevented and stopped at document",
    !accepted && ! reached^,
  );
  settle();
};
let completed = {
  let app = start(~selected=today, ());
  layout();
  check(
    "initial value",
    value(app) == Some("2026-03-05")
    && P.text(app.model().picker) == "2026-03-05",
  );
  check(
    "native input label",
    attr("#test-input-label", "for") == Some("test-input-control"),
  );
  check(
    "Choose accessible name includes selected civil date",
    attr(trigger, "aria-label")
    == Some("Choose date, selected date is March 5, 2026"),
  );
  check(
    "hidden ISO form value",
    get(node("input[name=visit]"), "value") == "2026-03-05",
  );
  check(
    "closed popup hidden without CSS",
    get(node("#test-panel"), "hidden"),
  );
  check("closed no controls idref", attr(trigger, "aria-controls") == None);
  idrefs();
  let original = app.model().picker;
  focus(node(trigger));
  click(trigger);
  settle();
  check("opened notification", latest(app) == P.Opened);
  check(
    "immutable original model",
    !P.is_open(original) && P.text(original) == "2026-03-05",
  );
  check(
    "semantic modal name",
    attr("#test-panel", "role") == Some("dialog")
    && attr("#test-panel", "aria-modal") == Some("true")
    && attr("#test-panel", "aria-labelledby")
    == Some("test-calendar-heading"),
  );
  check(
    "open presence",
    attr(trigger, "data-open") == Some("")
    && attr("#test-panel", "data-open") == Some(""),
  );
  check(
    "controls references rendered panel",
    attr(trigger, "aria-controls") == Some("test-panel"),
  );
  check(
    "live instructions after focus",
    String.length(get(node("#test-instructions"), "textContent")) > 0,
  );
  focus_date(app, "2026-03-05");
  idrefs();
  List.iter(
    ((key_, expected)) => {
      check("Calendar key claimed", !key(active_cell, "keydown", key_));
      settle();
      focus_date(app, expected);
    },
    [
      ("ArrowLeft", "2026-03-04"),
      ("ArrowRight", "2026-03-05"),
      ("ArrowUp", "2026-02-26"),
      ("ArrowDown", "2026-03-05"),
      ("Home", "2026-03-01"),
      ("End", "2026-03-07"),
      ("PageUp", "2026-02-07"),
      ("PageDown", "2026-03-07"),
    ],
  );
  ignore(key(~shift=true, active_cell, "keydown", "PageUp"));
  settle();
  focus_date(app, "2025-03-07");
  ignore(key(~shift=true, active_cell, "keydown", "PageDown"));
  settle();
  focus_date(app, "2026-03-07");
  List.iter(
    key_ =>
      check(
        "unclaimed key retains default",
        key(active_cell, "keydown", key_),
      ),
    ["Tab", "F6"],
  );
  List.iter(
    selector => {
      check(
        "ctrl Escape native",
        key(~ctrl=true, selector, "keydown", "Escape"),
      );
      check(
        "alt Escape native",
        key(~alt=true, selector, "keydown", "Escape"),
      );
      check(
        "meta Escape native",
        key(~meta=true, selector, "keydown", "Escape"),
      );
      check(
        "IME Escape native",
        key(~composing=true, selector, "keydown", "Escape"),
      );
    },
    [active_cell, trigger],
  );
  check(
    "modified grid shortcut untouched",
    key(~ctrl=true, active_cell, "keydown", "ArrowLeft"),
  );
  focus(node("#test-end"));
  check(
    "end sentinel redirects to first control",
    has_focus("#test-calendar-previous-year"),
  );
  focus(node("#test-start"));
  check(
    "start sentinel redirects to last control",
    has_focus("#test-close"),
  );
  focus(node("#outside"));
  check(
    "programmatic focus cannot leave an open modal",
    has_focus(active_cell),
  );
  escape(active_cell);
  check("Escape restores Choose", has_focus(trigger));
  check("Escape cause", latest(app) == P.Closed(P.Escape));
  check("closed Escape unclaimed", key(trigger, "keydown", "Escape"));
  check(
    "instructions reset",
    get(node("#test-instructions"), "textContent") == "",
  );
  click(trigger);
  settle();
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  check("selection Enter claimed", !key(active_cell, "keydown", "Enter"));
  settle();
  check(
    "selection commits and closes",
    value(app) == Some("2026-03-06") && !P.is_open(app.model().picker),
  );
  check("selection cause", latest(app) == P.Closed(P.Selection));
  check("selection returns focus", has_focus(trigger));
  click(trigger);
  settle();
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  ignore(key(active_cell, "keydown", " "));
  settle();
  check("Space selection", value(app) == Some("2026-03-07"));
  edit("2026-04-13");
  enter();
  check(
    "typed valid changes value",
    value(app) == Some("2026-04-13")
    && P.invalid_reason(app.model().picker) == None,
  );
  let outs = app.model().outs;
  ignore(emit(input, "blur"));
  settle();
  check(
    "Enter plus blur does not duplicate commit",
    app.model().outs == outs,
  );
  click(trigger);
  settle();
  focus_date(app, "2026-04-13");
  escape(active_cell);
  edit("invalid raw text");
  enter();
  check(
    "invalid preserves raw text and committed date",
    P.text(app.model().picker) == "invalid raw text"
    && value(app) == Some("2026-04-13"),
  );
  check(
    "parse invalid reason",
    latest(app) == P.Invalid("invalid raw text", P.Syntax),
  );
  check(
    "invalid ARIA and presence",
    attr(input, "aria-invalid") == Some("true")
    && attr(input, "data-invalid") == Some("")
    && attr(input, "aria-errormessage") == Some("test-input-error"),
  );
  check("error is visible", !get(node("#test-input-error"), "hidden"));
  idrefs();
  edit("");
  ignore(emit(input, "blur"));
  settle();
  check(
    "empty clears",
    value(app) == None && latest(app) == P.Changed(None),
  );
  check(
    "clear removes invalid semantics",
    attr(input, "aria-invalid") == None
    && get(node("#test-input-error"), "hidden"),
  );
  click(trigger);
  settle();
  focus_date(app, "2026-03-05");
  escape(active_cell);
  edit("2026-06-17");
  app.dispatch(Open);
  settle();
  focus_date(app, "2026-06-17");
  check(
    "programmatic opening does not commit unblurred text",
    value(app) == None,
  );
  app.dispatch(Close);
  settle();
  app.dispatch(Set(Some(today)));
  settle();
  check(
    "silent setter formats text and selection",
    value(app) == Some("2026-03-05")
    && P.text(app.model().picker) == "2026-03-05",
  );
  let outs = app.model().outs;
  app.dispatch(Open);
  app.dispatch(Open);
  settle();
  app.dispatch(Close);
  app.dispatch(Close);
  settle();
  check("programmatic setters silent", app.model().outs == outs);
  click(trigger);
  settle();
  pointer("#outside");
  focus(node("#outside"));
  settle();
  check("outside reason", latest(app) == P.Closed(P.Outside));
  check("outside focus not stolen", has_focus("#outside"));
  click(trigger);
  settle();
  pointer(input);
  focus(node(input));
  settle();
  check(
    "text input is outside and keeps native pointer focus",
    latest(app) == P.Closed(P.Outside)
    && !P.is_open(app.model().picker)
    && has_focus(input),
  );
  click(trigger);
  settle();
  pointer("#nonfocusable");
  settle();
  check("nonfocusable outside does not focus trigger", !has_focus(trigger));
  check("outside close no restore", latest(app) == P.Closed(P.Outside));
  click(trigger);
  settle();
  click("#test-cancel");
  settle();
  check(
    "Cancel returns focus",
    has_focus(trigger) && latest(app) == P.Closed(P.Cancel),
  );
  click(trigger);
  settle();
  click(trigger);
  settle();
  check("expanded trigger cancels", latest(app) == P.Closed(P.Cancel));
  click(trigger);
  settle();
  click("#test-close");
  settle();
  check(
    "generic close reports Programmatic and returns focus",
    latest(app) == P.Closed(P.Programmatic) && has_focus(trigger),
  );
  /* Several edits before a render use latest model state, not view snapshots. */
  put(node(input), "value", "2026-01-01");
  ignore(dispatch(node(input), event("input", { "bubbles": true })));
  put(node(input), "value", "2026-02-02");
  ignore(dispatch(node(input), event("input", { "bubbles": true })));
  enter();
  check("batched edit intent", value(app) == Some("2026-02-02"));
  app.dispatch(Open);
  app.dispatch(Close);
  focus(node("#outside"));
  settle();
  check("closing cancels opening focus", has_focus("#outside"));
  app.dispatch(Open);
  settle();
  ignore(key(active_cell, "keydown", "Escape"));
  pointer("#nonfocusable");
  settle();
  check("outside cancels pending Escape focus", !has_focus(trigger));
  app.dispatch(Open);
  settle();
  app.dispatch(Remove);
  settle();
  check("unmount removes global subscriptions", listener_count() == 0);
  stop(app);

  let limits = {
    C.min: C.Date.of_iso("2026-03-02"),
    max: C.Date.of_iso("2026-03-27"),
    is_unavailable: date =>
      C.Date.day_of_week(date) == 0 || C.Date.day_of_week(date) == 6,
  };
  let app = start(~limits, ~selected=today, ());
  layout();
  List.iter(
    ((text, reason)) => {
      edit(text);
      enter();
      check(
        "distinct constraint failure",
        latest(app) == P.Invalid(text, reason),
      );
      check(
        "constraint invalid keeps old value",
        value(app) == Some("2026-03-05"),
      );
    },
    [
      ("2026-03-01", P.Before_min),
      ("2026-03-28", P.After_max),
      ("2026-03-07", P.Unavailable),
    ],
  );
  click(trigger);
  settle();
  focus_date(app, "2026-03-05");
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  check(
    "unavailable grid date remains focusable",
    attr(active_cell, "aria-disabled") == Some("true"),
  );
  ignore(key(active_cell, "keydown", "Enter"));
  settle();
  check(
    "unavailable selection rejected",
    P.is_open(app.model().picker) && value(app) == Some("2026-03-05"),
  );
  click(active_cell);
  settle();
  check("unavailable click rejected", P.is_open(app.model().picker));
  escape(active_cell);
  stop(app);

  let app = start(~selected=today, ~confirm=true, ());
  layout();
  click(trigger);
  settle();
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  ignore(key(active_cell, "keydown", "Enter"));
  settle();
  check(
    "confirm stages without committing",
    value(app) == Some("2026-03-05") && P.is_open(app.model().picker),
  );
  check(
    "staged date selected",
    attr(active_cell, "aria-selected") == Some("true"),
  );
  check(
    "hidden value stays committed",
    get(node("input[name=visit]"), "value") == "2026-03-05",
  );
  click("#test-cancel");
  settle();
  check("cancel discards staged date", value(app) == Some("2026-03-05"));
  click(trigger);
  settle();
  focus_date(app, "2026-03-05");
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  ignore(key(active_cell, "keydown", " "));
  settle();
  click("#test-confirm");
  settle();
  check(
    "OK commits staged date",
    value(app) == Some("2026-03-06")
    && latest(app) == P.Closed(P.Confirm)
    && has_focus(trigger),
  );
  click(trigger);
  settle();
  app.dispatch(
    Limits({
      ...C.unconstrained,
      is_unavailable: _date => true,
    }),
  );
  settle();
  check(
    "changed constraints disable OK",
    get(node("#test-confirm"), "disabled"),
  );
  click("#test-confirm");
  settle();
  check(
    "stale OK rejected",
    P.is_open(app.model().picker) && value(app) == Some("2026-03-06"),
  );
  escape(active_cell);
  stop(app);

  let app = start(~readonly=true, ~selected=today, ());
  layout();
  check(
    "readonly native input and presence",
    get(node(input), "readOnly")
    && attr(input, "data-readonly") == Some(""),
  );
  click(trigger);
  settle();
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  focus_date(app, "2026-03-06");
  ignore(key(active_cell, "keydown", "Enter"));
  ignore(key(active_cell, "keydown", " "));
  settle();
  check(
    "readonly browsing cannot change",
    value(app) == Some("2026-03-05") && P.is_open(app.model().picker),
  );
  check(
    "readonly grid semantics",
    attr("#test-calendar-grid", "aria-readonly") == Some("true"),
  );
  edit("2026-03-09");
  check(
    "readonly input Enter remains native",
    key(input, "keydown", "Enter"),
  );
  settle();
  check(
    "readonly synthetic editing rejected",
    value(app) == Some("2026-03-05"),
  );
  escape(active_cell);
  stop(app);

  let app = start(~disabled=true, ~selected=today, ());
  layout();
  check(
    "disabled form controls",
    get(node(input), "disabled")
    && get(node(trigger), "disabled")
    && get(node("input[name=visit]"), "disabled"),
  );
  click(trigger);
  app.dispatch(Open);
  settle();
  check("disabled cannot open", !P.is_open(app.model().picker));
  edit("2026-03-09");
  ignore(key(input, "keydown", "Enter"));
  settle();
  check(
    "disabled cannot edit",
    value(app) == Some("2026-03-05") && app.model().outs == [],
  );
  stop(app);

  let custom_format = date => {
    let iso = C.Date.to_iso(date);
    String.sub(iso, 8, 2)
    ++ "/"
    ++ String.sub(iso, 5, 2)
    ++ "/"
    ++ String.sub(iso, 0, 4);
  };
  let custom_parse = text =>
    String.length(text) == 10 && text.[2] == '/' && text.[5] == '/'
      ? C.Date.of_iso(
          String.sub(text, 6, 4)
          ++ "-"
          ++ String.sub(text, 3, 2)
          ++ "-"
          ++ String.sub(text, 0, 2),
        )
      : None;
  let app =
    start(
      ~selected=today,
      ~format=custom_format,
      ~parse=custom_parse,
      ~dir=C.Rtl,
      (),
    );
  layout();
  check("custom initial format", P.text(app.model().picker) == "05/03/2026");
  edit("17/04/2026");
  enter();
  check(
    "custom parse produces ISO form value",
    value(app) == Some("2026-04-17")
    && get(node("input[name=visit]"), "value") == "2026-04-17",
  );
  click(trigger);
  settle();
  ignore(key(active_cell, "keydown", "ArrowLeft"));
  settle();
  focus_date(app, "2026-04-18");
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  focus_date(app, "2026-04-17");
  click(active_cell);
  settle();
  check(
    "pointer selection formats custom text",
    P.text(app.model().picker) == "17/04/2026",
  );
  stop(app);

  let labels: P.labels = {
    choose: {js|Choisir une date|js},
    choose_selected: date =>
      {js|Choisir une date, sélection : |js} ++ C.Date.to_iso(date),
    confirm: {js|Confirmer|js},
    cancel: {js|Annuler|js},
    close: {js|Fermer|js},
    instructions: {js|Échap ferme le calendrier.|js},
    error: _ => {js|Date non valide : réessayez.|js},
  };
  let app =
    start(~labels, ~required=true, ~selected=today, ~confirm=true, ());
  layout();
  check("native required input", get(node(input), "required"));
  check(
    "localized Unicode selected name",
    attr(trigger, "aria-label")
    == Some({js|Choisir une date, sélection : 2026-03-05|js}),
  );
  edit("non");
  enter();
  check(
    "localized Unicode validation message",
    get(node("#test-input-error"), "textContent")
    == {js|Date non valide : réessayez.|js},
  );
  click(trigger);
  settle();
  check(
    "localized instructions and actions",
    get(node("#test-instructions"), "textContent") == labels.instructions
    && get(node("#test-confirm"), "textContent") == labels.confirm
    && get(node("#test-cancel"), "textContent") == labels.cancel
    && attr("#test-close", "aria-label") == Some(labels.close),
  );
  app.dispatch(Modes(false, true));
  settle();
  check(
    "current readonly disables OK",
    get(node("#test-confirm"), "disabled"),
  );
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  ignore(key(active_cell, "keydown", "Enter"));
  settle();
  check(
    "current readonly prevents selection",
    value(app) == Some("2026-03-05"),
  );
  app.dispatch(Modes(true, false));
  settle();
  let focus_before = P.focus_date(app.model().picker);
  ignore(key(active_cell, "keydown", "ArrowRight"));
  settle();
  check(
    "current disabled rejects grid intent",
    C.Date.equal(focus_before, P.focus_date(app.model().picker)),
  );
  check(
    "current disabled disables form value",
    get(node("input[name=visit]"), "disabled"),
  );
  escape(active_cell);
  stop(app);

  let app = start(~unmount=true, ());
  layout();
  check("closed unmounted popup", Js.testAny(node("#test-panel")));
  app.dispatch(Open);
  settle();
  check("open mounts popup", !Js.testAny(node("#test-panel")));
  app.dispatch(Close);
  settle();
  check("Left permits unmount", Js.testAny(node("#test-panel")));
  stop(app);
  let app = start();
  layout();
  let unfinished = Js.Promise.make((~resolve as _, ~reject as _) => ());
  put(node("#test-panel"), "getAnimations", _ =>
    [|{ "finished": unfinished }|]
  );
  app.dispatch(Open);
  settle();
  check(
    "active animation wait",
    P.phase(app.model().picker) == Ui.Animation.Entering,
  );
  stop(app);
  let app = start(~top_layer=true, ());
  layout();
  check(
    "native promotion contract",
    attr("#test-panel", "popover") == Some("manual"),
  );
  app.dispatch(Open);
  app.shutdown();
  settle();
  check("early shutdown cancels all starts", listener_count() == 0);
  report("date_picker");
  Js.Promise.resolve();
};
