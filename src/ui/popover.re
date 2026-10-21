module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Browser = Tea_browser.Browser_dom;
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
      ~anchor_id=Ui_attr.part_id(id, "trigger"),
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
/* Focus containment cannot be expressed by tea.browser's outside source:
   its focus=true option conflates pointerdown and focusin, losing the reason,
   and does not observe blur() to body. This private source preserves both. */
module Native = {
  external document: Dom.document = "document";
  [@mel.send]
  external by_id: (Dom.document, string) => Js.Nullable.t(Dom.node) =
    "getElementById";
  [@mel.get]
  external active: Dom.document => Js.Nullable.t(Dom.node) = "activeElement";
  [@mel.get] external target: Dom.event => Js.Nullable.t(Dom.node) = "target";
  [@mel.get]
  external related: Dom.event => Js.Nullable.t(Dom.node) = "relatedTarget";
  [@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
  [@mel.send] external focus: Dom.node => unit = "focus";
  [@mel.get]
  external parent: Dom.node => Js.Nullable.t(Dom.node) = "parentElement";
  [@mel.get] external local_name: Dom.node => string = "localName";
  [@mel.get] external tab_index: Dom.node => int = "tabIndex";
  [@mel.get] external name: Dom.node => string = "name";
  [@mel.get] external form: Dom.node => Js.Nullable.t(Dom.node) = "form";
  [@mel.get] external checked: Dom.node => bool = "checked";
  [@mel.send] external matches: (Dom.node, string) => bool = "matches";
  [@mel.send] external has: (Dom.node, string) => bool = "hasAttribute";
  [@mel.send]
  external query: (Dom.node, string) => array(Dom.node) = "querySelectorAll";
  [@mel.send]
  external query_document: (Dom.document, string) => array(Dom.node) =
    "querySelectorAll";
  external window: Dom.window = "window";
  [@mel.send]
  external computed: (Dom.window, Dom.node) => Dom.cssStyleDeclaration =
    "getComputedStyle";
  [@mel.send]
  external style: (Dom.cssStyleDeclaration, string) => string =
    "getPropertyValue";
  [@mel.send]
  external add: (Dom.document, string, Dom.event => unit, bool) => unit =
    "addEventListener";
  [@mel.send]
  external remove: (Dom.document, string, Dom.event => unit, bool) => unit =
    "removeEventListener";
  let within = (ids, node) =>
    switch (Js.Nullable.toOption(node)) {
    | None => false
    | Some(node) =>
      List.exists(
        id =>
          switch (Js.Nullable.toOption(by_id(document, id))) {
          | None => false
          | Some(root) => contains(root, node)
          },
        ids,
      )
    };
  /* Browser_dom focus tasks cannot be cancelled between scheduling and focus.
     The subscription below needs an atomic live check + focus, including when
     focus leaves or the component unmounts in the intervening frame. */
  let rec visible = node => {
    let css = computed(window, node);
    !has(node, "hidden")
    && !has(node, "inert")
    && !String.equal(style(css, "display"), "none")
    && !List.mem(style(css, "visibility"), ["hidden", "collapse"])
    && (
      switch (Js.Nullable.toOption(parent(node))) {
      | None => true
      | Some(parent) =>
        (
          !String.equal(local_name(parent), "details")
          || has(parent, "open")
          || String.equal(local_name(node), "summary")
        )
        && visible(parent)
      }
    );
  };
  let radio_stop = node =>
    !matches(node, "input[type=radio]")
    || String.equal(name(node), "")
    || !
         Array.exists(
           other =>
             other !== node
             && checked(other)
             && String.equal(name(other), name(node))
             && form(other) === form(node)
             && !matches(other, ":disabled"),
           query_document(document, "input[type=radio]"),
         );
  let first = root => {
    let candidates =
      query(
        root,
        "a[href],area[href],button,input:not([type=hidden]),select,textarea,iframe,summary,[tabindex],[contenteditable]:not([contenteditable=false]),audio[controls],video[controls]",
      );
    Array.fold_left(
      (best, node) => {
        let tab = tab_index(node);
        let implicit =
          matches(
            node,
            "[contenteditable]:not([contenteditable=false]),audio[controls],video[controls]",
          );
        if ((tab >= 0 || implicit && !has(node, "tabindex"))
            && !matches(node, ":disabled,input[type=hidden]")
            && visible(node)
            && radio_stop(node)) {
          let priority = tab > 0 ? tab : max_int;
          switch (best) {
          | Some((_, old)) when old <= priority => best
          | _ => Some((node, priority))
          };
        } else {
          best;
        };
      },
      None,
      candidates,
    )
    |> Option.map(((node, _)) => node);
  };
};
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
          if (live^ && !Native.within(ids, Native.active(Native.document))) {
            dispatch(Dismiss(Focus_out));
          };
        let on_in = event =>
          if (live^ && !Native.within(ids, Native.target(event))) {
            dispatch(Dismiss(Focus_out));
          };
        let on_out = event =>
          if (Js.Nullable.isNullable(Native.related(event))) {
            Tea_dom.Browser_platform.request_animation_frame(_ => check());
          };
        Native.add(Native.document, "focusin", on_in, true);
        Native.add(Native.document, "focusout", on_out, true);
        () => {
          live := false;
          Native.remove(Native.document, "focusin", on_in, true);
          Native.remove(Native.document, "focusout", on_out, true);
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
let focus_effect = (model, policy) =>
  Sub.make(
    ~home="Tea_ui.Popover.focus_effect",
    ~key=model.id ++ ":" ++ string_of_int(model.focus_generation),
    ~start=
      dispatch => {
        let live = ref(true);
        Browser.active_element_id(_ =>
          if (live^) {
            /* A restoring dismissal must not steal a subsequent intentional focus
               destination. Outside and focus-out dismissals never request this. */
            let allowed =
              model.opened
              || Native.within(
                   [trigger_id(model), panel_id(model)],
                   Native.active(Native.document),
                 )
              || (
                switch (Js.Nullable.toOption(Native.active(Native.document))) {
                | None => true
                | Some(node) =>
                  String.equal(Native.local_name(node), "body")
                }
              );
            if (allowed) {
              let target: option(Dom.node) =
                switch (policy) {
                | None => None
                | Element(id) =>
                  Js.Nullable.toOption(Native.by_id(Native.document, id))
                | First_tabbable =>
                  Option.bind(
                    Js.Nullable.toOption(
                      Native.by_id(Native.document, panel_id(model)),
                    ),
                    Native.first,
                  )
                };
              Option.iter(Native.focus, target);
            };
            if (live^) {
              dispatch(Focus_finished(model.focus_generation));
            };
          }
        );
        () => live := false;
      },
    ~tagger=message => message,
  );
let subscriptions = model =>
  Sub.batch([
    switch (model.focus_job) {
    | None => Sub.none
    | Some(policy) => focus_effect(model, policy)
    },
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
    List.filter(
      fact =>
        switch (fact) {
        | V.Event("keydown", _) => false
        | _ => true
        },
      trigger,
    )
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
  let anchor_attrs =
    List.filter(
      fact =>
        switch (fact) {
        | V.Property("hidden", _) => false
        | _ => true
        },
      Anchor.attrs(model.anchor),
    );
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
