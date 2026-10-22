module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Cmd = Tea_core.Cmd;
module Events = Tea_browser.Browser_events;
type direction =
  | Ltr
  | Rtl;
type orientation =
  | Horizontal
  | Vertical;
type activation =
  | Automatic
  | Manual;
type panel_rendering =
  | Selected_only
  | Keep_mounted;
type item = {
  key: string,
  label: string,
  disabled: bool,
};
type model = {
  id: string,
  focus_generation: int,
  focus_request: option(string),
  selected: option(string),
  focused: option(string),
  within: bool,
  pending: option(string),
  dir: direction,
  orientation,
  activation,
};
type msg =
  | Key(string, string)
  | Click(string)
  | Focus(string)
  | Leave(option(string))
  | Focus_finished(int);
type out_msg =
  | Changed(string);
type item_parts('parent) = {
  item,
  tab: list(Html.attr('parent)),
  panel: list(Html.attr('parent)),
  selected: bool,
  focused: bool,
  render_panel: bool,
};
type parts('parent) = {
  root: list(Html.attr('parent)),
  label: list(Html.attr('parent)),
  items: array(item_parts('parent)),
};
let same = (a, b) =>
  switch (a, b) {
  | (Some(a), Some(b)) => String.equal(a, b)
  | (None, None) => true
  | _ => false
  };
let index = (items: array(item), key) => {
  let rec find = i =>
    i >= Array.length(items)
      ? (-1) : same(Some(items[i].key), key) ? i : find(i + 1);
  find(0);
};
let flags = items => Array.map((item: item) => item.disabled, items);
let key_at = (items: array(item), i) => Option.map(i => items[i].key, i);
let valid = (items: array(item), key) => {
  let i = index(items, key);
  i >= 0 && !items[i].disabled ? key : None;
};
let normalize = (~items, model: model) => {
  let selected =
    switch (valid(items, model.selected)) {
    | Some(_) as key => key
    | None => key_at(items, Ui_nav.first(flags(items)))
    };
  let focused =
    switch (valid(items, model.focused)) {
    | Some(_) as key => key
    | None => selected
    };
  {
    ...model,
    selected,
    focused,
  };
};
let init =
    (
      ~id,
      ~items,
      ~selected=?,
      ~dir=Ltr,
      ~orientation=Horizontal,
      ~activation=Automatic,
      (),
    ) =>
  normalize(
    ~items,
    {
      id,
      focus_generation: 0,
      focus_request: None,
      selected,
      focused: selected,
      within: false,
      pending: None,
      dir,
      orientation,
      activation,
    },
  );
let selected = (model: model) => model.selected;
let focused = (model: model) => model.focused;
let request_focus = (model, key) => {
  ...model,
  focus_generation: model.focus_generation + 1,
  focus_request:
    Some(
      switch (key) {
      | None => model.id
      | Some(key) => Ui_attr.item_id(model.id, key)
      },
    ),
};
let repair_state = (~items, ~force, model) => {
  let repaired = normalize(~items, model);
  let repair_focus =
    model.within && (force || !same(model.focused, repaired.focused));
  let repaired = {
    ...repaired,
    pending: repair_focus ? repaired.focused : model.pending,
  };
  (
    repair_focus ? request_focus(repaired, repaired.focused) : repaired,
    Cmd.none,
  );
};
let reconcile = (~items, model) => repair_state(~items, ~force=true, model);
let configure = (~items, ~dir=?, ~orientation=?, ~activation=?, model) => {
  let model =
    normalize(
      ~items,
      {
        ...model,
        dir: Option.value(~default=model.dir, dir),
        orientation: Option.value(~default=model.orientation, orientation),
        activation: Option.value(~default=model.activation, activation),
      },
    );
  let focused = model.activation == Automatic ? model.selected : model.focused;
  let model = {
    ...model,
    focused,
    pending: model.within ? focused : None,
  };
  (model.within ? request_focus(model, focused) : model, Cmd.none);
};
let set_selected = (~items, key, model) => {
  let model =
    normalize(
      ~items,
      {
        ...model,
        selected: Some(key),
        focused: Some(key),
      },
    );
  (model.within ? request_focus(model, model.focused) : model, Cmd.none);
};
let step = (model, key) =>
  switch (model.orientation, key) {
  | (Vertical, "ArrowDown") => Some(1)
  | (Vertical, "ArrowUp") => Some(-1)
  | (Horizontal, key) =>
    Ui_nav.horizontal(~dir=model.dir == Rtl ? Ui_nav.Rtl : Ui_nav.Ltr, key)
  | _ => None
  };
let update = (~items, msg, original) => {
  let (model, repair) = repair_state(~items, ~force=false, original);
  let choose = (~move, ~activate, key) => {
    let changed = activate && !same(model.selected, key);
    let model = {
      ...model,
      focused: key,
      within: true,
      pending: move ? key : None,
      selected: changed ? key : model.selected,
    };
    (
      move ? request_focus(model, key) : model,
      repair,
      changed
        ? switch (key) {
          | Some(key) => [Changed(key)]
          | None => []
          }
        : [],
    );
  };
  switch (msg) {
  | Click(key) when valid(items, Some(key)) != None =>
    choose(~move=true, ~activate=true, Some(key))
  | Focus(key) when valid(items, Some(key)) != None =>
    switch (model.pending) {
    | Some(_) when !same(model.pending, Some(key)) => (model, repair, [])
    | Some(_) => (
        {
          ...model,
          pending: None,
          within: true,
        },
        repair,
        [],
      )
    | None =>
      choose(~move=false, ~activate=model.activation == Automatic, Some(key))
    }
  | Leave(target) =>
    let inside =
      same(target, Some(model.id))
      || Array.exists(
           (item: item) =>
             same(target, Some(Ui_attr.item_id(model.id, item.key))),
           items,
         );
    (
      inside
        ? model
        : {
          ...model,
          within: false,
          pending: None,
          focus_request: None,
          focused: model.selected,
        },
      repair,
      [],
    );
  | Key(source, key) when valid(items, Some(source)) != None =>
    let target =
      switch (key) {
      | "Home" => Ui_nav.first(flags(items))
      | "End" => Ui_nav.last(flags(items))
      | key =>
        switch (step(model, key)) {
        | Some(delta) =>
          let from = index(items, model.focused);
          delta > 0
            ? Ui_nav.next(~from, flags(items))
            : Ui_nav.prev(~from, flags(items));
        | None => None
        }
      };
    switch (key_at(items, target)) {
    | Some(_) as key =>
      choose(~move=true, ~activate=model.activation == Automatic, key)
    | None when String.equal(key, " ") || String.equal(key, "Enter") =>
      choose(~move=false, ~activate=true, model.focused)
    | None => (model, repair, [])
    };
  | Focus_finished(generation) when generation == model.focus_generation => (
      {
        ...model,
        focus_request: None,
        pending: None,
      },
      repair,
      [],
    )
  | _ => (model, repair, [])
  };
};
let subscriptions = model =>
  switch (model.focus_request) {
  | Some(target) when model.within =>
    Events.focus(
      ~key=
        string_of_int(String.length(model.id))
        ++ ":"
        ++ model.id
        ++ ":"
        ++ string_of_int(model.focus_generation),
      ~target=Events.Element(target),
      ~from=[model.id],
      _ =>
      Focus_finished(model.focus_generation)
    )
  | _ => Tea_core.Sub.none
  };
let view =
    (
      ~model,
      ~items,
      ~to_parent,
      ~aria_label=?,
      ~panel_rendering=Keep_mounted,
      ~panel_focusable=_ => true,
      ~on_key_down=_ => None,
      ~to_view,
      (),
    ) => {
  let model = normalize(~items, model);
  let part = Ui_attr.part_id(model.id);
  let tabstop = model.within ? model.focused : model.selected;
  let map = attrs => List.map(V.map_fact(to_parent), attrs);
  to_view({
    root:
      map([
        Html.id(model.id),
        Html.role("tablist"),
        Html.tabindex(-1),
        switch (aria_label) {
        | Some(label) => Html.aria("label", label)
        | None => Html.aria("labelledby", part("label"))
        },
        Html.aria(
          "orientation",
          model.orientation == Vertical ? "vertical" : "horizontal",
        ),
        Html.data(
          "orientation",
          model.orientation == Vertical ? "vertical" : "horizontal",
        ),
        Html.attribute("dir", model.dir == Rtl ? "rtl" : "ltr"),
        Html.on(
          "focusout",
          V.Normal(
            D.map(
              target => Leave(target),
              D.optional(D.at(["relatedTarget", "id"], D.string)),
            ),
          ),
        ),
      ]),
    label: [Html.id(part("label"))],
    items:
      Array.map(
        (item: item) => {
          let selected = same(model.selected, Some(item.key));
          let focused = model.within && same(model.focused, Some(item.key));
          let panel_id = Ui_attr.item_id(part("panel"), item.key);
          let render_panel = panel_rendering == Keep_mounted || selected;
          {
            item,
            selected,
            focused,
            render_panel,
            tab:
              map([
                Html.id(Ui_attr.item_id(model.id, item.key)),
                Html.role("tab"),
                Html.aria("label", item.label),
                Ui_attr.aria_bool("selected", selected),
                Html.aria("controls", panel_id),
                Html.tabindex(
                  !item.disabled && same(tabstop, Some(item.key)) ? 0 : (-1),
                ),
                item.disabled
                  ? Ui_attr.aria_bool("disabled", true) : Html.none,
                Ui_attr.data_flag("selected", selected),
                Ui_attr.data_flag("active", focused),
                Ui_attr.data_flag("disabled", item.disabled),
                Html.prevent_default_on(
                  "click",
                  D.succeed((Click(item.key), true)),
                ),
                Html.on_focus(Focus(item.key)),
                Html.on(
                  "pointerdown",
                  V.Custom_optional(
                    D.succeed({
                      V.message: None,
                      stop_propagation: false,
                      prevent_default: item.disabled,
                    }),
                  ),
                ),
              ])
              @ [
                Html.on(
                  "keydown",
                  V.Custom_optional(
                    Ui_keyboard.custom(event =>
                      if (!Ui_keyboard.plain(event)
                          || String.equal(event.key, "Tab")) {
                        None;
                      } else if (step(model, event.key) != None
                                 || List.exists(
                                      String.equal(event.key),
                                      ["Home", "End", "Enter", " "],
                                    )) {
                        Some((
                          Some(to_parent(Key(item.key, event.key))),
                          true,
                        ));
                      } else {
                        Option.map(
                          msg => (Some(msg), false),
                          on_key_down(event.key),
                        );
                      }
                    ),
                  ),
                ),
              ],
            panel: [
              Html.id(panel_id),
              Html.role("tabpanel"),
              Html.aria("labelledby", Ui_attr.item_id(model.id, item.key)),
              Html.hidden(!selected),
              panel_focusable(item.key) ? Html.tabindex(0) : Html.none,
              Ui_attr.data_flag("selected", selected),
            ],
          };
        },
        items,
      ),
  });
};
