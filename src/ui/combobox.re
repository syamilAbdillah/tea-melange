open Tea_core;
module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
module Platform = Tea_dom.Browser_platform;
module D = Decode;

type item = {
  key: string,
  label: string,
  disabled: bool,
};
type autocomplete =
  | List
  | Both
  | None_;
type unmatched =
  | Revert
  | Clear;
type mode =
  | Restricted(unmatched)
  | Free_form;
type filter_mode =
  | Contains
  | Starts_with;
type direction =
  | Ltr
  | Rtl;
type out_msg =
  | Query_changed(string)
  | Selected(string)
  | Removed(string)
  | Selection_changed(list(string))
  | Submitted(string)
  | Opened
  | Closed;

/* Same folding as typeahead: lowercase, NFD without marks, ligature folds. */
let fold = Ui_typeahead.normalize;
let filter = (~query, ~label, ~mode=Contains, items) => {
  let query = fold(query);
  Array.of_list(
    List.filter(
      item => {
        let text = fold(label(item));
        switch (mode) {
        | Starts_with => String.starts_with(~prefix=query, text)
        | Contains => Js.String.includes(~search=query, text)
        };
      },
      Array.to_list(items),
    ),
  );
};
type model = {
  id: string,
  text: string,
  query: string,
  selected: list(string),
  active: option(string),
  open_: bool,
  mode,
  multiple: bool,
  autocomplete,
  wrap: bool,
  open_on_type: bool,
  disabled: bool,
  readonly: bool,
  composing: bool,
  inline_allowed: bool,
  completion: option(int),
  loading: bool,
  pending: string,
  announcement: string,
  animation: Animation.model,
  anchor: Anchor.model,
  focus: Ui_focus.t,
};
type msg =
  | Key(Ui_keyboard.event)
  | Input(string, bool, bool)
  | Composition_start
  | Composition_end(string)
  | Blur
  | Outside
  | Toggle
  | Hover(string)
  | Choose(string)
  | Remove(string)
  | Animated(Animation.msg)
  | Anchored(Anchor.msg)
  | Announce(string)
  | Focused(int);
let part = (model, name) => Ui_attr.part_id(model.id, name);
let unique = keys =>
  List.rev(
    List.fold_left(
      (acc, key) =>
        List.exists(String.equal(key), acc) ? acc : [key, ...acc],
      [],
      keys,
    ),
  );
let selection = (~multiple, keys) =>
  multiple
    ? unique(keys)
    : (
      switch (keys) {
      | [] => []
      | [key, ..._] => [key]
      }
    );
let init =
    (
      ~id,
      ~text="",
      ~selected=[],
      ~mode=Restricted(Revert),
      ~multiple=false,
      ~autocomplete=List,
      ~wrap=true,
      ~open_on_type=true,
      ~disabled=false,
      ~readonly=false,
      ~dir=Ltr,
      ~top_layer=true,
      ~match_width=true,
      (),
    ) => {
  id,
  text,
  query: text,
  selected: selection(~multiple, selected),
  active: None,
  open_: false,
  mode,
  multiple,
  autocomplete,
  wrap,
  open_on_type,
  disabled,
  readonly,
  composing: false,
  inline_allowed: false,
  completion: None,
  loading: false,
  pending: "",
  announcement: "",
  focus: Ui_focus.none,
  animation:
    Animation.init(~id=Ui_attr.part_id(id, "popup"), ~open_=false, ()),
  anchor:
    Anchor.init(
      ~anchor_id=Ui_attr.part_id(id, "wrapper"),
      ~floating_id=Ui_attr.part_id(id, "popup"),
      ~top_layer,
      ~match_width,
      ~dir=
        switch (dir) {
        | Ltr => Anchor.Ltr
        | Rtl => Anchor.Rtl
        },
      (),
    ),
};
let text = model => model.text;
let query = model => model.query;
let selected = model => model.selected;
let is_open = model => model.open_;
let should_render = model => Animation.should_render(model.animation);
let phase = model => Animation.phase(model.animation);
let find = (~items, key) =>
  Array.find_opt((item: item) => String.equal(item.key, key), items);
let active_key = (~items, model) =>
  switch (model.active) {
  | Some(key) =>
    switch (find(~items, key)) {
    | Some(item) when !item.disabled => Some(key)
    | _ => None
    }
  | None => None
  };
let set_text = (text, model) => {
  ...model,
  text,
  query: text,
  completion: None,
  inline_allowed: false,
  active: None,
};
let set_selected = (keys, model) => {
  ...model,
  selected: selection(~multiple=model.multiple, keys),
};
let open_ = model =>
  if (model.disabled || model.open_) {
    (model, Cmd.none);
  } else {
    let (anchor, a) = Anchor.start(model.anchor);
    let (animation, b) = Animation.show(model.animation);
    (
      {
        ...model,
        open_: true,
        anchor,
        animation,
      },
      Cmd.batch([
        Cmd.map(m => Anchored(m), a),
        Cmd.map(m => Animated(m), b),
      ]),
    );
  };
let close = model => {
  let (animation, cmd) = Animation.hide(model.animation);
  (
    {
      ...model,
      open_: false,
      focus: Ui_focus.cancel(model.focus),
      animation,
      active: None,
      completion: None,
      inline_allowed: false,
      pending: "",
      announcement: "",
    },
    Cmd.map(m => Animated(m), cmd),
  );
};
let set_disabled = (disabled, model) =>
  disabled
    ? close({
        ...model,
        disabled,
      })
    : (
      {
        ...model,
        disabled,
      },
      Cmd.none,
    );
let set_readonly = (readonly, model) => {
  ...model,
  readonly,
  completion: None,
  inline_allowed: false,
};
let result_label = count => string_of_int(count) ++ " results available";
let refresh =
    (
      ~items,
      ~loading=false,
      ~results_label=result_label,
      ~empty_label="No results",
      ~loading_label="Loading",
      model,
    ) => {
  let active = active_key(~items, model);
  let model = {
    ...model,
    active,
    loading,
  };
  let model =
    if (model.open_
        && !loading
        && model.autocomplete == Both
        && model.inline_allowed
        && !model.composing
        && !String.equal(model.query, "")) {
      let candidates =
        filter(
          ~query=model.query,
          ~label=(item: item) => item.label,
          ~mode=Starts_with,
          items,
        );
      switch (Array.find_opt((item: item) => !item.disabled, candidates)) {
      | Some(item)
          when String.length(item.label) > String.length(model.query) =>
        /* Require a literal case-insensitive prefix as well: normalized Unicode
           matches can have different UTF-16 lengths and cannot define a range. */
        if (String.starts_with(
              ~prefix=Js.String.toLowerCase(model.query),
              Js.String.toLowerCase(item.label),
            )) {
          {
            ...model,
            active: Some(item.key),
            text: item.label,
            completion: Some(Js.String.length(model.query)),
          };
        } else {
          {
            ...model,
            text: model.query,
            completion: None,
          };
        }
      | _ => {
          ...model,
          text: model.query,
          completion: None,
        }
      };
    } else {
      model;
    };
  {
    ...model,
    pending:
      !model.open_
        ? ""
        : loading
            ? loading_label
            : Array.length(items) == 0
                ? empty_label : results_label(Array.length(items)),
  };
};
let opened = model => {
  let was = model.open_;
  let (model, cmd) = open_(model);
  (model, cmd, !was && model.open_ ? [Opened] : []);
};
let closed = model => {
  let was = model.open_;
  let (model, cmd) = close(model);
  (model, cmd, was ? [Closed] : []);
};
let changed = (value, model) =>
  String.equal(value, model.query) ? [] : [Query_changed(value)];
let request_focus = model => {
  ...model,
  focus: Ui_focus.element(model.focus, part(model, "control")),
};
let remove = (key, model) =>
  if (model.disabled
      || model.readonly
      || !List.exists(String.equal(key), model.selected)) {
    (model, Cmd.none, []);
  } else {
    let selected = List.filter(k => !String.equal(k, key), model.selected);
    (
      request_focus({
        ...model,
        selected,
      }),
      Cmd.none,
      [Removed(key), Selection_changed(selected)],
    );
  };
let choose = (~items, key, model) =>
  switch (find(~items, key)) {
  | Some(item)
      when !item.disabled && !model.disabled && !model.readonly && model.open_ =>
    let selected = model.multiple ? unique(model.selected @ [key]) : [key];
    let value = model.multiple ? "" : item.label;
    let outs =
      [Selected(key)]
      @ (selected == model.selected ? [] : [Selection_changed(selected)])
      @ changed(value, model);
    let model = {
      ...model,
      selected,
      text: value,
      query: value,
      completion: None,
      inline_allowed: false,
    };
    if (model.multiple) {
      (
        request_focus({
          ...model,
          active: Some(key),
        }),
        Cmd.none,
        outs,
      );
    } else {
      let (model, cmd, closing) = closed(model);
      (model, cmd, outs @ closing);
    };
  | _ => (model, Cmd.none, [])
  };
let commit_blur = (~items, ~label_for_key, model) => {
  let (next, cmd, outs) = closed(model);
  if (model.readonly || model.disabled || model.mode == Free_form) {
    (next, cmd, outs);
  } else {
    let exact =
      Array.find_opt(
        (item: item) =>
          !item.disabled && String.equal(item.label, model.text),
        items,
      );
    switch (exact) {
    | Some(item) when !model.multiple =>
      let selected = [item.key];
      (
        {
          ...next,
          selected,
          query: model.text,
        },
        cmd,
        outs
        @ changed(model.text, model)
        @ (
          model.selected == selected
            ? [] : [Selected(item.key), Selection_changed(selected)]
        ),
      );
    | _ =>
      let value =
        switch (model.mode, model.selected) {
        | (Restricted(Revert), [key, ..._]) when !model.multiple =>
          Option.value(~default="", label_for_key(key))
        | _ => ""
        };
      let selected =
        switch (model.mode) {
        | Restricted(Clear) when !model.multiple => []
        | _ => model.selected
        };
      (
        {
          ...next,
          text: value,
          query: value,
          selected,
        },
        cmd,
        outs
        @ changed(value, model)
        @ (selected == model.selected ? [] : [Selection_changed(selected)]),
      );
    };
  };
};
let navigate = (~items, ~forward, model) => {
  let flags = Array.map((item: item) => item.disabled, items);
  let from =
    switch (active_key(~items, model)) {
    | None => (-1)
    | Some(key) =>
      let rec index = i =>
        i >= Array.length(items)
          ? (-1) : String.equal(items[i].key, key) ? i : index(i + 1);
      index(0);
    };
  let index =
    forward
      ? Ui_nav.next(~wrap=model.wrap, ~from, flags)
      : Ui_nav.prev(~wrap=model.wrap, ~from, flags);
  {
    ...model,
    active: Option.map(i => items[i].key, index),
    completion: None,
    inline_allowed: false,
  };
};
let input = (value, ~allow_inline, model) =>
  if (model.disabled || model.readonly) {
    (model, Cmd.none, []);
  } else if (model.composing) {
    (
      {
        ...model,
        text: value,
        completion: None,
        inline_allowed: false,
      },
      Cmd.none,
      [],
    );
  } else {
    let outs =
      String.equal(model.query, value) ? [] : [Query_changed(value)];
    let model = {
      ...model,
      text: value,
      query: value,
      active: None,
      completion: None,
      inline_allowed: allow_inline,
    };
    let (model, cmd, opening) =
      model.open_on_type ? opened(model) : (model, Cmd.none, []);
    (model, cmd, outs @ opening);
  };
let update = (~items, ~label_for_key=?, message, model) => {
  let label_for_key =
    Option.value(
      ~default=
        key => Option.map((item: item) => item.label, find(~items, key)),
      label_for_key,
    );
  let model = {
    ...model,
    active: active_key(~items, model),
  };
  let (model, cmd, outs) =
    switch (message) {
    | Animated(message) =>
      let (animation, cmd, outs) =
        Animation.update(message, model.animation);
      let left = List.exists(out => out == Animation.Left, outs);
      let (anchor, acmd) =
        left ? Anchor.stop(model.anchor) : (model.anchor, Cmd.none);
      (
        {
          ...model,
          animation,
          anchor,
        },
        Cmd.batch([
          Cmd.map(m => Animated(m), cmd),
          Cmd.map(m => Anchored(m), acmd),
        ]),
        [],
      );
    | Anchored(message) =>
      let (anchor, cmd, _) = Anchor.update(message, model.anchor);
      (
        {
          ...model,
          anchor,
        },
        Cmd.map(m => Anchored(m), cmd),
        [],
      );
    | Announce(label) => (
        String.equal(label, model.pending)
          ? {
            ...model,
            announcement: label,
          }
          : model,
        Cmd.none,
        [],
      )
    | Composition_start => (
        {
          ...model,
          composing: true,
          completion: None,
          inline_allowed: false,
        },
        Cmd.none,
        [],
      )
    | Composition_end(value) =>
      input(
        value,
        ~allow_inline=false,
        {
          ...model,
          composing: false,
        },
      )
    | Input(value, composing, allow_inline) =>
      input(
        value,
        ~allow_inline,
        {
          ...model,
          composing: composing || model.composing,
        },
      )
    | Focused(generation) => (
        {
          ...model,
          focus: fst(Ui_focus.finish(model.focus, generation)),
        },
        Cmd.none,
        [],
      )
    | Blur
    | Outside => commit_blur(~items, ~label_for_key, model)
    | Remove(key) => remove(key, model)
    | Hover(key) =>
      switch (find(~items, key)) {
      | Some(item) when model.open_ && !item.disabled && !model.disabled => (
          {
            ...model,
            active: Some(key),
            completion: None,
            inline_allowed: false,
          },
          Cmd.none,
          [],
        )
      | _ => (model, Cmd.none, [])
      }
    | Choose(key) => choose(~items, key, model)
    | Toggle when !model.disabled =>
      let (model, cmd, outs) = model.open_ ? closed(model) : opened(model);
      (request_focus(model), cmd, outs);
    | Key(event) when !model.disabled && !model.composing =>
      if (event.alt && !event.ctrl && !event.meta && !event.shift) {
        switch (event.key) {
        | "ArrowDown" => opened(model)
        | "ArrowUp" => closed(model)
        | _ => (model, Cmd.none, [])
        };
      } else if (String.equal(event.key, "Tab")) {
        closed(model);
      } else if (!Ui_keyboard.plain(event)) {
        (model, Cmd.none, []);
      } else {
        switch (event.key) {
        | "ArrowDown"
        | "ArrowUp" =>
          let (model, cmd, outs) = opened(model);
          (
            navigate(
              ~items,
              ~forward=String.equal(event.key, "ArrowDown"),
              model,
            ),
            cmd,
            outs,
          );
        | "Home"
        | "End"
        | "ArrowLeft"
        | "ArrowRight" => (
            {
              ...model,
              completion: None,
              inline_allowed: false,
            },
            Cmd.none,
            [],
          )
        | "Enter" =>
          switch (active_key(~items, model)) {
          | Some(key) when model.open_ => choose(~items, key, model)
          | _ when model.mode == Free_form && !model.readonly =>
            let (model, cmd, outs) = closed(model);
            (model, cmd, [Submitted(model.text), ...outs]);
          | _ => closed(model)
          }
        | "Escape" when model.open_ => closed(model)
        | "Escape" when !model.readonly =>
          let outs = changed("", model);
          let selected = model.multiple ? model.selected : [];
          (
            {
              ...set_text("", model),
              selected,
            },
            Cmd.none,
            outs
            @ (
              selected == model.selected ? [] : [Selection_changed(selected)]
            ),
          );
        | "Backspace" when !model.readonly =>
          switch (model.completion) {
          | Some(_) => (
              {
                ...model,
                text: model.query,
                completion: None,
                inline_allowed: false,
              },
              Cmd.none,
              [],
            )
          | None when model.multiple && String.equal(model.text, "") =>
            switch (List.rev(model.selected)) {
            | [key, ..._] => remove(key, model)
            | [] => (model, Cmd.none, [])
            }
          | _ => (model, Cmd.none, [])
          }
        | _ => (model, Cmd.none, [])
        };
      }
    | _ => (model, Cmd.none, [])
    };
  /* Parents call refresh again after processing Query_changed, using the new
     filtered/remote results. No collection is retained in this model. */
  (refresh(~items, ~loading=model.loading, model), cmd, outs);
};
let frame_subscription = (~key, ~home, action) =>
  Sub.make(
    ~home,
    ~key,
    ~tagger=m => m,
    ~start=
      dispatch => {
        let live = ref(true);
        Platform.request_animation_frame(_ =>
          if (live^) {
            action(dispatch);
          }
        );
        () => live := false;
      },
  );
let identity = model => Ui_sub.prefix(model.id);
let subscriptions = model =>
  Sub.batch([
    Sub.map(m => Animated(m), Animation.subscriptions(model.animation)),
    Sub.map(m => Anchored(m), Anchor.subscriptions(model.anchor)),
    Ui_focus.sub(
      ~home="Combobox",
      ~id=model.id,
      ~prevent_scroll=false,
      model.focus,
      (generation, _) =>
      Focused(generation)
    ),
    model.open_
      ? Events.on_pointer_down_outside(
          ~ids=[part(model, "wrapper"), part(model, "popup")],
          Outside,
        )
      : Sub.none,
    !String.equal(model.pending, model.announcement)
      ? frame_subscription(
          ~home="Tea_ui.Combobox.announce",
          ~key=identity(model) ++ ":" ++ model.pending,
          dispatch =>
          dispatch(Announce(model.pending))
        )
      : Sub.none,
    switch (model.active) {
    | Some(key) when model.open_ =>
      frame_subscription(
        ~home="Tea_ui.Combobox.scroll",
        ~key=identity(model) ++ ":" ++ Ui_attr.safe_key(key),
        _ =>
        Browser.scroll_into_view(Ui_attr.item_id(model.id, key), _ => ())
      )
    | _ => Sub.none
    },
    switch (model.completion) {
    | Some(start) when model.open_ =>
      frame_subscription(
        ~home="Tea_ui.Combobox.range",
        ~key=
          identity(model) ++ ":" ++ model.text ++ ":" ++ string_of_int(start),
        _ =>
        switch (Ui_dom.find(part(model, "control"))) {
        | Some(el)
            when
              Ui_dom.is_active(el)
              && String.equal(Ui_dom.value(el), model.text) =>
          Ui_dom.set_selection_range(el, start, Js.String.length(model.text))
        | _ => ()
        }
      )
    | _ => Sub.none
    },
  ]);
type parts('parent) = {
  wrapper: list(Html.attr('parent)),
  label: list(Html.attr('parent)),
  control: list(Html.attr('parent)),
  description: list(Html.attr('parent)),
  error: list(Html.attr('parent)),
  popup: list(Html.attr('parent)),
  toggle: list(Html.attr('parent)),
  option_: string => list(Html.attr('parent)),
  chip: string => list(Html.attr('parent)),
  remove: (~key: string, ~label: string) => list(Html.attr('parent)),
  loading: list(Html.attr('parent)),
  empty: list(Html.attr('parent)),
  live: list(Html.attr('parent)),
  announcement: string,
  active_key: option(string),
  selected: list(string),
  is_open: bool,
  should_render: bool,
};
let no_blur =
  Html.on(
    "pointerdown",
    V.Custom_optional(
      D.succeed({
        V.message: None,
        prevent_default: true,
        stop_propagation: false,
      }),
    ),
  );
let view =
    (
      ~model,
      ~items,
      ~to_parent,
      ~has_description=false,
      ~has_error=false,
      ~invalid=false,
      ~required=false,
      ~name=?,
      ~rendered_keys=?,
      ~on_key_down=_ => None,
      ~to_view,
      (),
    ) => {
  let active = active_key(~items, model);
  let rendered = key =>
    Option.fold(
      ~none=true,
      ~some=keys => List.exists(String.equal(key), keys),
      rendered_keys,
    );
  let map = facts => List.map(V.map_fact(to_parent), facts);
  let key = (event: Ui_keyboard.event) =>
    if (model.disabled || model.composing) {
      None;
    } else {
      let intent = Some(to_parent(Key(event)));
      if (event.alt
          && !event.ctrl
          && !event.meta
          && !event.shift
          && (event.key == "ArrowDown" || event.key == "ArrowUp")) {
        Some((intent, true));
      } else if (event.key == "Tab") {
        Some((intent, false));
      } else if (!Ui_keyboard.plain(event)) {
        None;
      } else {
        switch (event.key) {
        | "ArrowDown"
        | "ArrowUp" => Some((intent, true))
        | "Escape" =>
          model.open_
          || !model.readonly
          && (model.text != "" || !model.multiple && model.selected != [])
            ? Some((intent, true)) : None
        | "Enter" => Some((intent, model.open_))
        | "Home"
        | "End"
        | "ArrowLeft"
        | "ArrowRight" => Some((intent, false))
        | "Backspace" =>
          Some((
            intent,
            !model.readonly
            && (
              model.completion != None
              || model.multiple
              && model.text == ""
              && model.selected != []
            ),
          ))
        | key => Option.map(m => (Some(m), false), on_key_down(key))
        };
      };
    };
  let input_decoder =
    D.map3(
      (value, composing, input_type) =>
        Input(
          value,
          composing,
          !String.starts_with(~prefix="delete", input_type),
        ),
      D.at(["target", "value"], D.string),
      D.one_of([D.field("isComposing", D.bool), D.succeed(false)]),
      D.one_of([D.field("inputType", D.string), D.succeed("")]),
    );
  let base =
    Input.parts(
      ~id=model.id,
      ~value=model.text,
      ~on_input=value => Input(value, false, true),
      ~on_blur=Blur,
      ~has_description,
      ~has_error,
      ~invalid,
      ~required,
      ~disabled=model.disabled,
      ~readonly=model.readonly,
      ~name?,
      ~autocomplete="off",
      (),
    );
  let control =
    List.filter(
      fact =>
        switch (fact) {
        | V.Event("input", _) => false
        | _ => true
        },
      base.control,
    );
  let popup =
    List.filter(
      fact =>
        switch (fact) {
        | V.Property("hidden", _) => false
        | _ => true
        },
      Anchor.attrs(model.anchor),
    )
    @ Animation.attrs(model.animation);
  to_view({
    wrapper: [
      Html.id(part(model, "wrapper")),
      Ui_attr.data_flag("open", model.open_),
    ],
    label: map(base.label),
    control:
      map(
        control
        @ [
          Html.role("combobox"),
          Ui_attr.aria_bool("expanded", model.open_),
          Ui_attr.aria_ids(
            "controls",
            should_render(model) ? [part(model, "popup")] : [],
          ),
          Ui_attr.aria_ids(
            "activedescendant",
            model.open_ && should_render(model)
              ? switch (active) {
                | Some(key) when rendered(key) => [
                    Ui_attr.item_id(model.id, key),
                  ]
                | _ => []
                }
              : [],
          ),
          Html.aria(
            "autocomplete",
            switch (model.autocomplete) {
            | List => "list"
            | Both => "both"
            | None_ => "none"
            },
          ),
          Ui_attr.data_flag("open", model.open_),
          model.disabled || model.readonly
            ? Html.none : Html.on("input", V.Normal(input_decoder)),
          Html.on(
            "compositionstart",
            V.Normal(D.succeed(Composition_start)),
          ),
          Html.on(
            "compositionend",
            V.Normal(
              D.map(
                value => Composition_end(value),
                D.at(["target", "value"], D.string),
              ),
            ),
          ),
        ],
      )
      @ [
        Html.on(
          "keydown",
          V.Custom_optional(
            D.map2(
              (event: Ui_keyboard.event, custom: V.custom(option('parent))) =>
                {
                  ...custom,
                  V.stop_propagation:
                    event.key == "Escape" && custom.prevent_default,
                },
              Ui_keyboard.decoder,
              Ui_keyboard.custom(key),
            ),
          ),
        ),
      ],
    description: map(base.description),
    error: map(base.error),
    popup:
      popup
      @ [
        Html.role("listbox"),
        Html.aria("labelledby", part(model, "label")),
        model.multiple
          ? Ui_attr.aria_bool("multiselectable", true) : Html.none,
        Ui_attr.aria_bool("busy", model.loading),
      ],
    toggle:
      map([
        Html.id(part(model, "toggle")),
        Html.type_("button"),
        Html.tabindex(-1),
        Html.disabled(model.disabled),
        Html.aria("label", "Show suggestions"),
        Ui_attr.aria_bool("expanded", model.open_),
        Ui_attr.aria_ids(
          "controls",
          should_render(model) ? [part(model, "popup")] : [],
        ),
        no_blur,
        Html.on_click(Toggle),
      ]),
    option_: key =>
      switch (find(~items, key)) {
      | None => []
      | Some(item) =>
        let chosen = List.exists(String.equal(key), model.selected);
        let active = active == Some(key);
        map([
          Html.id(Ui_attr.item_id(model.id, key)),
          Html.role("option"),
          Ui_attr.aria_bool("selected", chosen),
          Ui_attr.aria_bool("disabled", item.disabled),
          Ui_attr.data_flag("selected", chosen),
          Ui_attr.data_flag("active", active),
          Ui_attr.data_flag("disabled", item.disabled),
          no_blur,
          Html.on("pointermove", V.Normal(D.succeed(Hover(key)))),
          Html.on_click(Choose(key)),
        ]);
      },
    chip: key => [
      Html.id(part(model, "chip-" ++ Ui_attr.safe_key(key))),
      Ui_attr.data_flag("selected", true),
    ],
    remove: (~key, ~label) =>
      map([
        Html.type_("button"),
        Html.tabindex(-1),
        Html.aria("label", "Remove " ++ label),
        Html.disabled(model.disabled || model.readonly),
        no_blur,
        Html.on_click(Remove(key)),
      ]),
    loading: [Html.hidden(!model.open_ || !model.loading)],
    empty: [
      Html.hidden(!model.open_ || model.loading || Array.length(items) != 0),
    ],
    live: [
      Html.id(part(model, "live")),
      Html.role("status"),
      Html.aria("live", "polite"),
      Html.aria("atomic", "true"),
    ],
    announcement: model.announcement,
    active_key: active,
    selected: model.selected,
    is_open: model.open_,
    should_render: should_render(model),
  });
};
