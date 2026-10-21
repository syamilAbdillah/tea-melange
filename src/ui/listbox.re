module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module D = Tea_core.Decode;
module Dom = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
/* Browser_dom.focus has no cancellation handle. These private bindings allow
   runtime-owned, guarded post-render focus. No runtime cells enter the model. */
module Native = {
  type element;
  type document;
  external document: document = "document";
  [@mel.send]
  external find: (document, string) => Js.Nullable.t(element) =
    "getElementById";
  [@mel.send]
  external focus: (element, {. "preventScroll": bool }) => unit = "focus";
};
type focus_lease = {
  live: ref(bool),
  epoch: ref(int),
  opened: ref(bool),
  restore: ref(bool),
};
/* Live leases exist only between subscription start/stop, keyed by the
   document-unique prefix. Commands set runtime policy; pure functions never
   inspect this registry. No runtime state is stored in or captured by model. */
let focus_leases: ref(list((string, focus_lease))) = ref([]);
let lease_for = id =>
  List.find_map(
    ((key, lease)) => String.equal(key, id) ? Some(lease) : None,
    focus_leases^,
  );
let policy = (id, epoch, opened, restore) =>
  Cmd.effect(_ =>
    Option.iter(
      lease => {
        lease.epoch := epoch;
        lease.opened := opened;
        lease.restore := restore;
      },
      lease_for(id),
    )
  );
let focus_if = (owner, epoch, opened, id) =>
  Cmd.effect(_ =>
    Option.iter(
      lease => {
        /* Capture this actual lease, not just its id: a later mounted instance
           may reuse the prefix, but cannot revive this stopped callback. */
        Tea_dom.Browser_platform.request_animation_frame(_ =>
          if (lease.live^ && lease.epoch^ == epoch && lease.opened^ == opened) {
            Option.iter(
              element => Native.focus(element, { "preventScroll": true }),
              Js.Nullable.toOption(Native.find(Native.document, id)),
            );
          }
        )
      },
      lease_for(owner),
    )
  );
/* restore_focus owns no listeners: its disposer is the restoration itself.
   Cancelling that one-shot action is required for outside/Tab dismissal. */
let rec connect: type a. (Sub.t(a), a => unit, unit) => unit =
        (sub, dispatch) =>
  switch (sub) {
  | Sub.None => (() => ())
  | Sub.Batch(subs) =>
    let stops = List.map(sub => connect(sub, dispatch), subs);
    (() => List.iter(stop => stop(), stops));
  | Sub.Leaf(leaf) => leaf.start(raw => dispatch(leaf.tagger(raw)))
  | Sub.Filtered(leaf) =>
    leaf.start(raw => Option.iter(dispatch, leaf.tagger(raw)))
  };
type item('value) = {
  key: string,
  label: string,
  disabled: bool,
  value: 'value,
};
type selection('value) = {
  keys: list(string),
  values: list('value),
};
type out_msg('value) =
  | Changed(selection('value))
  | Opened
  | Closed;
type model = {
  id: string,
  selected: list(string),
  active: option(string),
  position: int,
  multiple: bool,
  disabled: bool,
  readonly: bool,
  invalid: bool,
  wrap: bool,
  animation: Animation.model,
  anchor: Anchor.model,
  search: Ui_typeahead.t,
  focus_pending: bool,
  focus_epoch: int,
  return_focus: bool,
};
type msg =
  | Trigger
  | Key(bool, Ui_keyboard.event)
  | Point(string)
  | Select(string)
  | Outside
  | Blur(option(string))
  | Animated(Animation.msg)
  | Anchored(Anchor.msg);
let part = (model, name) => Ui_attr.part_id(model.id, name);
let is_open = model =>
  switch (Animation.phase(model.animation)) {
  | Animation.Entering
  | Animation.Open => true
  | Animation.Closed
  | Animation.Leaving => false
  };
let should_render = model => Animation.should_render(model.animation);
let selected_keys = model => model.selected;
let active_key = model => model.active;
let phase = model => Animation.phase(model.animation);
let unique = keys =>
  List.fold_left(
    (acc, key) => List.exists(String.equal(key), acc) ? acc : acc @ [key],
    [],
    keys,
  );
let selection_keys = (multiple, keys) =>
  switch (unique(keys)) {
  | [] => []
  | [first, ...rest] => multiple ? [first, ...rest] : [first]
  };
let set_selected = (keys, model) => {
  ...model,
  selected: selection_keys(model.multiple, keys),
};
let init =
    (
      ~id,
      ~selected=[],
      ~multiple=false,
      ~disabled=false,
      ~readonly=false,
      ~invalid=false,
      ~wrap=false,
      ~dir=Anchor.Ltr,
      ~top_layer=true,
      (),
    ) => {
  id,
  selected: selection_keys(multiple, selected),
  active: None,
  position: 0,
  multiple,
  disabled,
  readonly,
  invalid,
  wrap,
  animation:
    Animation.init(~id=Ui_attr.part_id(id, "listbox"), ~open_=false, ()),
  anchor:
    Anchor.init(
      ~anchor_id=Ui_attr.part_id(id, "button"),
      ~floating_id=Ui_attr.part_id(id, "listbox"),
      ~dir,
      ~top_layer,
      ~match_width=true,
      ~offset=4.,
      (),
    ),
  search: Ui_typeahead.empty,
  focus_pending: false,
  focus_epoch: 0,
  return_focus: false,
};
let index = (items, key) => {
  let rec find = i =>
    i >= Array.length(items)
      ? None : String.equal(items[i].key, key) ? Some(i) : find(i + 1);
  find(0);
};
let flags = (items: array(item('value))) =>
  Array.map((item: item('value)) => item.disabled, items);
let current = (items, model) =>
  Option.bind(model.active, key => index(items, key));
let repaired = (items: array(item('value)), model) => {
  let enabled = i => i >= 0 && i < Array.length(items) && !items[i].disabled;
  switch (current(items, model)) {
  | Some(i) when enabled(i) => Some(i)
  | _ =>
    let center =
      max(
        0,
        min(
          Option.value(~default=model.position, current(items, model)),
          Array.length(items) - 1,
        ),
      );
    let rec nearest = distance =>
      distance >= Array.length(items)
        ? None
        : enabled(center + distance)
            ? Some(center + distance)
            : enabled(center - distance)
                ? Some(center - distance) : nearest(distance + 1);
    nearest(0);
  };
};
let with_active = (items, target, model) => {
  ...model,
  active: Option.map(i => items[i].key, target),
  position: Option.value(~default=0, target),
};
let scroll = model =>
  switch (model.active) {
  | Some(key) when is_open(model) =>
    Cmd.effect(_ =>
      Dom.scroll_into_view(~block=`Nearest, Ui_attr.item_id(model.id, key), _ =>
        ()
      )
    )
  | _ => Cmd.none
  };
let reconcile = (~items, model) => {
  let next = with_active(items, repaired(items, model), model);
  (next, scroll(next));
};
let open_ = (~items, ~last=false, model) =>
  if (model.disabled || is_open(model)) {
    (model, Cmd.none);
  } else {
    let selected =
      Array.mapi((i, item) => (i, item), items)
      |> Array.to_list
      |> List.find_opt(((_, item: item('value))) =>
           !item.disabled
           && List.exists(String.equal(item.key), model.selected)
         );
    let target =
      switch (selected) {
      | Some((i, _)) => Some(i)
      | None =>
        last ? Ui_nav.last(flags(items)) : Ui_nav.first(flags(items))
      };
    let (anchor, ac) = Anchor.start(model.anchor);
    let (animation, an) = Animation.show(model.animation);
    let next =
      with_active(
        items,
        target,
        {
          ...model,
          anchor,
          animation,
          search: Ui_typeahead.empty,
          focus_pending: true,
          focus_epoch: model.focus_epoch + 1,
          return_focus: true,
        },
      );
    (
      next,
      Cmd.batch([
        policy(model.id, next.focus_epoch, true, true),
        Cmd.map(m => Anchored(m), ac),
        Cmd.map(m => Animated(m), an),
        /* A retained position is already visible on a leave reversal. Initial
           opening waits for Positioned so visibility:hidden cannot eat focus. */
        Option.is_some(Anchor.position(anchor))
          ? focus_if(
              model.id,
              next.focus_epoch,
              true,
              part(model, "listbox"),
            )
          : Cmd.none,
        scroll(next),
      ]),
    );
  };
let close = (~return_focus=false, model) =>
  if (!is_open(model)) {
    (model, Cmd.none);
  } else {
    let (animation, command) = Animation.hide(model.animation);
    (
      {
        ...model,
        animation,
        focus_pending: false,
        focus_epoch: model.focus_epoch + 1,
        return_focus,
        search: Ui_typeahead.empty,
      },
      Cmd.batch([
        policy(model.id, model.focus_epoch + 1, false, return_focus),
        Cmd.map(m => Animated(m), command),
        return_focus && is_open(model)
          ? focus_if(
              model.id,
              model.focus_epoch + 1,
              false,
              part(model, "button"),
            )
          : Cmd.none,
      ]),
    );
  };
let configure = (~disabled=?, ~readonly=?, ~invalid=?, ~wrap=?, model) => {
  let next = {
    ...model,
    disabled: Option.value(~default=model.disabled, disabled),
    readonly: Option.value(~default=model.readonly, readonly),
    invalid: Option.value(~default=model.invalid, invalid),
    wrap: Option.value(~default=model.wrap, wrap),
  };
  next.disabled ? close(next) : (next, Cmd.none);
};
let change = (~items, key, model) =>
  switch (index(items, key)) {
  | Some(i)
      when
        is_open(model)
        && !model.disabled
        && !model.readonly
        && !items[i].disabled =>
    let keys =
      model.multiple
        ? List.exists(String.equal(key), model.selected)
            ? List.filter(k => !String.equal(k, key), model.selected)
            : model.selected @ [key]
        : [key];
    let changed = !List.equal(String.equal, keys, model.selected);
    let next =
      with_active(
        items,
        Some(i),
        {
          ...model,
          selected: keys,
          search: Ui_typeahead.empty,
        },
      );
    let (next, command) =
      model.multiple ? (next, Cmd.none) : close(~return_focus=true, next);
    let values =
      List.filter_map(
        key => Option.map(i => items[i].value, index(items, key)),
        keys,
      );
    (
      next,
      command,
      changed
        ? [
          Changed({
            keys,
            values,
          }),
        ]
        : [],
    );
  | _ => (model, Cmd.none, [])
  };
let printable = key => {
  let n = String.length(key);
  n == 1
  || n == 2
  && Char.code(key.[0]) >= 0xd800
  && Char.code(key.[0]) <= 0xdbff;
};
let searching = (event: Ui_keyboard.event, model) =>
  !String.equal(Ui_typeahead.buffer(model.search), "")
  && (
    switch (Ui_typeahead.last_time_stamp(model.search)) {
    | Some(last) =>
      event.time_stamp >= last && event.time_stamp -. last < 500.
    | None => false
    }
  );
let typeahead = (items, event: Ui_keyboard.event, model) => {
  let (search, target) =
    Ui_typeahead.feed(
      ~time_stamp=event.time_stamp,
      ~key=event.key,
      ~from=Option.value(~default=-1, current(items, model)),
      ~labels=Array.map(item => item.label, items),
      ~disabled=flags(items),
      model.search,
    );
  let next =
    with_active(
      items,
      Option.is_some(target) ? target : current(items, model),
      {
        ...model,
        search,
      },
    );
  (next, Cmd.none, []);
};
let update = (~items, message, original) => {
  let model = with_active(items, repaired(items, original), original);
  let (next, command, outs) =
    switch (message) {
    | Animated(message) =>
      let (animation, command, outs) =
        Animation.update(message, model.animation);
      let left = List.exists(out => out == Animation.Left, outs);
      let (anchor, stop) =
        left ? Anchor.stop(model.anchor) : (model.anchor, Cmd.none);
      (
        {
          ...model,
          animation,
          anchor,
        },
        Cmd.batch([
          Cmd.map(m => Animated(m), command),
          Cmd.map(m => Anchored(m), stop),
        ]),
        left ? [Closed] : [],
      );
    | Anchored(message) =>
      let (anchor, command, outs) = Anchor.update(message, model.anchor);
      let positioned =
        List.exists(
          out =>
            switch (out) {
            | Anchor.Positioned(position) => !position.reference_hidden
            | _ => false
            },
          outs,
        );
      let focus = positioned && model.focus_pending && is_open(model);
      (
        {
          ...model,
          anchor,
          focus_pending: model.focus_pending && !focus,
        },
        Cmd.batch([
          Cmd.map(m => Anchored(m), command),
          focus
            ? focus_if(
                model.id,
                model.focus_epoch,
                true,
                part(model, "listbox"),
              )
            : Cmd.none,
        ]),
        [],
      );
    | Blur(target)
        when
          !is_open(model)
          || Option.value(
               ~default=false,
               Option.map(String.equal(part(model, "button")), target),
             ) => (
        model,
        Cmd.none,
        [],
      )
    | Outside
    | Blur(_) =>
      let (model, command) = close(model);
      (model, command, []);
    | Trigger when model.disabled => (model, Cmd.none, [])
    | Trigger =>
      let opening = !is_open(model);
      let (model, command) =
        opening ? open_(~items, model) : close(~return_focus=true, model);
      (model, command, opening ? [Opened] : []);
    | Point(key) when is_open(model) && !model.disabled =>
      switch (index(items, key)) {
      | Some(i) when !items[i].disabled => (
          with_active(
            items,
            Some(i),
            {
              ...model,
              search: Ui_typeahead.empty,
            },
          ),
          Cmd.none,
          [],
        )
      | _ => (model, Cmd.none, [])
      }
    | Point(_) => (model, Cmd.none, [])
    | Select(key) => change(~items, key, model)
    | Key(trigger, event) when !model.disabled =>
      if (String.equal(event.key, "Tab")) {
        let (model, command) = close(model);
        (model, command, []);
      } else if (!Ui_keyboard.plain(event)) {
        (model, Cmd.none, []);
      } else if (String.equal(event.key, "Escape")) {
        let (model, command) = close(~return_focus=true, model);
        (model, command, []);
      } else if (trigger && !is_open(model)) {
        let (next, command) =
          open_(~items, ~last=String.equal(event.key, "ArrowUp"), model);
        if (printable(event.key) && !String.equal(event.key, " ")) {
          let (next, search, _) = typeahead(items, event, next);
          (next, Cmd.batch([command, search]), [Opened]);
        } else {
          (next, command, [Opened]);
        };
      } else if (!is_open(model)) {
        (model, Cmd.none, []);
      } else {
        switch (event.key) {
        | "Enter"
        | " " when !String.equal(event.key, " ") || !searching(event, model) =>
          switch (model.active) {
          | Some(key) => change(~items, key, model)
          | None => (model, Cmd.none, [])
          }
        | "ArrowDown"
        | "ArrowUp"
        | "Home"
        | "End"
        | "PageDown"
        | "PageUp" =>
          let disabled = flags(items);
          let from = Option.value(~default=-1, current(items, model));
          let target =
            switch (event.key) {
            | "ArrowDown" => Ui_nav.next(~wrap=model.wrap, ~from, disabled)
            | "ArrowUp" => Ui_nav.prev(~wrap=model.wrap, ~from, disabled)
            | "Home" => Ui_nav.first(disabled)
            | "End" => Ui_nav.last(disabled)
            | key =>
              Ui_nav.page(
                ~size=10,
                ~forward=String.equal(key, "PageDown"),
                ~from,
                disabled,
              )
            };
          let next =
            with_active(
              items,
              target,
              {
                ...model,
                search: Ui_typeahead.empty,
              },
            );
          (next, Cmd.none, []);
        | _ => typeahead(items, event, model)
        };
      }
    | Key(_, _) => (model, Cmd.none, [])
    };
  (next, Cmd.batch([command, scroll(next)]), outs);
};
let subscriptions = model =>
  Sub.batch([
    Sub.map(m => Animated(m), Animation.subscriptions(model.animation)),
    Sub.map(m => Anchored(m), Anchor.subscriptions(model.anchor)),
    should_render(model)
      ? Sub.make(
          ~home="Tea_ui.Listbox.focus",
          ~key=model.id,
          ~tagger=x => x,
          ~start=
            _ => {
              let lease = {
                live: ref(true),
                epoch: ref(model.focus_epoch),
                opened: ref(is_open(model)),
                restore: ref(model.return_focus),
              };
              focus_leases := [(model.id, lease), ...focus_leases^];
              let stop =
                connect(
                  Events.restore_focus(
                    ~key=model.id,
                    ~within=part(model, "listbox"),
                    (),
                  ),
                  _ =>
                  ()
                );
              () => {
                lease.live := false;
                focus_leases :=
                  List.filter(
                    ((id, _)) => !String.equal(id, model.id),
                    focus_leases^,
                  );
                /* Normal closes already made their explicit focus decision.
                   Only unmount/shutdown of a still-open popup restores here.
                   Discarding restore_focus's stopper owns no resources. */
                if (lease.opened^ && lease.restore^) {
                  stop();
                };
              };
            },
        )
      : Sub.none,
    is_open(model)
      ? Events.on_pointer_down_outside(
          ~ids=[part(model, "button"), part(model, "listbox")],
          Outside,
        )
      : Sub.none,
  ]);
type parts('value, 'parent) = {
  root: list(Html.attr('parent)),
  label: list(Html.attr('parent)),
  button: list(Html.attr('parent)),
  listbox: list(Html.attr('parent)),
  option_: string => list(Html.attr('parent)),
  group: string => list(Html.attr('parent)),
  group_label: string => list(Html.attr('parent)),
  empty: list(Html.attr('parent)),
  selected: array(item('value)),
  active_key: option(string),
  display_value: string,
  is_open: bool,
  should_render: bool,
  form_inputs: list(Html.t('parent)),
};
let view =
    (
      ~model,
      ~items,
      ~to_parent,
      ~placeholder="Select an option",
      ~name=?,
      ~form_value=item => item.key,
      ~on_key_down=_ => None,
      ~to_view,
      (),
    ) => {
  let model = with_active(items, repaired(items, model), model);
  let mapped = attrs => List.map(V.map_fact(to_parent), attrs);
  let opened = is_open(model);
  let key = trigger =>
    Html.on(
      "keydown",
      V.Custom_optional(
        /* A claimed Escape belongs to this listbox alone: stop it so an
           enclosing Dialog or other overlay does not also dismiss. */
        D.map2(
          (claimed, key) =>
            {
              ...claimed,
              V.stop_propagation:
                claimed.V.prevent_default && String.equal(key, "Escape"),
            },
          Ui_keyboard.custom(event => {
            let handled =
              String.equal(event.key, "Tab")
              || Ui_keyboard.plain(event)
              && (
                switch (event.key) {
                | "Enter"
                | " "
                | "ArrowDown"
                | "ArrowUp" => true
                | "Escape" => opened
                | "Home"
                | "End"
                | "PageUp"
                | "PageDown" => !trigger && opened
                | key => printable(key)
                }
              );
            if (handled && !model.disabled) {
              Some((
                Some(to_parent(Key(trigger, event))),
                !String.equal(event.key, "Tab"),
              ));
            } else {
              Option.map(
                message => (Some(message), false),
                Ui_keyboard.plain(event) ? on_key_down(event.key) : None,
              );
            };
          }),
          D.field("key", D.string),
        ),
      ),
    );
  let button =
    Button.parts(
      ~id=part(model, "button"),
      ~on_press=Trigger,
      ~disabled=model.disabled,
      (),
    ).
      root
    |> List.filter(fact =>
         switch (fact) {
         | V.Event("keydown", _)
         | V.Attribute("data-disabled", _) => false
         | _ => true
         }
       )
    |> mapped;
  let anchor =
    Anchor.attrs(model.anchor)
    |> List.filter(fact =>
         switch (fact) {
         | V.Property("hidden", _) => false
         | _ => true
         }
       );
  let selected =
    Array.to_list(items)
    |> List.filter(item =>
         List.exists(String.equal(item.key), model.selected)
       );
  let state = [
    Ui_attr.data_flag("open", opened),
    Ui_attr.data_flag("disabled", model.disabled),
    Ui_attr.data_flag("readonly", model.readonly),
    Ui_attr.data_flag("invalid", model.invalid),
  ];
  let group_id = key => part(model, "group-" ++ Ui_attr.safe_key(key));
  to_view({
    root: state,
    label: [Html.id(part(model, "label"))],
    button:
      button
      @ state
      @ [
        key(true),
        Html.aria("haspopup", "listbox"),
        Ui_attr.aria_bool("expanded", opened),
        Ui_attr.aria_ids(
          "labelledby",
          [part(model, "label"), part(model, "button")],
        ),
        Ui_attr.aria_ids("controls", [part(model, "listbox")]),
        model.invalid ? Ui_attr.aria_bool("invalid", true) : Html.none,
      ],
    listbox:
      anchor
      @ Animation.attrs(model.animation)
      @ state
      @ [
        Html.role("listbox"),
        Html.tabindex(opened ? 0 : (-1)),
        Html.property("inert", V.Bool(!opened)),
        Ui_attr.aria_bool("hidden", !opened),
        key(false),
        Html.on(
          "blur",
          V.Normal(
            D.map(
              target => to_parent(Blur(target)),
              D.optional(D.at(["relatedTarget", "id"], D.string)),
            ),
          ),
        ),
        Ui_attr.aria_ids("labelledby", [part(model, "label")]),
        Ui_attr.aria_ids(
          "activedescendant",
          opened
            ? Option.to_list(
                Option.map(
                  key => Ui_attr.item_id(model.id, key),
                  model.active,
                ),
              )
            : [],
        ),
        Ui_attr.aria_bool("multiselectable", model.multiple),
        model.readonly ? Ui_attr.aria_bool("readonly", true) : Html.none,
        model.disabled ? Ui_attr.aria_bool("disabled", true) : Html.none,
        model.invalid ? Ui_attr.aria_bool("invalid", true) : Html.none,
      ],
    option_: key =>
      switch (index(items, key)) {
      | None => []
      | Some(i) =>
        let item = items[i];
        let selected = List.exists(String.equal(key), model.selected);
        let disabled = model.disabled || item.disabled;
        [
          Html.id(Ui_attr.item_id(model.id, key)),
          Html.role("option"),
          Ui_attr.aria_bool("selected", selected),
          Ui_attr.aria_bool("disabled", disabled),
          Html.aria("posinset", string_of_int(i + 1)),
          Html.aria("setsize", string_of_int(Array.length(items))),
          Ui_attr.data_flag("selected", selected),
          Ui_attr.data_flag("disabled", disabled),
          Ui_attr.data_flag(
            "active",
            opened
            && Option.value(
                 ~default=false,
                 Option.map(String.equal(key), model.active),
               ),
          ),
          Html.on(
            "pointermove",
            V.Normal(D.succeed(to_parent(Point(key)))),
          ),
          Html.on(
            "pointerdown",
            V.Custom_optional(
              D.succeed({
                V.message: None,
                prevent_default: true,
                stop_propagation: false,
              }),
            ),
          ),
          Html.on_click(to_parent(Select(key))),
        ];
      },
    group: key => [
      Html.role("group"),
      Ui_attr.aria_ids("labelledby", [group_id(key)]),
    ],
    group_label: key => [
      Html.id(group_id(key)),
      Html.role("presentation"),
    ],
    empty: [
      Html.role("option"),
      Ui_attr.aria_bool("disabled", true),
      Ui_attr.aria_bool("selected", false),
      Html.hidden(Array.length(items) != 0),
    ],
    selected: Array.of_list(selected),
    active_key: model.active,
    display_value:
      List.is_empty(selected)
        ? placeholder
        : String.concat(
            ", ",
            List.map((item: item('value)) => item.label, selected),
          ),
    is_open: opened,
    should_render: should_render(model),
    form_inputs:
      switch (name) {
      | None => []
      | Some(name) =>
        List.map(
          item =>
            Html.input(
              [
                Html.type_("hidden"),
                Html.name(name),
                Html.value(form_value(item)),
                Html.disabled(model.disabled),
              ],
              [],
            ),
          selected,
        )
      },
  });
};
