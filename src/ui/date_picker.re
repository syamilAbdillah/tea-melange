module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module D = Tea_core.Decode;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
module Date = Calendar.Date;
type date = Calendar.date;
type invalid_reason =
  | Syntax
  | Before_min
  | After_max
  | Unavailable;
type reason =
  | Escape
  | Outside
  | Selection
  | Confirm
  | Cancel
  | Programmatic;
type out_msg =
  | Changed(option(date))
  | Invalid(string, invalid_reason)
  | Opened
  | Closed(reason);
type labels = {
  choose: string,
  choose_selected: date => string,
  confirm: string,
  cancel: string,
  close: string,
  instructions: string,
  error: invalid_reason => string,
};
let english = {
  choose: "Choose date",
  choose_selected: date =>
    "Choose date, selected date is "
    ++ Calendar.english.month(Date.month(date))
    ++ " "
    ++ string_of_int(Date.day(date))
    ++ ", "
    ++ string_of_int(Date.year(date)),
  confirm: "OK",
  cancel: "Cancel",
  close: "Close",
  instructions: "Cursor keys can navigate dates. Home and End move to the first and last day of the week. Page Up and Page Down change month; add Shift to change year. Enter or Space selects. Escape closes.",
  error: reason =>
    switch (reason) {
    | Syntax => "Enter a valid date."
    | Before_min => "The date is before the allowed range."
    | After_max => "The date is after the allowed range."
    | Unavailable => "This date is unavailable."
    },
};
type msg =
  | Edit(string)
  | Commit
  | Toggle
  | Dismiss(reason)
  | Accept
  | Calendar(Calendar.msg)
  | Popover(Popover.msg)
  | Focus_finished(int, bool);
type model = {
  id: string,
  value: option(date),
  text: string,
  invalid: option(invalid_reason),
  dirty: bool,
  fallback: date,
  confirm: bool,
  format: date => string,
  parse: string => option(date),
  calendar: Calendar.model,
  popover: Popover.model,
  initial_focus: bool,
  return_focus: bool,
  instructions_ready: bool,
  generation: int,
};
let part = (model, name) => Ui_attr.part_id(model.id, name);
let value = model => model.value;
let text = model => model.text;
let invalid_reason = model => model.invalid;
let is_open = model => Popover.is_open(model.popover);
let should_render = model => Popover.should_render(model.popover);
let phase = model => Popover.phase(model.popover);
let focus_date = model => Calendar.focus_date(model.calendar);
let init =
    (
      ~id,
      ~focus,
      ~selected=?,
      ~confirm=false,
      ~format=Date.to_iso,
      ~parse=Date.of_iso,
      ~first_day_of_week=0,
      ~dir=Calendar.Ltr,
      ~fixed_weeks=true,
      ~show_outside_month=false,
      ~placement=(Anchor.Bottom, Anchor.Start),
      ~top_layer=true,
      (),
    ) => {
  id,
  value: selected,
  text: Option.fold(~none="", ~some=format, selected),
  invalid: None,
  dirty: false,
  fallback: focus,
  confirm,
  format,
  parse,
  calendar:
    Calendar.init(
      ~id=Ui_attr.part_id(id, "calendar"),
      ~focus=Option.value(~default=focus, selected),
      ~selected?,
      ~first_day_of_week,
      ~dir,
      ~fixed_weeks,
      ~show_outside_month,
      (),
    ),
  popover:
    Popover.init(
      ~id,
      ~anchor_id=Ui_attr.part_id(id, "group"),
      ~close_on_focus_out=false,
      ~placement,
      ~dir=
        switch (dir) {
        | Calendar.Ltr => Anchor.Ltr
        | Calendar.Rtl => Anchor.Rtl
        },
      ~top_layer,
      (),
    ),
  initial_focus: false,
  return_focus: false,
  instructions_ready: false,
  generation: 0,
};
let validate = (constraints: Calendar.constraints, date) =>
  if (Option.fold(
        ~none=false,
        ~some=min => Date.compare(date, min) < 0,
        constraints.min,
      )) {
    Some(Before_min);
  } else if (Option.fold(
               ~none=false,
               ~some=max => Date.compare(date, max) > 0,
               constraints.max,
             )) {
    Some(After_max);
  } else if (constraints.is_unavailable(date)) {
    Some(Unavailable);
  } else {
    None;
  };
let set_value = (value, model) => {
  ...model,
  value,
  text: Option.fold(~none="", ~some=model.format, value),
  dirty: false,
  invalid: None,
  calendar: Calendar.set_selected(value, model.calendar),
};
let open_ = (~disabled=false, ~constraints=Calendar.unconstrained, model) =>
  if (disabled || is_open(model)) {
    (model, Cmd.none);
  } else {
    let typed =
      Option.bind(model.parse(model.text), date =>
        Option.is_none(validate(constraints, date)) ? Some(date) : None
      );
    let focus =
      Option.value(
        ~default=Option.value(~default=model.fallback, model.value),
        typed,
      );
    let calendar =
      Calendar.set_focus(
        focus,
        Calendar.set_selected(model.value, model.calendar),
      );
    let (popover, command) = Popover.open_(model.popover);
    (
      {
        ...model,
        calendar,
        popover,
        initial_focus: true,
        return_focus: false,
        instructions_ready: false,
        generation: model.generation + 1,
      },
      Cmd.map(message => Popover(message), command),
    );
  };
let close = model => {
  let (popover, command) = Popover.close(model.popover);
  (
    {
      ...model,
      popover,
      initial_focus: false,
      return_focus: false,
      instructions_ready: false,
      generation: model.generation + 1,
      calendar: Calendar.set_selected(model.value, model.calendar),
    },
    Cmd.map(message => Popover(message), command),
  );
};
let dismiss = (reason, model) =>
  if (!is_open(model)) {
    (
      {
        ...model,
        return_focus: reason == Outside ? false : model.return_focus,
      },
      Cmd.none,
      [],
    );
  } else {
    let (model, command) = close(model);
    (
      {
        ...model,
        return_focus: reason != Outside,
      },
      command,
      [Closed(reason)],
    );
  };
let commit = (~constraints, ~disabled, ~readonly, model) =>
  if (disabled || readonly || !model.dirty) {
    (model, []);
  } else {
    let parsed =
      String.equal(model.text, "")
        ? Ok(None)
        : (
          switch (model.parse(model.text)) {
          | None => Error(Syntax)
          | Some(date) =>
            switch (validate(constraints, date)) {
            | Some(reason) => Error(reason)
            | None => Ok(Some(date))
            }
          }
        );
    switch (parsed) {
    | Error(reason) => (
        {
          ...model,
          invalid: Some(reason),
          dirty: false,
        },
        [Invalid(model.text, reason)],
      )
    | Ok(value) => (set_value(value, model), [Changed(value)])
    };
  };
let update =
    (
      ~constraints=Calendar.unconstrained,
      ~disabled=false,
      ~readonly=false,
      message,
      model,
    ) =>
  switch (message) {
  | Edit(text) when !disabled && !readonly => (
      {
        ...model,
        text,
        dirty: true,
      },
      Cmd.none,
      [],
    )
  | Edit(_) => (model, Cmd.none, [])
  | Commit =>
    let (model, outs) = commit(~constraints, ~disabled, ~readonly, model);
    (model, Cmd.none, outs);
  | Toggle when disabled => (model, Cmd.none, [])
  | Toggle when is_open(model) => dismiss(Cancel, model)
  | Toggle =>
    let (model, command) = open_(~constraints, model);
    (model, command, [Opened]);
  | Dismiss(reason) => dismiss(reason, model)
  | Accept when is_open(model) && model.confirm && !disabled && !readonly =>
    let candidate = Calendar.selected(model.calendar);
    switch (Option.bind(candidate, date => validate(constraints, date))) {
    | Some(_) => (model, Cmd.none, [])
    | None =>
      let model = set_value(candidate, model);
      let (model, command, outs) = dismiss(Confirm, model);
      (model, command, [Changed(candidate), ...outs]);
    };
  | Accept => (model, Cmd.none, [])
  | Calendar(message) when is_open(model) && !disabled =>
    let (calendar, command, outs) =
      Calendar.update(
        ~constraints,
        ~readonly=readonly || disabled,
        message,
        model.calendar,
      );
    let model = {
      ...model,
      calendar,
    };
    let (model, closing, notifications) =
      List.fold_left(
        ((model, closing, notifications), out) =>
          switch (out) {
          | Calendar.Changed(date) when !model.confirm =>
            let model = set_value(Some(date), model);
            let (model, closing, outs) = dismiss(Selection, model);
            (model, closing, [Changed(Some(date)), ...outs]);
          | _ => (model, closing, notifications)
          },
        (model, Cmd.none, []),
        outs,
      );
    (
      model,
      Cmd.batch([Cmd.map(message => Calendar(message), command), closing]),
      notifications,
    );
  | Calendar(_) => (model, Cmd.none, [])
  | Popover(message) =>
    let (popover, command, outs) = Popover.update(message, model.popover);
    let model = {
      ...model,
      popover,
    };
    let (model, notifications) =
      List.fold_left(
        ((model, notifications), out) =>
          switch (out) {
          | Popover.Closed(cause) =>
            let reason =
              switch (cause) {
              | Popover.Escape => Escape
              | Popover.Outside
              | Popover.Focus_out => Outside
              | Popover.Trigger => Cancel
              | Popover.Programmatic => Programmatic
              };
            (
              {
                ...model,
                initial_focus: false,
                return_focus: false,
                instructions_ready: false,
                generation: model.generation + 1,
                calendar: Calendar.set_selected(model.value, model.calendar),
              },
              notifications @ [Closed(reason)],
            );
          | Popover.Opened => (model, notifications @ [Opened])
          },
        (model, []),
        outs,
      );
    (model, Cmd.map(message => Popover(message), command), notifications);
  | Focus_finished(generation, focused) when generation == model.generation =>
    if (model.initial_focus && !focused && is_open(model)) {
      /* A user who left during opening must not acquire a modal focus boundary
         when positioning catches up. Closing also removes modal semantics. */
      let (model, command) = close(model);
      (model, command, [Closed(Outside)]);
    } else {
      (
        {
          ...model,
          initial_focus: false,
          return_focus: false,
          instructions_ready: is_open(model) && focused,
        },
        Cmd.none,
        [],
      );
    }
  | Focus_finished(_, _) => (model, Cmd.none, [])
  };
/* The modal boundary sentinels wrap focus synchronously; deferred opening
   and return focus go through Browser_events.focus. */
let is_boundary = node => Ui_dom.matches(node, "[data-tea-date-boundary]");
let grid_focus = panel =>
  Option.iter(
    Ui_dom.focus,
    Browser.first_tabbable(
      ~exclude=node => !Ui_dom.matches(node, "[role=gridcell]"),
      panel,
    ),
  );
let ready = model =>
  is_open(model)
  && Option.fold(
       ~none=false,
       ~some=position => !position.Anchor.reference_hidden,
       Popover.position(model.popover),
     );
let focus_effect = model =>
  Events.focus(
    ~key=
      "Date_picker:"
      ++ Ui_sub.prefix(model.id)
      ++ ":"
      ++ string_of_int(model.generation),
    ~target=
      Events.Element(
        model.initial_focus
          ? Ui_attr.item_id(
              part(model, "calendar"),
              Date.to_iso(focus_date(model)),
            )
          : Popover.trigger_id(model.popover),
      ),
    ~ready=model.return_focus || ready(model),
    ~from=model.initial_focus ? [] : [Popover.panel_id(model.popover)],
    ~prevent_scroll=false,
    focused =>
    Focus_finished(model.generation, focused)
  );
let boundary_source = model =>
  Sub.make(
    ~home="Tea_ui.Date_picker.boundary",
    ~key=model.id,
    ~start=
      _dispatch => {
        let live = ref(true);
        let on_focus = event =>
          if (live^) {
            let target = Ui_dom.target(event);
            switch (Ui_dom.find(Popover.panel_id(model.popover)), target) {
            | (Some(panel), Some(node)) =>
              let start = Ui_dom.within(part(model, "start"), target);
              let end_ = Ui_dom.within(part(model, "end"), target);
              if (start || end_) {
                Option.iter(
                  Ui_dom.focus,
                  (start ? Browser.last_tabbable : Browser.first_tabbable)(
                    ~exclude=is_boundary,
                    panel,
                  ),
                );
              } else if (!Ui_dom.contains(panel, node)) {
                grid_focus(panel);
              };
            | _ => ()
            };
          };
        let off =
          Ui_dom.on(~capture=true, Ui_dom.document, "focusin", on_focus);
        () => {
          live := false;
          off();
        };
      },
    ~tagger=message => message,
  );
let subscriptions = model =>
  Sub.batch([
    is_open(model)
      ? Sub.map(
          message => Calendar(message),
          Calendar.subscriptions(model.calendar),
        )
      : Sub.none,
    Sub.map(
      message => Popover(message),
      Popover.subscriptions(model.popover),
    ),
    model.return_focus || model.initial_focus && is_open(model)
      ? focus_effect(model) : Sub.none,
    ready(model) && model.instructions_ready
      ? boundary_source(model) : Sub.none,
    model.return_focus
      ? Events.on_pointer_down_outside(
          ~ids=[
            Popover.trigger_id(model.popover),
            Popover.panel_id(model.popover),
          ],
          Dismiss(Outside),
        )
      : Sub.none,
  ]);
type parts('parent) = {
  group: list(Html.attr('parent)),
  label: list(Html.attr('parent)),
  input: list(Html.attr('parent)),
  trigger: list(Html.attr('parent)),
  description: list(Html.attr('parent)),
  error: list(Html.attr('parent)),
  error_text: string,
  hidden_input: list(Html.attr('parent)),
  dialog: list(Html.attr('parent)),
  start_boundary: list(Html.attr('parent)),
  end_boundary: list(Html.attr('parent)),
  instructions: list(Html.attr('parent)),
  instructions_text: string,
  calendar: Calendar.parts('parent),
  confirm_button: list(Html.attr('parent)),
  cancel_button: list(Html.attr('parent)),
  close_button: list(Html.attr('parent)),
  confirm: bool,
  open_: bool,
  rendered: bool,
  value: option(date),
  text: string,
};
let view =
    (
      ~model,
      ~constraints=Calendar.unconstrained,
      ~disabled=false,
      ~readonly=false,
      ~required=false,
      ~has_description=false,
      ~name=?,
      ~today=?,
      ~labels=english,
      ~calendar_labels=Calendar.english,
      ~to_parent,
      ~to_view,
      (),
    ) => {
  let invalid = Option.is_some(model.invalid);
  let input =
    Input.parts(
      ~id=part(model, "input"),
      ~value=model.text,
      ~on_input=text => to_parent(Edit(text)),
      ~on_blur=to_parent(Commit),
      ~has_description,
      ~has_error=invalid,
      ~invalid,
      ~disabled,
      ~readonly,
      ~required,
      (),
    );
  let input_keys =
    Html.on(
      "keydown",
      V.Custom_optional(
        Ui_keyboard.custom(event =>
          Ui_keyboard.plain(event)
          && String.equal(event.key, "Enter")
          && !disabled
          && !readonly
            ? Some((Some(to_parent(Commit)), true)) : None
        ),
      ),
    );
  Popover.view(
    ~model=model.popover,
    ~to_parent=message => to_parent(Popover(message)),
    ~to_view=
      popover =>
        Calendar.view(
          ~model=model.calendar,
          ~constraints,
          ~readonly=readonly || disabled,
          ~today?,
          ~labels=calendar_labels,
          ~to_parent=message => to_parent(Calendar(message)),
          ~to_view=
            calendar => {
              let trigger =
                Button.parts(
                  ~id=Popover.trigger_id(model.popover),
                  ~disabled,
                  ~on_press=to_parent(Toggle),
                  (),
                );
              let trigger_key =
                Html.on(
                  "keydown",
                  V.Custom_optional(
                    D.map(
                      handled =>
                        {
                          ...handled,
                          V.stop_propagation: handled.V.prevent_default,
                        },
                      Ui_keyboard.custom(event =>
                        Ui_keyboard.plain(event)
                        && String.equal(event.key, "Escape")
                        && is_open(model)
                          ? Some((Some(to_parent(Dismiss(Escape))), true))
                          : None
                      ),
                    ),
                  ),
                );
              let trigger_attrs = Ui_attr.without("keydown", trigger.root);
              let button = (name, message, disabled) =>
                Button.parts(
                  ~id=part(model, name),
                  ~disabled,
                  ~on_press=to_parent(message),
                  (),
                ).
                  root;
              let boundary = name => [
                Html.id(part(model, name)),
                Html.tabindex(0),
                Html.aria("hidden", "true"),
                Html.data("tea-date-boundary", ""),
              ];
              to_view({
                group: [Html.id(part(model, "group"))],
                label: input.label,
                input: input.control @ [input_keys],
                trigger:
                  trigger_attrs
                  @ [
                    trigger_key,
                    Html.aria("haspopup", "dialog"),
                    Ui_attr.aria_bool("expanded", is_open(model)),
                    should_render(model)
                      ? Html.aria(
                          "controls",
                          Popover.panel_id(model.popover),
                        )
                      : Html.none,
                    Html.aria(
                      "label",
                      Option.fold(
                        ~none=labels.choose,
                        ~some=labels.choose_selected,
                        model.value,
                      ),
                    ),
                    Ui_attr.data_flag("open", is_open(model)),
                    Ui_attr.data_flag("readonly", readonly),
                  ],
                description: input.description,
                error: input.error,
                error_text:
                  Option.fold(~none="", ~some=labels.error, model.invalid),
                hidden_input: [
                  Html.type_("hidden"),
                  Option.fold(~none=Html.none, ~some=Html.name, name),
                  Html.value(
                    Option.fold(~none="", ~some=Date.to_iso, model.value),
                  ),
                  Html.disabled(disabled),
                ],
                dialog:
                  popover.panel
                  @ [
                    Html.role("dialog"),
                    Html.aria("modal", "true"),
                    Html.aria("labelledby", part(model, "calendar-heading")),
                  ],
                start_boundary: boundary("start"),
                end_boundary: boundary("end"),
                instructions: [
                  Html.id(part(model, "instructions")),
                  Html.aria("live", "polite"),
                  Html.aria("atomic", "true"),
                ],
                instructions_text:
                  model.instructions_ready ? labels.instructions : "",
                calendar,
                confirm_button:
                  button(
                    "confirm",
                    Accept,
                    disabled
                    || readonly
                    || Option.fold(
                         ~none=false,
                         ~some=
                           date =>
                             Option.is_some(validate(constraints, date)),
                         Calendar.selected(model.calendar),
                       ),
                  ),
                cancel_button: button("cancel", Dismiss(Cancel), false),
                close_button:
                  button("close", Dismiss(Programmatic), false)
                  @ [Html.aria("label", labels.close)],
                confirm: model.confirm,
                open_: is_open(model),
                rendered: should_render(model),
                value: model.value,
                text: model.text,
              });
            },
          (),
        ),
    (),
  );
};
