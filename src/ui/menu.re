module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module D = Tea_core.Decode;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
/* Tab's native default runs before an after-render Browser_dom task, so
   focus that must set the sequential-navigation origin runs synchronously
   from update's command. */
let focus_now = id => Cmd.effect(_ => Ui_dom.focus_id(id));
type direction =
  | Ltr
  | Rtl;
type item = {
  key: string,
  label: string,
  disabled: bool,
  kind,
}
and kind =
  | Action
  | Checkbox
  | Radio(string)
  | Submenu(array(item))
  | Group(array(item))
  | Separator;
type close_reason =
  | Escape
  | Tab
  | Outside
  | Trigger
  | Activation;
type panel = {
  owner: option(string),
  parent: option(string),
  active: option(string),
  animation: Animation.model,
  anchor: Anchor.model,
  open_: bool,
  search: Ui_typeahead.t,
  focus_pending: bool,
};
type hover = {
  hover_owner: option(string),
  key: string,
};
type model = {
  id: string,
  dir: direction,
  top_layer: bool,
  hover_delay: float,
  panels: list(panel),
  checks: list(string),
  radios: list((string, string)),
  selected: option(string),
  hover: option(hover),
  restore: bool,
  focus_epoch: int,
};
type msg =
  | Trigger_click
  | Trigger_key(Ui_keyboard.event)
  | Menu_key(option(string), Ui_keyboard.event)
  | Activate(option(string), string, bool)
  | Move(option(string), string)
  | Enter(option(string))
  | Leave(option(string))
  | Hover(hover)
  | Outside_down
  | Focused(option(string), int)
  | Animated(option(string), Animation.msg)
  | Anchored(option(string), Anchor.msg);
type out_msg =
  | Selected(string)
  | Checked(string, bool)
  | Radio_selected(string, string)
  | Opened
  | Closed(close_reason);
let equal_owner = Option.equal(String.equal);
let trigger_id = m => Ui_attr.part_id(m.id, "button");
let item_id = (m, key) => Ui_attr.item_id(m.id, key);
let panel_id = (m, owner) =>
  switch (owner) {
  | None => Ui_attr.part_id(m.id, "menu")
  | Some(key) => Ui_attr.item_id(Ui_attr.part_id(m.id, "submenu"), key)
  };
let make_panel = (m, owner, parent) => {
  let id = panel_id(m, owner);
  {
    owner,
    parent,
    active: None,
    animation: Animation.init(~id, ~open_=false, ()),
    anchor:
      Anchor.init(
        ~anchor_id=
          switch (owner) {
          | None => trigger_id(m)
          | Some(key) => item_id(m, key)
          },
        ~floating_id=id,
        ~top_layer=m.top_layer,
        ~dir=
          switch (m.dir) {
          | Ltr => Anchor.Ltr
          | Rtl => Anchor.Rtl
          },
        ~placement=
          switch (owner) {
          | None => (Anchor.Bottom, Anchor.Start)
          | Some(_) => (
              m.dir == Ltr ? Anchor.Right : Anchor.Left,
              Anchor.Start,
            )
          },
        ~offset=4.,
        (),
      ),
    open_: false,
    search: Ui_typeahead.empty,
    focus_pending: false,
  };
};
let init =
    (
      ~id,
      ~dir=Ltr,
      ~top_layer=true,
      ~hover_delay=220.,
      ~checked=[],
      ~radio=[],
      (),
    ) => {
  let hover_delay =
    switch (classify_float(hover_delay)) {
    | FP_nan
    | FP_infinite => 220.
    | _ => max(0., min(2147483647., hover_delay))
    };
  let m = {
    id,
    dir,
    top_layer,
    hover_delay,
    panels: [],
    checks: checked,
    radios: radio,
    selected: None,
    hover: None,
    restore: false,
    focus_epoch: 0,
  };
  {
    ...m,
    panels: [make_panel(m, None, None)],
  };
};
let find_panel = (m, owner) =>
  List.find_opt(p => equal_owner(p.owner, owner), m.panels);
let get_panel = (m, owner) =>
  Option.value(~default=make_panel(m, owner, None), find_panel(m, owner));
let put_panel = (m, panel) => {
  let found = List.exists(p => equal_owner(p.owner, panel.owner), m.panels);
  {
    ...m,
    panels:
      found
        ? List.map(
            p => equal_owner(p.owner, panel.owner) ? panel : p,
            m.panels,
          )
        : m.panels @ [panel],
  };
};
let is_open = m => get_panel(m, None).open_;
let phase = (~submenu=?, m) =>
  Animation.phase(get_panel(m, submenu).animation);
let active_key = (~submenu=?, m) => get_panel(m, submenu).active;
let selected = m => m.selected;
let set_selected = (selected, m) => {
  ...m,
  selected,
};
let checked = (~key, m) => List.exists(String.equal(key), m.checks);
let set_checked = (~key, value, m) => {
  let checks = List.filter(k => !String.equal(k, key), m.checks);
  {
    ...m,
    checks: value ? [key, ...checks] : checks,
  };
};
let radio_value = (~group, m) =>
  Option.map(
    ((_, key)) => key,
    List.find_opt(((g, _)) => String.equal(g, group), m.radios),
  );
let set_radio = (~group, ~key, m) => {
  let radios = List.filter(((g, _)) => !String.equal(g, group), m.radios);
  {
    ...m,
    radios:
      switch (key) {
      | None => radios
      | Some(key) => [(group, key), ...radios]
      },
  };
};
/* Groups participate in their containing menu; submenus never do. */
let rec navigable = items =>
  Array.of_list(
    List.concat_map(
      (i: item) =>
        switch (i.kind) {
        | Group(children) => Array.to_list(navigable(children))
        | Separator => []
        | _ => [i]
        },
      Array.to_list(items),
    ),
  );
let rec locate = (items: array(item), key, owner) => {
  let rec loop = index =>
    index >= Array.length(items)
      ? None
      : {
        let i = items[index];
        if (String.equal(i.key, key)) {
          Some((i, owner));
        } else {
          let nested =
            switch (i.kind) {
            | Group(children) => locate(children, key, owner)
            | Submenu(children) => locate(children, key, Some(i.key))
            | _ => None
            };
          switch (nested) {
          | Some(_) => nested
          | None => loop(index + 1)
          };
        };
      };
  loop(0);
};
let panel_items = (items, owner) =>
  switch (owner) {
  | None => Some(items)
  | Some(key) =>
    switch (locate(items, key, None)) {
    | Some(({ kind: Submenu(children), _ }, _)) => Some(children)
    | _ => None
    }
  };
let index_of = (items: array(item), key) => {
  let rec loop = n =>
    n >= Array.length(items)
      ? (-1) : equal_owner(Some(items[n].key), key) ? n : loop(n + 1);
  loop(0);
};
let all_flags = items => Array.make(Array.length(items), false);
let key_at = (items: array(item), index) =>
  Option.map(i => items[i].key, index);
let rec belongs = (m, owner, ancestor) =>
  equal_owner(owner, ancestor)
  || (
    switch (owner) {
    | None => false
    | Some(_) =>
      switch (find_panel(m, owner)) {
      | None => false
      | Some(p) => belongs(m, p.parent, ancestor)
      }
    }
  );
let hide_branch = (m, ancestor) => {
  let commands =
    List.map(
      p =>
        if (belongs(m, p.owner, ancestor)) {
          let (animation, cmd) = Animation.hide(p.animation);
          (
            {
              ...p,
              animation,
              open_: false,
              active: None,
              search: Ui_typeahead.empty,
              focus_pending: false,
            },
            Cmd.map(msg => Animated(p.owner, msg), cmd),
          );
        } else {
          (p, Cmd.none);
        },
      m.panels,
    );
  (
    {
      ...m,
      panels: List.map(fst, commands),
      hover: None,
    },
    Cmd.batch(List.map(snd, commands)),
  );
};
let close_tree = (m, restore) => {
  let (m, cmd) = hide_branch(m, None);
  (
    {
      ...m,
      restore,
    },
    Cmd.batch([cmd, restore ? focus_now(trigger_id(m)) : Cmd.none]),
  );
};
let close = m => close_tree(m, is_open(m));
let focus_panel = (m, p) => {
  let p = {
    ...p,
    focus_pending: true,
  };
  let m = {
    ...m,
    focus_epoch: m.focus_epoch + 1,
    panels:
      List.map(
        p =>
          {
            ...p,
            focus_pending: false,
          },
        m.panels,
      ),
  };
  (put_panel(m, p), Cmd.none);
};
let show_panel = (~items, m, p, last) => {
  let candidates = navigable(items);
  let flags = all_flags(candidates);
  let enabled = Array.map((i: item) => i.disabled, candidates);
  let first =
    switch (Ui_nav.first(enabled)) {
    | Some(i) => Some(i)
    | None => Ui_nav.first(flags)
    };
  let active = key_at(candidates, last ? Ui_nav.last(flags) : first);
  let (anchor, a) = Anchor.start(p.anchor);
  let (animation, b) = Animation.show(p.animation);
  let p = {
    ...p,
    anchor,
    animation,
    active,
    open_: true,
    search: Ui_typeahead.empty,
  };
  let (m, f) =
    focus_panel(
      {
        ...m,
        hover: None,
        restore: false,
      },
      p,
    );
  (
    m,
    Cmd.batch([
      Cmd.map(msg => Anchored(p.owner, msg), a),
      Cmd.map(msg => Animated(p.owner, msg), b),
      f,
    ]),
  );
};
let open_ = (~items, m) =>
  is_open(m)
    ? (m, Cmd.none) : show_panel(~items, m, get_panel(m, None), false);
let open_submenu = (~items, m, parent, key) =>
  switch (locate(items, key, None)) {
  | Some(({ disabled: false, kind: Submenu(children), _ }, owner))
      when equal_owner(owner, parent) =>
    let siblings =
      List.filter(
        p =>
          !equal_owner(p.owner, Some(key))
          && equal_owner(p.parent, parent)
          && Option.is_some(p.owner)
          && p.open_,
        m.panels,
      );
    let (m, commands) =
      List.fold_left(
        ((m, commands), p) => {
          let (m, cmd) = hide_branch(m, p.owner);
          (m, [cmd, ...commands]);
        },
        (m, []),
        siblings,
      );
    let p =
      switch (find_panel(m, Some(key))) {
      | Some(p) => p
      | None => make_panel(m, Some(key), parent)
      };
    let parent_panel = get_panel(m, parent);
    let m =
      put_panel(
        m,
        {
          ...parent_panel,
          active: Some(key),
        },
      );
    let (m, cmd) = show_panel(~items=children, m, p, false);
    (m, Cmd.batch([cmd, ...commands]));
  | _ => (m, Cmd.none)
  };
let repair = (~items, m) => {
  let (m, commands) =
    List.fold_left(
      ((m, commands), p) => {
        let valid =
          switch (p.owner) {
          | None => true
          | Some(key) =>
            switch (locate(items, key, None)) {
            | Some(({ disabled: false, kind: Submenu(_), _ }, parent)) =>
              equal_owner(parent, p.parent) && get_panel(m, parent).open_
            | _ => false
            }
          };
        if (p.open_ && !valid) {
          let (m, cmd) = hide_branch(m, p.owner);
          (m, [cmd, ...commands]);
        } else {
          let p = get_panel(m, p.owner);
          let candidates =
            navigable(
              Option.value(~default=[||], panel_items(items, p.owner)),
            );
          let active =
            switch (p.active) {
            | None => None
            | Some(_) =>
              key_at(
                candidates,
                Ui_nav.repair(
                  ~from=index_of(candidates, p.active),
                  all_flags(candidates),
                ),
              )
            };
          (
            put_panel(
              m,
              {
                ...p,
                active,
              },
            ),
            commands,
          );
        };
      },
      (m, []),
      m.panels,
    );
  (m, Cmd.batch(commands));
};
let close_user = (m, reason, restore) => {
  let was_open = is_open(m);
  let (m, cmd) = close_tree(m, restore);
  (m, cmd, was_open ? [Closed(reason)] : []);
};
let close_submenu = (m, owner) =>
  switch (find_panel(m, owner)) {
  | Some(p) when Option.is_some(owner) =>
    let parent = get_panel(m, p.parent);
    let (m, cmd) = hide_branch(m, owner);
    (
      put_panel(
        m,
        {
          ...parent,
          active: owner,
        },
      ),
      Cmd.batch([cmd, focus_now(panel_id(m, p.parent))]),
      [],
    );
  | _ => (m, Cmd.none, [])
  };
let activate = (~items, m, owner, key, keep_toggle) => {
  let p = get_panel(m, owner);
  switch (locate(items, key, None)) {
  | Some((i, found_owner))
      when p.open_ && equal_owner(owner, found_owner) && !i.disabled =>
    let (m, out, toggle) =
      switch (i.kind) {
      | Action => (
          {
            ...m,
            selected: Some(key),
          },
          Some(Selected(key)),
          false,
        )
      | Checkbox =>
        let value = !checked(~key, m);
        (set_checked(~key, value, m), Some(Checked(key, value)), true);
      | Radio(group) => (
          set_radio(~group, ~key=Some(key), m),
          Some(Radio_selected(group, key)),
          true,
        )
      | _ => (m, None, false)
      };
    switch (i.kind, out) {
    | (Submenu(_), _) =>
      let (m, cmd) = open_submenu(~items, m, owner, key);
      (m, cmd, []);
    | (_, Some(out)) when toggle && keep_toggle => (m, Cmd.none, [out])
    | (_, Some(out)) =>
      let (m, cmd, closed) = close_user(m, Activation, true);
      (m, cmd, [out, ...closed]);
    | _ => (m, Cmd.none, [])
    };
  | _ => (m, Cmd.none, [])
  };
};
let scroll_active = (m, active) =>
  switch (active) {
  | None => Cmd.none
  | Some(key) =>
    Cmd.effect(_ => Browser.scroll_into_view(item_id(m, key), _ => ()))
  };
let menu_key = (~items, m, owner, event: Ui_keyboard.event) => {
  let p = get_panel(m, owner);
  if (!p.open_ || !Ui_keyboard.plain(event) || !Ui_keyboard.available(event)) {
    (m, Cmd.none, []);
  } else {
    let candidates =
      navigable(Option.value(~default=[||], panel_items(items, owner)));
    let flags = all_flags(candidates);
    let from = index_of(candidates, p.active);
    let move = active => {
      let (m, cmd) =
        List.fold_left(
          ((m, commands), child) =>
            if (equal_owner(child.parent, owner)
                && Option.is_some(child.owner)
                && child.open_) {
              let (m, cmd) = hide_branch(m, child.owner);
              (m, [cmd, ...commands]);
            } else {
              (m, commands);
            },
          (m, []),
          m.panels,
        );
      (
        put_panel(
          {
            ...m,
            hover: None,
          },
          {
            ...p,
            active,
            search: Ui_typeahead.empty,
          },
        ),
        Cmd.batch([scroll_active(m, active), ...cmd]),
        [],
      );
    };
    switch (event.key) {
    | "Tab" =>
      /* This command must run before the browser's unprevented Tab default. */
      let (m, cmd, outs) = close_user(m, Tab, false);
      (m, Cmd.batch([cmd, focus_now(trigger_id(m))]), outs);
    | "Escape" =>
      Option.is_some(owner)
        ? close_submenu(m, owner) : close_user(m, Escape, true)
    | "ArrowDown" => move(key_at(candidates, Ui_nav.next(~from, flags)))
    | "ArrowUp" => move(key_at(candidates, Ui_nav.prev(~from, flags)))
    | "Home" => move(key_at(candidates, Ui_nav.first(flags)))
    | "End" => move(key_at(candidates, Ui_nav.last(flags)))
    | "Enter"
    | " " =>
      switch (p.active) {
      | None => (m, Cmd.none, [])
      | Some(key) =>
        activate(~items, m, owner, key, String.equal(event.key, " "))
      }
    | key
        when
          Ui_nav.horizontal(~dir=m.dir == Ltr ? Ui_nav.Ltr : Ui_nav.Rtl, key)
          == Some(1) =>
      switch (p.active) {
      | None => (m, Cmd.none, [])
      | Some(key) =>
        let (m, cmd) = open_submenu(~items, m, owner, key);
        (m, cmd, []);
      }
    | key
        when
          Ui_nav.horizontal(~dir=m.dir == Ltr ? Ui_nav.Ltr : Ui_nav.Rtl, key)
          == Some(-1) =>
      close_submenu(m, owner)
    | key =>
      let (search, index) =
        Ui_typeahead.feed(
          ~time_stamp=event.time_stamp,
          ~key,
          ~from,
          ~labels=Array.map((i: item) => i.label, candidates),
          ~disabled=flags,
          p.search,
        );
      switch (index) {
      | None => (
          put_panel(
            m,
            {
              ...p,
              search,
            },
          ),
          Cmd.none,
          [],
        )
      | Some(_) =>
        let (m, cmd, outs) = move(key_at(candidates, index));
        let p = get_panel(m, owner);
        (
          put_panel(
            m,
            {
              ...p,
              search,
            },
          ),
          cmd,
          outs,
        );
      };
    };
  };
};
let update = (~items, message, m) => {
  let (m, repair_cmd) = repair(~items, m);
  let (m, cmd, outs) =
    switch (message) {
    | Trigger_click =>
      if (is_open(m)) {
        close_user(m, Trigger, true);
      } else {
        let (m, cmd) = open_(~items, m);
        (m, cmd, [Opened]);
      }
    | Trigger_key(event) =>
      if (!Ui_keyboard.plain(event) || !Ui_keyboard.available(event)) {
        (m, Cmd.none, []);
      } else {
        let was_open = is_open(m);
        let (m, cmd) =
          show_panel(
            ~items,
            m,
            get_panel(m, None),
            String.equal(event.key, "ArrowUp"),
          );
        (m, cmd, was_open ? [] : [Opened]);
      }
    | Menu_key(owner, event) => menu_key(~items, m, owner, event)
    | Activate(owner, key, keep) => activate(~items, m, owner, key, keep)
    | Outside_down => close_user(m, Outside, false)
    | Focused(owner, epoch) when epoch == m.focus_epoch =>
      let p = get_panel(m, owner);
      (
        put_panel(
          m,
          {
            ...p,
            focus_pending: false,
          },
        ),
        Cmd.none,
        [],
      );
    | Focused(_, _) => (m, Cmd.none, [])
    | Move(owner, key) =>
      let p = get_panel(m, owner);
      let candidates =
        navigable(Option.value(~default=[||], panel_items(items, owner)));
      if (!p.open_ || index_of(candidates, Some(key)) < 0) {
        (m, Cmd.none, []);
      } else {
        let hover =
          if (equal_owner(p.active, Some(key))
              && (Option.is_some(m.hover) || get_panel(m, Some(key)).open_)) {
            m.hover;
          } else {
            Some({
              hover_owner: owner,
              key,
            });
          };
        let (m, cmd) =
          focus_panel(
            {
              ...m,
              hover,
            },
            {
              ...p,
              active: Some(key),
              search: Ui_typeahead.empty,
            },
          );
        (m, cmd, []);
      };
    | Enter(owner) =>
      let hover =
        switch (m.hover) {
        | Some(h) when !equal_owner(h.hover_owner, owner) => None
        | h => h
        };
      (
        {
          ...m,
          hover,
        },
        Cmd.none,
        [],
      );
    | Leave(owner) =>
      let has_child =
        List.exists(
          p =>
            p.open_
            && Option.is_some(p.owner)
            && equal_owner(p.parent, owner),
          m.panels,
        );
      let p = get_panel(m, owner);
      let hover =
        switch (m.hover) {
        | Some(h) when equal_owner(h.hover_owner, owner) => None
        | h => h
        };
      (
        put_panel(
          {
            ...m,
            hover,
          },
          has_child
            ? p
            : {
              ...p,
              active: None,
            },
        ),
        Cmd.none,
        [],
      );
    | Hover(h) =>
      switch (m.hover) {
      | Some(current)
          when
            equal_owner(current.hover_owner, h.hover_owner)
            && String.equal(current.key, h.key)
            && get_panel(m, h.hover_owner).open_ =>
        let siblings =
          List.filter(
            p =>
              Option.is_some(p.owner)
              && p.open_
              && equal_owner(p.parent, h.hover_owner)
              && !equal_owner(p.owner, Some(h.key)),
            m.panels,
          );
        let (m, commands) =
          List.fold_left(
            ((m, commands), p) => {
              let (m, cmd) = hide_branch(m, p.owner);
              (m, [cmd, ...commands]);
            },
            (
              {
                ...m,
                hover: None,
              },
              [],
            ),
            siblings,
          );
        let (m, cmd) = open_submenu(~items, m, h.hover_owner, h.key);
        (m, Cmd.batch([cmd, ...commands]), []);
      | _ => (m, Cmd.none, [])
      }
    | Animated(owner, msg) =>
      switch (find_panel(m, owner)) {
      | None => (m, Cmd.none, [])
      | Some(p) =>
        let (animation, cmd, outs) = Animation.update(msg, p.animation);
        let left =
          List.exists(
            out =>
              switch (out) {
              | Animation.Left => true
              | _ => false
              },
            outs,
          );
        let (anchor, stop) =
          left ? Anchor.stop(p.anchor) : (p.anchor, Cmd.none);
        let m =
          put_panel(
            m,
            {
              ...p,
              animation,
              anchor,
            },
          );
        let m =
          left && Option.is_some(owner)
            ? {
              ...m,
              panels:
                List.filter(p => !equal_owner(p.owner, owner), m.panels),
            }
            : left
                ? {
                  ...m,
                  restore: false,
                }
                : m;
        (
          m,
          Cmd.batch([
            Cmd.map(msg => Animated(owner, msg), cmd),
            Cmd.map(msg => Anchored(owner, msg), stop),
          ]),
          [],
        );
      }
    | Anchored(owner, msg) =>
      switch (find_panel(m, owner)) {
      | None => (m, Cmd.none, [])
      | Some(p) =>
        let (anchor, cmd, _) = Anchor.update(msg, p.anchor);
        let p = {
          ...p,
          anchor,
        };
        let m = put_panel(m, p);
        (m, Cmd.map(msg => Anchored(owner, msg), cmd), []);
      }
    };
  (m, Cmd.batch([repair_cmd, cmd]), outs);
};
let subscriptions = m =>
  Sub.batch([
    /* This lifetime-only guard never runs on outside close. It restores only
       when shutdown/removal actually owns focus, including nested popovers. */
    Sub.make(
      ~home="Tea_ui.Menu.unmount",
      ~key=m.id,
      ~tagger=() => Outside_down,
      ~start=
        (_, ()) => {
          let tree =
            Option.bind(Ui_dom.active(), el =>
              Ui_dom.closest(el, "[data-menu-tree]")
            );
          switch (
            Option.bind(tree, el => Ui_dom.attribute(el, "data-menu-tree"))
          ) {
          | Some(id) when String.equal(id, m.id) =>
            Ui_dom.focus_id(trigger_id(m))
          | _ => ()
          };
        },
    ),
    Sub.batch(
      List.map(
        p =>
          Sub.batch([
            Sub.map(
              msg => Animated(p.owner, msg),
              Animation.subscriptions(p.animation),
            ),
            Sub.map(
              msg => Anchored(p.owner, msg),
              Anchor.subscriptions(p.anchor),
            ),
            p.open_ && p.focus_pending
              ? Events.focus(
                  ~key=
                    panel_id(m, p.owner)
                    ++ ":"
                    ++ string_of_int(m.focus_epoch),
                  ~target=Events.Element(panel_id(m, p.owner)),
                  ~ready=
                    Option.fold(
                      ~none=false,
                      ~some=position => !position.Anchor.reference_hidden,
                      Anchor.position(p.anchor),
                    ),
                  _ =>
                  Focused(p.owner, m.focus_epoch)
                )
              : Sub.none,
          ]),
        m.panels,
      ),
    ),
    is_open(m)
      ? Events.on_pointer_down_outside(
          ~ids=[
            trigger_id(m),
            ...List.map(
                 p => panel_id(m, p.owner),
                 List.filter(p => Anchor.active(p.anchor), m.panels),
               ),
          ],
          Outside_down,
        )
      : Sub.none,
    /* Escape/activation already focus synchronously. A short restoration lease
       across their leave protects unmount without making an outside close steal
       focus. Browser_events has no cancellation mode for an open-time lease. */
    m.restore && Animation.should_render(get_panel(m, None).animation)
      ? Events.restore_focus(
          ~key=Ui_attr.part_id(m.id, "restore"),
          ~within=panel_id(m, None),
          (),
        )
      : Sub.none,
    switch (m.hover) {
    | None => Sub.none
    | Some(h) =>
      Sub.make(
        ~home="Tea_ui.Menu.hover",
        ~key=panel_id(m, h.hover_owner) ++ ":" ++ Ui_attr.safe_key(h.key),
        ~tagger=() => Hover(h),
        ~start=
          dispatch => {
            let timer =
              Ui_dom.timeout(() => dispatch(), int_of_float(m.hover_delay));
            () => Ui_dom.clear_timeout(timer);
          },
      )
    },
  ]);
type item_parts('parent) = {
  item,
  attrs: list(Html.attr('parent)),
  label: list(Html.attr('parent)),
  children: array(item_parts('parent)),
  active: bool,
  checked: bool,
};
type panel_parts('parent) = {
  owner: option(string),
  attrs: list(Html.attr('parent)),
  items: array(item_parts('parent)),
  open_: bool,
  phase: Animation.phase,
};
type parts('parent) = {
  trigger: list(Html.attr('parent)),
  panels: array(panel_parts('parent)),
};
let event = (name, msg) => Html.on(name, V.Normal(D.succeed(msg)));
let prevent_pointer_focus = () =>
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
let key_handler = (intent, hook, trigger, open_, to_parent) =>
  Html.on(
    "keydown",
    V.Custom_optional(
      D.map2(
        (e: Ui_keyboard.event, handled) =>
          {
            ...handled,
            V.stop_propagation:
              String.equal(e.key, "Escape") && handled.V.prevent_default,
          },
        Ui_keyboard.decoder,
        Ui_keyboard.custom(e =>
          if (!Ui_keyboard.plain(e)) {
            None;
          } else {
            let handled =
              String.equal(e.key, "Escape")
              || (trigger || open_)
              && (
                trigger
                  ? List.mem(e.key, ["Enter", " ", "ArrowDown", "ArrowUp"])
                  : List.mem(
                      e.key,
                      [
                        "Enter",
                        " ",
                        "ArrowDown",
                        "ArrowUp",
                        "Home",
                        "End",
                        "ArrowLeft",
                        "ArrowRight",
                        "Escape",
                        "Tab",
                      ],
                    )
                    || Ui_typeahead.character(e.key)
              );
            if (handled) {
              Some((
                Some(to_parent(intent(e))),
                !String.equal(e.key, "Tab")
                && (!String.equal(e.key, "Escape") || open_),
              ));
            } else {
              Option.map(msg => (Some(msg), false), hook(e.key));
            };
          }
        ),
      ),
    ),
  );
let view =
    (
      ~model as m,
      ~items,
      ~to_parent,
      ~on_trigger_key_down=_ => None,
      ~on_menu_key_down=_ => None,
      ~to_view,
      (),
    ) => {
  let rec entries = (p: panel, items) =>
    Array.map(
      (i: item) => {
        let active = p.open_ && equal_owner(p.active, Some(i.key));
        let checked =
          switch (i.kind) {
          | Checkbox => checked(~key=i.key, m)
          | Radio(group) =>
            equal_owner(radio_value(~group, m), Some(i.key))
          | _ => false
          };
        let id = item_id(m, i.key);
        let role =
          switch (i.kind) {
          | Checkbox => "menuitemcheckbox"
          | Radio(_) => "menuitemradio"
          | Group(_) => "group"
          | Separator => "separator"
          | _ => "menuitem"
          };
        let actionable =
          switch (i.kind) {
          | Group(_)
          | Separator => false
          | _ => true
          };
        let submenu =
          switch (i.kind) {
          | Submenu(_) => true
          | _ => false
          };
        let sub_open = submenu && get_panel(m, Some(i.key)).open_;
        {
          item: i,
          active,
          checked,
          attrs:
            [
              Html.id(id),
              Html.role(role),
              actionable ? Html.tabindex(-1) : Html.none,
              i.disabled && actionable
                ? Ui_attr.aria_bool("disabled", true) : Html.none,
              Ui_attr.data_flag("disabled", i.disabled),
              Ui_attr.data_flag("active", active),
              Ui_attr.data_flag("checked", checked),
              Ui_attr.data_flag("open", sub_open),
              switch (i.kind) {
              | Checkbox
              | Radio(_) => Ui_attr.aria_bool("checked", checked)
              | _ => Html.none
              },
              switch (i.kind) {
              | Group(_) =>
                Html.aria(
                  "labelledby",
                  Ui_attr.item_id(Ui_attr.part_id(m.id, "label"), i.key),
                )
              | _ => Html.none
              },
              submenu ? Html.aria("haspopup", "menu") : Html.none,
              submenu ? Ui_attr.aria_bool("expanded", sub_open) : Html.none,
              submenu
                ? Html.aria("controls", panel_id(m, Some(i.key)))
                : Html.none,
            ]
            @ (
              actionable
                ? [
                  event("pointermove", to_parent(Move(p.owner, i.key))),
                  prevent_pointer_focus(),
                  Html.on(
                    "click",
                    V.Custom_optional(
                      D.succeed({
                        V.message:
                          Some(to_parent(Activate(p.owner, i.key, false))),
                        prevent_default: true,
                        stop_propagation: true,
                      }),
                    ),
                  ),
                ]
                : []
            ),
          label:
            switch (i.kind) {
            | Group(_) => [
                Html.id(
                  Ui_attr.item_id(Ui_attr.part_id(m.id, "label"), i.key),
                ),
              ]
            | _ => []
            },
          children:
            switch (i.kind) {
            | Group(children) => entries(p, children)
            | _ => [||]
            },
        };
      },
      items,
    );
  let make = (owner, items) => {
    let p = get_panel(m, owner);
    let candidates = navigable(items);
    let active = index_of(candidates, p.active) >= 0 ? p.active : None;
    {
      owner,
      open_: p.open_,
      phase: Animation.phase(p.animation),
      /* Animation alone owns hidden. Anchor alone owns id. */
      attrs:
        Ui_attr.without("hidden", Anchor.attrs(p.anchor))
        @ Animation.attrs(p.animation)
        @ [
          Html.role("menu"),
          Html.data("menu-tree", m.id),
          Html.tabindex(-1),
          Html.attribute("dir", m.dir == Ltr ? "ltr" : "rtl"),
          Html.aria(
            "labelledby",
            switch (owner) {
            | None => trigger_id(m)
            | Some(key) => item_id(m, key)
            },
          ),
          Ui_attr.aria_ids(
            "activedescendant",
            p.open_
              ? List.map(key => item_id(m, key), Option.to_list(active))
              : [],
          ),
          Ui_attr.data_flag("open", p.open_),
          key_handler(
            e => Menu_key(owner, e),
            on_menu_key_down,
            false,
            p.open_,
            to_parent,
          ),
          event("pointerenter", to_parent(Enter(owner))),
          event("pointerleave", to_parent(Leave(owner))),
        ],
      items: entries(p, items),
    };
  };
  let rec subpanels = items =>
    List.concat_map(
      (i: item) =>
        switch (i.kind) {
        | Group(children) => subpanels(children)
        | Submenu(children) => [
            make(Some(i.key), children),
            ...subpanels(children),
          ]
        | _ => []
        },
      Array.to_list(items),
    );
  let button =
    Button.parts(~id=trigger_id(m), ~on_press=to_parent(Trigger_click), ());
  to_view({
    trigger:
      Ui_attr.without("keydown", button.root)
      @ [
        Html.aria("haspopup", "menu"),
        Ui_attr.aria_bool("expanded", is_open(m)),
        Html.aria("controls", panel_id(m, None)),
        Ui_attr.data_flag("open", is_open(m)),
        key_handler(
          e =>
            String.equal(e.key, "Escape")
              ? Menu_key(None, e) : Trigger_key(e),
          on_trigger_key_down,
          true,
          is_open(m),
          to_parent,
        ),
      ],
    panels: Array.of_list([make(None, items), ...subpanels(items)]),
  });
};
