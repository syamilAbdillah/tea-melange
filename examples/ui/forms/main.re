open Tea;
module S = Ui.Select;
type form_data;
type iterator;
external document: Dom.document = "document";
[@mel.send]
external by_id: (Dom.document, string) => Dom.node = "getElementById";
[@mel.new] external form_data: Dom.node => form_data = "FormData";
[@mel.send] external entries: form_data => iterator = "entries";
[@mel.scope "Array"]
external from: iterator => array((string, string)) = "from";
type msg =
  | Name(string)
  | Notes(string)
  | Changed
  | Blurred
  | Toggle_native
  | Toggle_custom
  | Toggle_switch
  | Selection(S.selection)
  | Multiple(S.selection)
  | Disabled
  | Invalid
  | Readonly
  | Mixed
  | Options(int)
  | Submit
  | Serialized(string);
type model = {
  name: string,
  notes: string,
  native: bool,
  native_mixed: bool,
  custom: bool,
  custom_mixed: bool,
  updates: bool,
  selection: S.selection,
  multiple: S.selection,
  disabled: bool,
  invalid: bool,
  readonly: bool,
  option_mode: int,
  changes: int,
  blurs: int,
  toggles: int,
  serialized: string,
};
let all_items = [|
  S.Option({
    key: "sencha",
    label: "Sencha",
    disabled: false,
  }),
  S.Option({
    key: "sold-out",
    label: "Sold out",
    disabled: true,
  }),
  S.Group({
    key: "oxidized",
    label: "Oxidized teas",
    disabled: false,
    items: [|
      {
        key: "oolong / roasted",
        label: "Roasted oolong",
        disabled: false,
      },
      {
        key: "assam",
        label: "Assam",
        disabled: false,
      },
    |],
  }),
|];
let items = mode =>
  switch (mode) {
  | 1 => [||]
  | 2 => [|
      S.Group({
        key: "disabled",
        label: "Unavailable",
        disabled: true,
        items: [|
          {
            key: "sencha",
            label: "Sencha",
            disabled: false,
          },
        |],
      }),
    |]
  | 3 => [|
      S.Option({
        key: "only",
        label: "Only tea",
        disabled: false,
      }),
    |]
  | 4 => [|all_items[2], all_items[0]|]
  | _ => all_items
  };
let update = (msg, m) =>
  switch (msg) {
  | Submit => (
      m,
      Cmd.effect(dispatch => {
        let data =
          from(entries(form_data(by_id(document, "tasting-form"))));
        dispatch(
          Serialized(
            Option.value(~default="[]", Js.Json.stringifyAny(data)),
          ),
        );
      }),
    )
  | _ => (
      switch (msg) {
      | Name(name) => {
          ...m,
          name,
        }
      | Notes(notes) => {
          ...m,
          notes,
        }
      | Changed => {
          ...m,
          changes: m.changes + 1,
        }
      | Blurred => {
          ...m,
          blurs: m.blurs + 1,
        }
      | Toggle_native => {
          ...m,
          native:
            Ui.Checkbox.next_checked(
              ~checked=m.native,
              ~indeterminate=m.native_mixed,
              (),
            ),
          native_mixed: false,
          toggles: m.toggles + 1,
        }
      | Toggle_custom => {
          ...m,
          custom:
            Ui.Checkbox.next_checked(
              ~checked=m.custom,
              ~indeterminate=m.custom_mixed,
              (),
            ),
          custom_mixed: false,
          toggles: m.toggles + 1,
        }
      | Toggle_switch => {
          ...m,
          updates: !m.updates,
          toggles: m.toggles + 1,
        }
      | Selection(selection) => {
          ...m,
          selection: S.normalize(~items=items(m.option_mode), selection),
        }
      | Multiple(multiple) => {
          ...m,
          multiple: S.normalize(~items=items(m.option_mode), multiple),
        }
      | Disabled => {
          ...m,
          disabled: !m.disabled,
        }
      | Invalid => {
          ...m,
          invalid: !m.invalid,
        }
      | Readonly => {
          ...m,
          readonly: !m.readonly,
        }
      | Mixed => {
          ...m,
          native_mixed: true,
          custom_mixed: true,
        }
      | Options(option_mode) => {
          ...m,
          option_mode,
          selection: S.normalize(~items=items(option_mode), m.selection),
          multiple: S.normalize(~items=items(option_mode), m.multiple),
        }
      | Serialized(serialized) => {
          ...m,
          serialized,
        }
      | Submit => m
      },
      Cmd.none,
    )
  };
let button = (id, msg, text) =>
  Html.button(
    [Html.id(id), Html.type_("button"), Html.on_click(msg)],
    [Html.text(text)],
  );
let select_field = (~id, ~label, ~multiple, ~disabled, m) =>
  S.view(
    ~id,
    ~items=items(m.option_mode),
    ~value=multiple ? m.multiple : m.selection,
    ~on_change=value => multiple ? Multiple(value) : Selection(value),
    ~placeholder="Choose a tea",
    ~name=multiple ? "extras" : "tea",
    ~required=!multiple,
    ~disabled,
    ~invalid=m.invalid,
    ~has_description=true,
    ~has_error=m.invalid,
    ~to_view=
      p =>
        Html.div(
          [Html.class_("field")],
          [
            Html.label(p.label, [Html.text(label)]),
            Html.keyed(
              "select",
              p.control @ (multiple ? [Html.attribute("size", "5")] : []),
              p.options,
            ),
            Html.p(
              p.description @ [Html.class_("help")],
              [
                Html.text(
                  multiple
                    ? "Choose any extra teas. Use your platform's multiple-selection keys."
                    : "Choose one tea for the tasting.",
                ),
              ],
            ),
            m.invalid
              ? Html.p(
                  p.error @ [Html.class_("error")],
                  [Html.text("Choose a tea before reserving.")],
                )
              : Html.text(""),
          ],
        ),
    (),
  );
let view = m =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Reserve a tea tasting")]),
      Html.p(
        [],
        [
          Html.text(
            "A native form, with explicit labels and parent-owned state. Try the keyboard, change the validation state, then inspect what the browser submits.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("tools")],
        [
          button(
            "disable",
            Disabled,
            m.disabled ? "Enable fieldset" : "Disable fieldset",
          ),
          button(
            "validate",
            Invalid,
            m.invalid ? "Clear errors" : "Show errors",
          ),
          button(
            "readonly",
            Readonly,
            m.readonly ? "Allow text editing" : "Make text read-only",
          ),
          button("mixed", Mixed, "Restore mixed checkboxes"),
        ],
      ),
      Html.div(
        [Html.class_("tools")],
        [
          button("options-full", Options(0), "All teas"),
          button("options-empty", Options(1), "No teas"),
          button("options-disabled", Options(2), "Unavailable teas"),
          button("options-single", Options(3), "One tea"),
          button("options-reordered", Options(4), "Reorder teas"),
        ],
      ),
      Html.form(
        [
          Html.id("tasting-form"),
          Html.attribute("novalidate", ""),
          Html.on_submit(Submit),
        ],
        [
          Ui.Fieldset.view(
            ~id="preferences",
            ~disabled=m.disabled,
            ~invalid=m.invalid,
            ~has_description=true,
            ~to_view=
              f =>
                Html.fieldset(
                  f.root,
                  [
                    Html.legend(
                      f.legend,
                      [Html.text("Tasting preferences")],
                    ),
                    Html.p(
                      f.description @ [Html.class_("help")],
                      [
                        Html.text(
                          "Disabling this fieldset excludes every preference from form submission, including custom controls.",
                        ),
                      ],
                    ),
                    Ui.Input.view(
                      ~id="name",
                      ~name="name",
                      ~type_="text",
                      ~autocomplete="name",
                      ~inputmode="text",
                      ~value=m.name,
                      ~on_input=value => Name(value),
                      ~on_change=_ => Changed,
                      ~on_blur=Blurred,
                      ~required=true,
                      ~disabled=f.disabled,
                      ~readonly=m.readonly,
                      ~invalid=m.invalid,
                      ~has_description=true,
                      ~has_error=m.invalid,
                      ~to_view=
                        p =>
                          Html.div(
                            [Html.class_("field")],
                            [
                              Html.label(p.label, [Html.text("Your name")]),
                              Html.input(p.control, []),
                              Html.p(
                                p.description @ [Html.class_("help")],
                                [
                                  Html.text(
                                    "Use the name we should put on your reservation.",
                                  ),
                                ],
                              ),
                              m.invalid
                                ? Html.p(
                                    p.error @ [Html.class_("error")],
                                    [
                                      Html.text(
                                        "Enter your name before reserving.",
                                      ),
                                    ],
                                  )
                                : Html.text(""),
                            ],
                          ),
                      (),
                    ),
                    Ui.Textarea.view(
                      ~id="notes",
                      ~name="notes",
                      ~value=m.notes,
                      ~on_input=value => Notes(value),
                      ~on_change=_ => Changed,
                      ~on_blur=Blurred,
                      ~rows=4,
                      ~disabled=f.disabled,
                      ~readonly=m.readonly,
                      ~invalid=m.invalid,
                      ~has_description=true,
                      ~has_error=m.invalid,
                      ~to_view=
                        p =>
                          Html.div(
                            [Html.class_("field")],
                            [
                              Html.label(
                                p.label,
                                [Html.text("Tasting notes")],
                              ),
                              Html.textarea(p.control, []),
                              Html.p(
                                p.description @ [Html.class_("help")],
                                [
                                  Html.text(
                                    "Tell us about dietary needs or a tea you would like to try.",
                                  ),
                                ],
                              ),
                              m.invalid
                                ? Html.p(
                                    p.error @ [Html.class_("error")],
                                    [
                                      Html.text("Review your tasting notes."),
                                    ],
                                  )
                                : Html.text(""),
                            ],
                          ),
                      (),
                    ),
                    Ui.Checkbox.view(
                      ~id="native",
                      ~checked=m.native,
                      ~indeterminate=m.native_mixed,
                      ~on_change=Toggle_native,
                      ~name="equipment",
                      ~value="provided",
                      ~disabled=f.disabled,
                      ~has_description=true,
                      ~to_view=
                        p =>
                          Html.div(
                            [Html.class_("field")],
                            [
                              Html.input(p.control, []),
                              Html.label(
                                p.label,
                                [Html.text("Include tasting equipment")],
                              ),
                              Html.p(
                                p.description @ [Html.class_("help")],
                                [
                                  Html.text(
                                    "Mixed becomes checked on activation. This is a native checkbox.",
                                  ),
                                ],
                              ),
                              p.hidden_input,
                            ],
                          ),
                      (),
                    ),
                    Ui.Checkbox.view(
                      ~id="custom",
                      ~native=false,
                      ~checked=m.custom,
                      ~indeterminate=m.custom_mixed,
                      ~on_change=Toggle_custom,
                      ~name="samples",
                      ~value="included",
                      ~disabled=f.disabled,
                      ~has_description=true,
                      ~to_view=
                        p =>
                          Html.div(
                            [Html.class_("field")],
                            [
                              Html.div(
                                p.control,
                                [
                                  Html.span(
                                    p.label,
                                    [Html.text("Include take-home samples")],
                                  ),
                                ],
                              ),
                              Html.p(
                                p.description @ [Html.class_("help")],
                                [
                                  Html.text(
                                    "A custom checkbox: Space toggles on release. Its nested label is clickable.",
                                  ),
                                ],
                              ),
                              p.hidden_input,
                            ],
                          ),
                      (),
                    ),
                    Ui.Switch_.view(
                      ~id="updates",
                      ~checked=m.updates,
                      ~on_change=Toggle_switch,
                      ~name="updates",
                      ~value="subscribed",
                      ~disabled=f.disabled,
                      ~has_description=true,
                      ~to_view=
                        p =>
                          Html.div(
                            [Html.class_("field")],
                            [
                              Html.label(
                                p.label,
                                [Html.text("Tasting updates")],
                              ),
                              Html.button(
                                p.root,
                                [Html.text(m.updates ? "On" : "Off")],
                              ),
                              Html.p(
                                p.description @ [Html.class_("help")],
                                [
                                  Html.text(
                                    "An ordinary button activation toggles the switch. The label stays constant.",
                                  ),
                                ],
                              ),
                              p.hidden_input,
                            ],
                          ),
                      (),
                    ),
                    select_field(
                      ~id="tea",
                      ~label="First tea",
                      ~multiple=false,
                      ~disabled=f.disabled,
                      m,
                    ),
                    select_field(
                      ~id="extras",
                      ~label="Extra teas",
                      ~multiple=true,
                      ~disabled=f.disabled,
                      m,
                    ),
                  ],
                ),
            (),
          ),
          Html.button(
            [Html.id("submit"), Html.type_("submit")],
            [Html.text("Print submitted form data")],
          ),
        ],
      ),
      Html.h2([], [Html.text("Browser submission")]),
      Html.pre(
        [Html.id("serialized"), Html.aria("live", "polite")],
        [Html.text(m.serialized)],
      ),
      Html.p(
        [Html.id("activity")],
        [
          Html.text(
            "Changes: "
            ++ string_of_int(m.changes)
            ++ ". Blurs: "
            ++ string_of_int(m.blurs)
            ++ ". Toggles: "
            ++ string_of_int(m.toggles)
            ++ ".",
          ),
        ],
      ),
    ],
  );
let () = {
  let app =
    App.element(
      ~init=
        () =>
          (
            {
              name: "",
              notes: "",
              native: false,
              native_mixed: true,
              custom: false,
              custom_mixed: true,
              updates: false,
              selection: S.Single(None),
              multiple: S.Multiple([|"sencha"|]),
              disabled: false,
              invalid: false,
              readonly: false,
              option_mode: 0,
              changes: 0,
              blurs: 0,
              toggles: 0,
              serialized: "No submission yet.",
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
  ignore(app);
};
