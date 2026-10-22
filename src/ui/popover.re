module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Events = Tea_browser.Browser_events;
type role =
  | Dialog(string)
  | Region
  | None;
type focus_on_open =
  | None
  | First_tabbable
  | Element(string);
type reason =
  | Escape
  | Outside
  | Focus_out
  | Trigger
  | Programmatic;
type out_msg =
  | Opened
  | Closed(reason);
type msg =
  | Toggle
  | Dismiss(reason)
  | Focus_finished(int)
  | Animation(Animation.msg)
  | Anchor(Anchor.msg);
type model = {
  id: string,
  role,
  focus_on_open,
  close_on_focus_out: bool,
  group_triggers: list(string),
  opened: bool,
  pending_focus: bool,
  focus_job: option(focus_on_open),
  focus_generation: int,
  restore_focus: bool,
  animation: Animation.model,
  anchor: Anchor.model,
};
let trigger_id = model => Ui_attr.part_id(model.id, "trigger");
let panel_id = model => Ui_attr.part_id(model.id, "panel");
let is_open = model => model.opened;
let should_render = model => Animation.should_render(model.animation);
let phase = model => Animation.phase(model.animation);
let position = model => Anchor.position(model.anchor);
let init =
    (
      ~id,
      ~role: role=None,
      ~focus_on_open: focus_on_open=None,
      ~close_on_focus_out=true,
      ~group_triggers=[],
      ~placement=(Anchor.Bottom, Anchor.Start),
      ~offset=8.,
      ~flip=true,
      ~shift_padding=Some(8.),
      ~match_width=false,
      ~dir=Anchor.Ltr,
      ~top_layer=true,
      ~arrow_size=?,
      ~anchor_id=?,
      (),
    ) => {
  id,
  role,
  focus_on_open,
  close_on_focus_out,
  group_triggers,
  opened: false,
  pending_focus: false,
  focus_job: None,
  focus_generation: 0,
  restore_focus: false,
  animation:
    Animation.init(~id=Ui_attr.part_id(id, "panel"), ~open_=false, ()),
  anchor:
    Anchor.init(
      ~anchor_id=
        Option.value(~default=Ui_attr.part_id(id, "trigger"), anchor_id),
      ~floating_id=Ui_attr.part_id(id, "panel"),
      ~placement,
      ~offset,
      ~flip,
      ~shift_padding,
      ~match_width,
      ~dir,
      ~top_layer,
      ~arrow_size?,
      (),
    ),
};
/* Browser_events.on_pointer_down_outside(~focus=true) would conflate pointer
   and focus departures and miss blur() to body, so focus has its own source. */
let focus_source = model =>
  Sub.make(
    ~home="Tea_ui.Popover.focus",
    ~key=model.id,
    ~start=
      dispatch => {
        let live = ref(true);
        let ids =
          [trigger_id(model), panel_id(model)] @ model.group_triggers;
        let check = () =>
          if (live^ && !Ui_dom.within_any(ids, Ui_dom.active())) {
            dispatch(Dismiss(Focus_out));
          };
        let on_in = event =>
          if (live^ && !Ui_dom.within_any(ids, Ui_dom.target(event))) {
            dispatch(Dismiss(Focus_out));
          };
        let on_out = event =>
          if (Option.is_none(Ui_dom.related(event))) {
            Tea_dom.Browser_platform.request_animation_frame(_ => check());
          };
        let off_in =
          Ui_dom.on(~capture=true, Ui_dom.document, "focusin", on_in);
        let off_out =
          Ui_dom.on(~capture=true, Ui_dom.document, "focusout", on_out);
        () => {
          live := false;
          off_in();
          off_out();
        };
      },
    ~tagger=message => message,
  );
let open_ = model =>
  if (model.opened) {
    (model, Cmd.none);
  } else {
    let (anchor, anchor_cmd) = Anchor.start(model.anchor);
    let (animation, animation_cmd) = Animation.show(model.animation);
    (
      {
        ...model,
        opened: true,
        pending_focus: true,
        focus_job: None,
        focus_generation: model.focus_generation + 1,
        restore_focus: false,
        anchor,
        animation,
      },
      Cmd.batch([
        Cmd.map(msg => Anchor(msg), anchor_cmd),
        Cmd.map(msg => Animation(msg), animation_cmd),
      ]),
    );
  };
let close = model =>
  if (!model.opened) {
    (model, Cmd.none);
  } else {
    let (animation, command) = Animation.hide(model.animation);
    (
      {
        ...model,
        opened: false,
        pending_focus: false,
        focus_job: None,
        focus_generation: model.focus_generation + 1,
        restore_focus: false,
        animation,
      },
      Cmd.map(msg => Animation(msg), command),
    );
  };
let update = (message, model) =>
  switch (message) {
  | Toggle when !model.opened =>
    let (model, command) = open_(model);
    (model, command, [Opened]);
  | Toggle =>
    let (model, command) = close(model);
    (model, command, [Closed(Trigger)]);
  | Dismiss(reason) when model.opened =>
    let (model, command) = close(model);
    let focus_job =
      switch (reason) {
      | Escape
      | Programmatic => Some(Element(trigger_id(model)))
      | Outside
      | Focus_out
      | Trigger => None
      };
    (
      {
        ...model,
        focus_job,
      },
      command,
      [Closed(reason)],
    );
  | Dismiss(Outside) => (
      {
        ...model,
        focus_job: None,
      },
      Cmd.none,
      [],
    )
  | Dismiss(_) => (model, Cmd.none, [])
  | Focus_finished(generation) when generation == model.focus_generation => (
      {
        ...model,
        focus_job: None,
        pending_focus: false,
        restore_focus: !model.opened && should_render(model),
      },
      Cmd.none,
      [],
    )
  | Focus_finished(_) => (model, Cmd.none, [])
  | Anchor(message) =>
    let (anchor, command, _) = Anchor.update(message, model.anchor);
    let ready =
      model.opened
      && model.pending_focus
      && (
        switch (Anchor.position(anchor)) {
        | Some(position) => !position.reference_hidden
        | None => false
        }
      );
    let focus_job: option(focus_on_open) =
      ready
        ? switch (model.focus_on_open) {
          | None => None
          | policy => Some(policy)
          }
        : model.focus_job;
    (
      {
        ...model,
        anchor,
        pending_focus: model.pending_focus && !ready,
        focus_job,
      },
      Cmd.map(msg => Anchor(msg), command),
      [],
    );
  | Animation(message) =>
    let (animation, command, outs) =
      Animation.update(message, model.animation);
    let (anchor, anchor_cmd) =
      List.exists(
        out =>
          switch (out) {
          | Animation.Left => true
          | Animation.Entered => false
          },
        outs,
      )
        ? Anchor.stop(model.anchor) : (model.anchor, Cmd.none);
    (
      {
        ...model,
        animation,
        anchor,
      },
      Cmd.batch([
        Cmd.map(msg => Animation(msg), command),
        Cmd.map(msg => Anchor(msg), anchor_cmd),
      ]),
      [],
    );
  };
let focus_effect = (model, policy) => {
  let target: option(Events.focus_target) =
    switch ((policy: focus_on_open)) {
    | None => None
    | Element(id) => Some(Events.Element(id))
    | First_tabbable => Some(Events.First_tabbable(panel_id(model)))
    };
  Ui_focus.sub(
    ~home="Popover",
    ~id=model.id,
    ~ready=!model.pending_focus,
    ~from=
      model.opened || policy == First_tabbable
        ? [] : [trigger_id(model), panel_id(model)],
    {
      generation: model.focus_generation,
      target,
    },
    (generation, _) =>
    Focus_finished(generation)
  );
};
let subscriptions = model =>
  Sub.batch([
    focus_effect(
      model,
      model.pending_focus
        ? model.focus_on_open : Option.value(~default=None, model.focus_job),
    ),
    Sub.map(
      msg => Animation(msg),
      Animation.subscriptions(model.animation),
    ),
    Sub.map(msg => Anchor(msg), Anchor.subscriptions(model.anchor)),
    model.opened || Option.is_some(model.focus_job)
      ? Events.on_pointer_down_outside(
          ~ids=[trigger_id(model), panel_id(model)],
          Dismiss(Outside),
        )
      : Sub.none,
    model.opened && model.close_on_focus_out ? focus_source(model) : Sub.none,
    /* Only restoring dismissals acquire this lease, after the trigger is focused.
       A lease acquired on open cannot be cancelled for an outside dismissal and
       would steal focus after a click on nonfocusable content. */
    model.restore_focus && should_render(model)
      ? Events.restore_focus(~key=model.id, ~within=panel_id(model), ())
      : Sub.none,
  ]);
type parts('parent) = {
  trigger: list(Html.attr('parent)),
  panel: list(Html.attr('parent)),
  close_button: list(Html.attr('parent)),
  open_: bool,
  rendered: bool,
  position: option(Anchor.position),
};
let view =
    (
      ~model,
      ~to_parent,
      ~on_trigger_key_down=_: option('parent) => None,
      ~on_panel_key_down=_: option('parent) => None,
      ~to_view,
      (),
    ) => {
  let key = (~native, hook) =>
    Html.on(
      "keydown",
      V.Custom_optional(
        /* A handled Escape belongs to this popover alone: stop it so an
           enclosing Dialog or other overlay does not also dismiss. */
        D.map(
          handled =>
            {
              ...handled,
              V.stop_propagation: handled.V.prevent_default,
            },
          Ui_keyboard.custom(event =>
            if (!Ui_keyboard.plain(event)) {
              None;
            } else if (String.equal(event.key, "Escape")) {
              /* Dispatch intent even before an opening frame has drawn.
                 update, not this render snapshot, decides whether to close. */
              Some((
                Some(to_parent(Dismiss(Escape))),
                model.opened,
              ));
            } else if (String.equal(event.key, "Tab")
                       || native
                       && List.mem(event.key, ["Enter", " "])) {
              None;
            } else {
              Option.map(
                message => (Some(message), false),
                hook(event.key),
              );
            }
          ),
        ),
      ),
    );
  let trigger =
    Button.parts(~id=trigger_id(model), ~on_press=Toggle, ()).root;
  /* Button owns keydown; replace that fact, retaining its native activation
     and keyup semantics. Native Enter/Space remain entirely browser-owned. */
  let trigger =
    Ui_attr.without("keydown", trigger)
    @ [
      Ui_attr.aria_bool("expanded", model.opened),
      should_render(model)
        ? Html.aria("controls", panel_id(model)) : Html.none,
      switch (model.role) {
      | Dialog(_) => Html.aria("haspopup", "dialog")
      | Region
      | None => Html.none
      },
      Ui_attr.data_flag("open", model.opened),
    ];
  let anchor_attrs = Ui_attr.without("hidden", Anchor.attrs(model.anchor));
  let panel =
    anchor_attrs
    @ Animation.attrs(model.animation)
    @ [
      Ui_attr.data_flag("open", model.opened),
      switch (model.role) {
      | Dialog(_) => Html.role("dialog")
      | Region => Html.role("region")
      | None => Html.none
      },
      switch (model.role) {
      | Dialog(label) => Html.aria("label", label)
      | Region => Html.aria("labelledby", trigger_id(model))
      | None => Html.none
      },
    ];
  let close_button =
    Button.parts(
      ~id=Ui_attr.part_id(model.id, "close"),
      ~on_press=Dismiss(Programmatic),
      (),
    ).
      root;
  let map = List.map(V.map_fact(to_parent));
  to_view({
    trigger: map(trigger) @ [key(~native=true, on_trigger_key_down)],
    panel: map(panel) @ [key(~native=false, on_panel_key_down)],
    close_button: map(close_button),
    open_: model.opened,
    rendered: should_render(model),
    position: position(model),
  });
};
