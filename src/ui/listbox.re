module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module D = Tea_core.Decode;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
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
  focus: Ui_focus.t,
};
type msg =
  | Trigger
  | Focused(int)
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
let selection_keys = (multiple, keys) =>
  switch (Ui_attr.unique_keys(keys)) {
  | [first, _, ..._] when !multiple => [first]
  | keys => keys
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
  focus: Ui_focus.none,
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
      Browser.scroll_into_view(
        ~block=`Nearest, Ui_attr.item_id(model.id, key), _ =>
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
          focus: Ui_focus.element(model.focus, part(model, "listbox")),
        },
      );
    (
      next,
      Cmd.batch([
        Cmd.map(m => Anchored(m), ac),
        Cmd.map(m => Animated(m), an),
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
        focus:
          return_focus
            ? Ui_focus.element(model.focus, part(model, "button"))
            : Ui_focus.cancel(model.focus),
        search: Ui_typeahead.empty,
      },
      Cmd.map(m => Animated(m), command),
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
      let left = List.mem(Animation.Left, outs);
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
      let (anchor, command, _) = Anchor.update(message, model.anchor);
      (
        {
          ...model,
          anchor,
        },
        Cmd.map(m => Anchored(m), command),
        [],
      );
    | Focused(generation) => (
        {
          ...model,
          focus: fst(Ui_focus.finish(model.focus, generation)),
        },
        Cmd.none,
        [],
      )
    | Blur(target)
        when !is_open(model) || target == Some(part(model, "button")) => (
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
    /* Unmount or shutdown while open returns focus to the button. A stopping
       subscription cannot read the model, so it reads the rendered data-open;
       ordinary closes have already decided focus and leave it closed. */
    Sub.make(
      ~home="Tea_ui.Listbox.unmount",
      ~key=model.id,
      ~tagger=x => x,
      ~start=
        (_, ()) => {
          let popup = part(model, "listbox");
          switch (Ui_dom.find(popup)) {
          | Some(el) when Ui_dom.matches(el, "[data-open]") =>
            Tea_dom.Browser_platform.request_animation_frame(_ => {
              let free =
                switch (Ui_dom.active()) {
                | None => true
                | Some(active) =>
                  active === Ui_dom.body(Ui_dom.document)
                  || !Ui_dom.connected(active)
                  || Ui_dom.within(popup, Some(active))
                };
              if (free) {
                Ui_dom.focus_id(part(model, "button"));
              };
            })
          | _ => ()
          };
        },
    ),
    Ui_focus.sub(
      ~home="Listbox",
      ~id=model.id,
      ~ready=
        !is_open(model) || Anchor.placed(Anchor.position(model.anchor)),
      ~from=
        is_open(model)
          ? [] : [part(model, "button"), part(model, "listbox")],
      model.focus,
      (generation, _) =>
      Focused(generation)
    ),
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
    |> Ui_attr.without("keydown")
    |> Ui_attr.without("data-disabled")
    |> mapped;
  let anchor = Ui_attr.without("hidden", Anchor.attrs(model.anchor));
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
          Ui_attr.data_flag("active", opened && model.active == Some(key)),
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
