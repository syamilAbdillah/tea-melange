module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Events = Tea_browser.Browser_events;
type mode =
  | Description
  | Label;
type out_msg =
  | Opened
  | Closed(float);
type msg =
  | Modality(bool)
  | Focus
  | Blur(float)
  | Escape(float)
  | Hover(Hover_intent.msg)
  | Animation(Animation.msg)
  | Anchor(Anchor.msg);
type model = {
  id: string,
  mode,
  disabled: bool,
  keyboard: bool,
  focus_open: bool,
  opened: bool,
  hover: Hover_intent.model,
  animation: Animation.model,
  anchor: Anchor.model,
};
let trigger_id = model => Ui_attr.part_id(model.id, "trigger");
let tooltip_id = model => Ui_attr.part_id(model.id, "tooltip");
let is_open = model => model.opened;
let is_visible = model => Animation.is_visible(model.animation);
let phase = model => Animation.phase(model.animation);
let position = model => Anchor.position(model.anchor);
let disabled = model => model.disabled;
let init =
    (
      ~id,
      ~mode=Description,
      ~disabled=false,
      ~open_delay_ms=600,
      ~close_delay_ms=150,
      ~placement=(Anchor.Top, Anchor.Center),
      ~offset=8.,
      ~flip=true,
      ~shift_padding=Some(8.),
      ~dir=Anchor.Ltr,
      ~top_layer=true,
      ~arrow_size=?,
      (),
    ) => {
  id,
  mode,
  disabled,
  keyboard: true,
  focus_open: false,
  opened: false,
  hover: Hover_intent.init(~open_delay_ms, ~close_delay_ms, ()),
  animation:
    Animation.init(~id=Ui_attr.part_id(id, "tooltip"), ~open_=false, ()),
  anchor:
    Anchor.init(
      ~anchor_id=Ui_attr.part_id(id, "trigger"),
      ~floating_id=Ui_attr.part_id(id, "tooltip"),
      ~placement,
      ~offset,
      ~flip,
      ~shift_padding,
      ~dir,
      ~top_layer,
      ~arrow_size?,
      (),
    ),
};
let open_ = model =>
  if (model.disabled || model.opened) {
    (model, Cmd.none);
  } else {
    let (anchor, a) = Anchor.start(model.anchor);
    let (animation, b) = Animation.show(model.animation);
    (
      {
        ...model,
        opened: true,
        anchor,
        animation,
      },
      Cmd.batch([
        Cmd.map(x => Anchor(x), a),
        Cmd.map(x => Animation(x), b),
      ]),
    );
  };
let hide = model => {
  let (animation, command) = Animation.hide(model.animation);
  (
    {
      ...model,
      opened: false,
      animation,
    },
    Cmd.map(x => Animation(x), command),
  );
};
let close = model => {
  let (hover, command) = Hover_intent.close_now(model.hover);
  let (model, closing) =
    hide({
      ...model,
      focus_open: false,
      hover,
    });
  (model, Cmd.batch([Cmd.map(x => Hover(x), command), closing]));
};
let set_disabled = (disabled, model) => {
  let model = {
    ...model,
    disabled,
  };
  disabled ? close(model) : (model, Cmd.none);
};
let dismiss = (time_stamp, model) => {
  let was_open = model.opened;
  let (model, command) = close(model);
  (model, command, was_open ? [Closed(time_stamp)] : []);
};
let update = (~group=?, message, model) =>
  switch (message) {
  | Modality(keyboard) => (
      {
        ...model,
        keyboard,
      },
      Cmd.none,
      [],
    )
  | Focus when model.keyboard && !model.disabled =>
    let was_open = model.opened;
    let (model, command) =
      open_({
        ...model,
        focus_open: true,
      });
    (model, command, was_open ? [] : [Opened]);
  | Focus => (model, Cmd.none, [])
  | Blur(time_stamp)
  | Escape(time_stamp) => dismiss(time_stamp, model)
  | Hover(message) =>
    let (hover, command, outs) =
      Hover_intent.update(
        ~disabled=model.disabled,
        ~group?,
        message,
        model.hover,
      );
    let model = {
      ...model,
      hover,
    };
    let (model, transition, notifications) =
      if (List.mem(Hover_intent.Intent_open, outs)) {
        let was_open = model.opened;
        let (model, transition) = open_(model);
        (model, transition, was_open || !model.opened ? [] : [Opened]);
      } else if (List.mem(Hover_intent.Intent_close, outs)
                 && !model.focus_open) {
        let was_open = model.opened;
        let (model, transition) = hide(model);
        let notifications =
          switch (Hover_intent.last_close_time(hover)) {
          | Some(time_stamp) when was_open => [Closed(time_stamp)]
          | _ => []
          };
        (model, transition, notifications);
      } else {
        (model, Cmd.none, []);
      };
    (
      model,
      Cmd.batch([Cmd.map(x => Hover(x), command), transition]),
      notifications,
    );
  | Anchor(message) =>
    let (anchor, command, _) = Anchor.update(message, model.anchor);
    (
      {
        ...model,
        anchor,
      },
      Cmd.map(x => Anchor(x), command),
      [],
    );
  | Animation(message) =>
    let (animation, command, outs) =
      Animation.update(message, model.animation);
    let (anchor, stopped) =
      List.mem(Animation.Left, outs)
        ? Anchor.stop(model.anchor) : (model.anchor, Cmd.none);
    (
      {
        ...model,
        animation,
        anchor,
      },
      Cmd.batch([
        Cmd.map(x => Animation(x), command),
        Cmd.map(x => Anchor(x), stopped),
      ]),
      [],
    );
  };
/* tea.browser has no general pointer-modality source or claimed global Escape
   source. These bindings are private and their listeners are subscription-owned.
   No mutable values enter the model. Keyboard decoding uses Ui_keyboard. */
module Native = {
  external document: Dom.document = "document";
  [@mel.send]
  external listen: (Dom.document, string, Js.Json.t => unit, bool) => unit =
    "addEventListener";
  [@mel.send]
  external unlisten: (Dom.document, string, Js.Json.t => unit, bool) => unit =
    "removeEventListener";
  [@mel.send]
  external by_id: (Dom.document, string) => Js.Nullable.t(Dom.node) =
    "getElementById";
  [@mel.get] external target: Js.Json.t => Js.Nullable.t(Dom.node) = "target";
  [@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
  [@mel.get] external prevented: Js.Json.t => bool = "defaultPrevented";
  [@mel.send] external prevent: Js.Json.t => unit = "preventDefault";
  [@mel.send] external stop: Js.Json.t => unit = "stopPropagation";
  let within = (id, event) =>
    switch (
      Js.Nullable.toOption(by_id(document, id)),
      Js.Nullable.toOption(target(event)),
    ) {
    | (Some(root), Some(target)) => contains(root, target)
    | _ => false
    };
};
let subscriptions = model =>
  Sub.batch([
    Sub.map(
      x => Hover(x),
      Hover_intent.subscriptions(~key=model.id, model.hover),
    ),
    Sub.map(x => Animation(x), Animation.subscriptions(model.animation)),
    Sub.map(x => Anchor(x), Anchor.subscriptions(model.anchor)),
    Sub.make(
      ~home="Tea_ui.Tooltip.pointer",
      ~key=model.id,
      ~tagger=() => Modality(false),
      ~start=
        dispatch => {
          let pointer = _ => dispatch();
          Native.listen(Native.document, "pointerdown", pointer, true);
          () =>
            Native.unlisten(Native.document, "pointerdown", pointer, true);
        },
    ),
    Events.on_key_down(
      ~key="tooltip:" ++ model.id,
      ~capture=true,
      D.and_then(
        event =>
          Ui_keyboard.available(event)
          && Ui_keyboard.plain(event)
          && !List.mem(event.key, ["Shift", "Control", "Alt", "Meta"])
            ? D.succeed(Modality(true)) : D.fail("Not keyboard modality"),
        Ui_keyboard.decoder,
      ),
    ),
    /* Hover-only users must dismiss without moving focus to the trigger (WCAG
       1.4.13). Capture only outside that trigger; its own handler claims Escape.
       defaultPrevented gives another claimed overlay priority. */
    !model.opened
      ? Sub.none
      : Sub.make(
          ~home="Tea_ui.Tooltip.escape",
          ~key=model.id,
          ~tagger=x => x,
          ~start=
            dispatch => {
              let key = event =>
                if (!Native.prevented(event)
                    && !Native.within(trigger_id(model), event)) {
                  switch (Tea_dom.Dom_decode.run(Ui_keyboard.decoder, event)) {
                  | Ok(key)
                      when
                        Ui_keyboard.available(key)
                        && Ui_keyboard.plain(key)
                        && String.equal(key.key, "Escape") =>
                    Native.prevent(event);
                    Native.stop(event);
                    dispatch(Escape(key.time_stamp));
                  | _ => ()
                  };
                };
              Native.listen(Native.document, "keydown", key, true);
              () => Native.unlisten(Native.document, "keydown", key, true);
            },
        ),
  ]);
type parts('parent) = {
  trigger: list(Html.attr('parent)),
  tooltip: list(Html.attr('parent)),
  arrow: list(Html.attr('parent)),
  open_: bool,
  visible: bool,
  position: option(Anchor.position),
};
let view = (~model, ~to_parent, ~on_key_down=_ => None, ~to_view, ()) => {
  let hover_attrs = attrs =>
    List.filter(
      fact =>
        switch (fact) {
        | V.Attribute("data-open", _)
        | V.Attribute("data-closed", _) => false
        | _ => true
        },
      attrs,
    );
  let trigger_hover =
    Hover_intent.trigger_attrs(
      ~model=model.hover,
      ~disabled=model.disabled,
      ~to_parent=x => to_parent(Hover(x)),
      (),
    );
  let content_hover =
    Hover_intent.content_attrs(
      ~model=model.hover,
      ~disabled=model.disabled,
      ~to_parent=x => to_parent(Hover(x)),
      (),
    );
  let key =
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
            if (!Ui_keyboard.plain(event)) {
              None;
            } else if (String.equal(event.key, "Escape")) {
              Some((
                Some(to_parent(Escape(event.time_stamp))),
                model.opened,
              ));
            } else if (String.equal(event.key, "Tab")) {
              None;
            } else {
              Option.map(
                message => (Some(message), false),
                on_key_down(event.key),
              );
            }
          ),
        ),
      ),
    );
  let anchor_attrs =
    List.filter(
      fact =>
        switch (fact) {
        | V.Property("hidden", _) => false
        | _ => true
        },
      Anchor.attrs(model.anchor),
    );
  let placement =
    switch (position(model)) {
    | None => Html.none
    | Some(position) =>
      Html.data("placement", Anchor.placement_name(position.placement))
    };
  to_view({
    trigger:
      hover_attrs(trigger_hover)
      @ [
        Html.id(trigger_id(model)),
        Html.aria(
          switch (model.mode) {
          | Description => "describedby"
          | Label => "labelledby"
          },
          tooltip_id(model),
        ),
        Ui_attr.data_flag("open", model.opened),
        Ui_attr.data_flag("closed", !model.opened),
        Html.on_focus(to_parent(Focus)),
        Html.on(
          "blur",
          V.Normal(
            D.map(
              time => to_parent(Blur(time)),
              D.field("timeStamp", D.float),
            ),
          ),
        ),
        key,
      ],
    tooltip:
      anchor_attrs
      @ Animation.attrs(model.animation)
      @ hover_attrs(content_hover)
      @ [Html.role("tooltip"), Ui_attr.data_flag("open", model.opened)],
    arrow: [
      Html.aria("hidden", "true"),
      placement,
      switch (position(model)) {
      | Some({ arrow_offset: Some(offset), _ }) =>
        Html.style("--tea-arrow-offset", Js.Float.toString(offset) ++ "px")
      | _ => Html.none
      },
    ],
    open_: model.opened,
    visible: is_visible(model),
    position: position(model),
  });
};
