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
type item = {
  key: string,
  label: string,
  disabled: bool,
};
type model = {
  id: string,
  focus_generation: int,
  focus_request: option(string),
  checked: option(string),
  focused: option(string),
  within: bool,
  pending: option(string),
  disabled: bool,
  readonly: bool,
  dir: direction,
  orientation,
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
  root: list(Html.attr('parent)),
  checked: bool,
  focused: bool,
};
type parts('parent) = {
  root: list(Html.attr('parent)),
  label: list(Html.attr('parent)),
  description: list(Html.attr('parent)),
  items: array(item_parts('parent)),
  hidden_input: option(Html.t('parent)),
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
  let checked = valid(items, model.checked);
  let fallback =
    switch (checked) {
    | Some(_) => checked
    | None => key_at(items, Ui_nav.first(flags(items)))
    };
  let focused =
    switch (valid(items, model.focused)) {
    | Some(_) as key => key
    | None => fallback
    };
  {
    ...model,
    checked,
    focused,
  };
};
let init =
    (
      ~id,
      ~items,
      ~checked=?,
      ~disabled=false,
      ~readonly=false,
      ~dir=Ltr,
      ~orientation=Vertical,
      (),
    ) =>
  normalize(
    ~items,
    {
      id,
      focus_generation: 0,
      focus_request: None,
      checked,
      focused: checked,
      within: false,
      pending: None,
      disabled,
      readonly,
      dir,
      orientation,
    },
  );
let checked = (model: model) => model.checked;
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
let configure =
    (~items, ~disabled=?, ~readonly=?, ~dir=?, ~orientation=?, model) => {
  let model =
    normalize(
      ~items,
      {
        ...model,
        disabled: Option.value(~default=model.disabled, disabled),
        readonly: Option.value(~default=model.readonly, readonly),
        dir: Option.value(~default=model.dir, dir),
        orientation: Option.value(~default=model.orientation, orientation),
      },
    );
  let entry =
    switch (model.checked) {
    | Some(_) => model.checked
    | None => key_at(items, Ui_nav.first(flags(items)))
    };
  let focused = model.readonly ? model.focused : entry;
  let model = {
    ...model,
    focused,
    pending: model.within ? focused : None,
  };
  (model.within ? request_focus(model, focused) : model, Cmd.none);
};
let set_checked = (~items, key, model) => {
  let model =
    normalize(
      ~items,
      {
        ...model,
        checked: key,
        focused: key,
      },
    );
  (model.within ? request_focus(model, model.focused) : model, Cmd.none);
};
let step = (model, key) =>
  switch (key) {
  | "ArrowDown" => Some(1)
  | "ArrowUp" => Some(-1)
  | key =>
    Ui_nav.horizontal(~dir=model.dir == Rtl ? Ui_nav.Rtl : Ui_nav.Ltr, key)
  };
let update = (~items, msg, original) => {
  let (model, repair) = repair_state(~items, ~force=false, original);
  let choose = (~move, key) => {
    let changed =
      !model.disabled && !model.readonly && !same(model.checked, key);
    let model = {
      ...model,
      focused: key,
      within: true,
      pending: move ? key : model.pending,
      checked: changed ? key : model.checked,
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
  | Click(key) when !model.disabled && valid(items, Some(key)) != None =>
    choose(~move=true, Some(key))
  | Focus(key) when valid(items, Some(key)) != None =>
    switch (model.pending) {
    | Some(_) when !same(model.pending, Some(key)) => (model, repair, [])
    | _ => (
        {
          ...model,
          focused: Some(key),
          within: true,
          pending: None,
        },
        repair,
        [],
      )
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
          focused:
            switch (model.checked) {
            | Some(_) => model.checked
            | None => key_at(items, Ui_nav.first(flags(items)))
            },
        },
      repair,
      [],
    );
  | Key(source, key)
      when !model.disabled && valid(items, Some(source)) != None =>
    switch (step(model, key)) {
    | Some(delta) =>
      let from = index(items, model.focused);
      let next =
        delta > 0
          ? Ui_nav.next(~from, flags(items))
          : Ui_nav.prev(~from, flags(items));
      switch (key_at(items, next)) {
      | Some(_) as key => choose(~move=true, key)
      | None => (model, repair, [])
      };
    | None when String.equal(key, " ") && !model.readonly =>
      choose(~move=false, model.focused)
    | None => (model, repair, [])
    }
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
      ~description=false,
      ~required=false,
      ~invalid=false,
      ~name=?,
      ~on_key_down=_ => None,
      ~to_view,
      (),
    ) => {
  let model = normalize(~items, model);
  let part = Ui_attr.part_id(model.id);
  let entry =
    switch (model.checked) {
    | Some(_) => model.checked
    | None => key_at(items, Ui_nav.first(flags(items)))
    };
  let tabstop = model.readonly && model.within ? model.focused : entry;
  let map = attrs => List.map(V.map_fact(to_parent), attrs);
  to_view({
    root:
      map([
        Html.id(model.id),
        Html.role("radiogroup"),
        Html.tabindex(-1),
        Html.aria("labelledby", part("label")),
        description
          ? Html.aria("describedby", part("description")) : Html.none,
        model.disabled ? Ui_attr.aria_bool("disabled", true) : Html.none,
        model.readonly ? Ui_attr.aria_bool("readonly", true) : Html.none,
        required ? Ui_attr.aria_bool("required", true) : Html.none,
        invalid ? Ui_attr.aria_bool("invalid", true) : Html.none,
        Ui_attr.data_flag("disabled", model.disabled),
        Ui_attr.data_flag("readonly", model.readonly),
        Ui_attr.data_flag("invalid", invalid),
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
    description: [Html.id(part("description"))],
    items:
      Array.map(
        (item: item) => {
          let disabled = model.disabled || item.disabled;
          let checked = same(model.checked, Some(item.key));
          let focused = model.within && same(model.focused, Some(item.key));
          {
            item,
            checked,
            focused,
            root:
              map([
                Html.id(Ui_attr.item_id(model.id, item.key)),
                Html.role("radio"),
                Html.aria("label", item.label),
                Ui_attr.aria_bool("checked", checked),
                Html.tabindex(
                  !item.disabled && same(tabstop, Some(item.key)) ? 0 : (-1),
                ),
                disabled ? Ui_attr.aria_bool("disabled", true) : Html.none,
                Ui_attr.data_flag("checked", checked),
                Ui_attr.data_flag("active", focused),
                Ui_attr.data_flag("disabled", disabled),
                Ui_attr.data_flag("readonly", model.readonly),
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
                      prevent_default: disabled,
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
                                 || String.equal(event.key, " ")) {
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
          };
        },
        items,
      ),
    hidden_input:
      switch (name, model.checked) {
      | (Some(name), Some(key)) =>
        Some(
          Html.input(
            [
              Html.type_("hidden"),
              Html.name(name),
              Html.value(key),
              Html.disabled(model.disabled),
            ],
            [],
          ),
        )
      | _ => None
      },
  });
};
