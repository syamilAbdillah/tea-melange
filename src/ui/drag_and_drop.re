module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
type orientation =
  | Vertical
  | Horizontal;
type direction =
  | Ltr
  | Rtl;
type item = {
  key: string,
  label: string,
  disabled: bool,
};
type container = {
  key: string,
  label: string,
  disabled: bool,
  items: array(item),
};
type target = {
  container: string,
  index: int,
};
type movement = {
  key: string,
  from_container: string,
  from_index: int,
  to_container: string,
  to_index: int,
};
type out_msg =
  | Drag_started(string)
  | Moved(movement)
  | Cancelled;
type labels = {
  instructions: string,
  picked_up: (string, int, int) => string,
  moved: (string, string, int, int) => string,
  dropped: string,
  cancelled: (string, int) => string,
};
let default_labels = {
  instructions: "Space or Enter picks up and drops. Arrow keys move. Shift with the cross-axis arrows changes containers. Escape cancels. Tab cancels and moves focus.",
  picked_up: (label, position, count) =>
    "Picked up "
    ++ label
    ++ ". Position "
    ++ string_of_int(position)
    ++ " of "
    ++ string_of_int(count)
    ++ ".",
  moved: (label, container, position, count) =>
    label
    ++ " moved to position "
    ++ string_of_int(position)
    ++ " of "
    ++ string_of_int(count)
    ++ " in "
    ++ container
    ++ ".",
  dropped: "Dropped.",
  cancelled: (label, position) =>
    "Cancelled, "
    ++ label
    ++ " returned to position "
    ++ string_of_int(position)
    ++ ".",
};
type point = {
  x: float,
  y: float,
  pointer: int,
  touch: bool,
};
type box = {
  id: string,
  left: float,
  right: float,
  top: float,
  bottom: float,
};
type session = {
  key: string,
  label: string,
  source: string,
  source_index: int,
  start: point,
  current: point,
  keyboard: bool,
  lifted: bool,
  ready: bool,
  target: option(target),
  ending: bool,
};
type click_guard = {
  pointer: int,
  released: bool,
};
type model = {
  id: string,
  orientation,
  dir: direction,
  threshold: float,
  touch_delay: int,
  drag_from_item: bool,
  labels,
  assertive: bool,
  session: option(session),
  guard: option(click_guard),
  text: string,
  generation: int,
  revision: int,
};
type msg =
  | Press(string, point)
  | Move(point)
  | Release(point)
  | Abort
  | Key(string, Ui_keyboard.event)
  | Activate(string)
  | Ready(int)
  | Measure
  | Measured(int, int, array(box))
  | Tick
  | Guard_release(int)
  | Guard_clear;
/* Whether a single-finger touch event touches the element with [id]. */
let own_touch = (event, id) => {
  let touches = name => {
    let list = Ui_dom.get(event, name);
    Js.testAny(list)
      ? [||]
      : Array.init(Ui_dom.length(list), i =>
          Ui_dom.get(list, string_of_int(i))
        );
  };
  switch (Ui_dom.find(id)) {
  | Some(el) =>
    Array.length(touches("touches")) == 1
    && Array.exists(
         touch =>
           Ui_dom.contains(el, Ui_dom.to_node(Ui_dom.get(touch, "target"))),
         touches("changedTouches"),
       )
  | None => false
  };
};
/* One auto-scroll step toward a pointer within [edge] px of a boundary: the
   nearest scrollable ancestor under the point, else the window. Returns
   whether anything scrolled. */
let auto_scroll = (x, y) => {
  let edge = 32.;
  let nudge = (position, low, high) =>
    position < low +. edge ? (-12.) : position > high -. edge ? 12. : 0.;
  let scrollable = overflow =>
    String.equal(overflow, "auto") || String.equal(overflow, "scroll");
  let rec ancestor = node =>
    switch (node) {
    | None => false
    | Some(el) =>
      let style = Ui_dom.computed_style(Ui_dom.window, el);
      let sx =
        scrollable(Ui_dom.overflow_x(style))
        && Ui_dom.scroll_width(el) > Ui_dom.client_width(el);
      let sy =
        scrollable(Ui_dom.overflow_y(style))
        && Ui_dom.scroll_height(el) > Ui_dom.client_height(el);
      let scrolled =
        (sx || sy)
        && {
          let r = Ui_dom.rect(el);
          let (left, top) = (
            Ui_dom.scroll_left(el),
            Ui_dom.scroll_top(el),
          );
          Ui_dom.set_scroll_left(
            el,
            left +. (sx ? nudge(x, r.left, r.right) : 0.),
          );
          Ui_dom.set_scroll_top(
            el,
            top +. (sy ? nudge(y, r.top, r.bottom) : 0.),
          );
          left != Ui_dom.scroll_left(el) || top != Ui_dom.scroll_top(el);
        };
      scrolled || ancestor(Ui_dom.parent(el));
    };
  ancestor(Ui_dom.element_from_point(x, y))
  || {
    let window = Ui_dom.window;
    let root = Ui_dom.scrolling_element();
    let (width, height) = (
      Ui_dom.inner_width(window),
      Ui_dom.inner_height(window),
    );
    let dx = Ui_dom.scroll_width(root) <= width ? 0. : nudge(x, 0., width);
    let dy =
      Ui_dom.scroll_height(root) <= height ? 0. : nudge(y, 0., height);
    (dx != 0. || dy != 0.)
    && {
      let (sx, sy) = (Ui_dom.scroll_x(window), Ui_dom.scroll_y(window));
      Ui_dom.scroll_by(window, dx, dy);
      sx != Ui_dom.scroll_x(window) || sy != Ui_dom.scroll_y(window);
    };
  };
};
let init =
    (
      ~id,
      ~orientation=Vertical,
      ~dir=Ltr,
      ~threshold=6.,
      ~touch_delay=250,
      ~drag_from_item=true,
      ~labels=default_labels,
      ~assertive=false,
      (),
    ) => {
  id,
  orientation,
  dir,
  threshold: max(0., threshold),
  touch_delay: max(0, touch_delay),
  drag_from_item,
  labels,
  assertive,
  session: None,
  guard: None,
  text: "",
  generation: 0,
  revision: 0,
};
let preview = model =>
  switch (model.session) {
  | Some(s) when s.lifted => s.target
  | _ => None
  };
let dragging = model =>
  switch (model.session) {
  | Some(s) when s.lifted => Some(s.key)
  | _ => None
  };
let announcement = model => model.text;
let item_id = (model, key) => Ui_attr.item_id(model.id, key);
let handle_id = (model, key) =>
  Ui_attr.part_id(item_id(model, key), "handle");
let container_id = (model, key) =>
  Ui_attr.part_id(model.id, "container-" ++ Ui_attr.safe_key(key));
let find_container = (containers: array(container), key) =>
  Array.find_opt((c: container) => String.equal(c.key, key), containers);
/* The container holding [key], the item's index there, and the item. */
let find_item = (containers: array(container), key) =>
  Array.find_map(
    (c: container) =>
      Array.find_mapi(
        (i, item: item) =>
          String.equal(item.key, key) ? Some((c, i, item)) : None,
        c.items,
      ),
    containers,
  );
let count = (c: container, key) =>
  Array.fold_left(
    (n, i: item) => n + (String.equal(i.key, key) ? 0 : 1),
    0,
    c.items,
  );
let repair_target = (containers, key, target) =>
  switch (target) {
  | None => None
  | Some(t) =>
    switch (find_container(containers, t.container)) {
    | Some(c) when !c.disabled =>
      Some({
        container: c.key,
        index: min(max(0, t.index), count(c, key)),
      })
    | _ => None
    }
  };
let cancel = model =>
  switch (model.session) {
  | None => (model, Cmd.none)
  | Some(s) => (
      {
        ...model,
        session: None,
        generation: model.generation + 1,
        text:
          s.lifted
            ? model.labels.cancelled(s.label, s.source_index + 1) : model.text,
      },
      s.lifted ? Browser.focus_cmd(handle_id(model, s.key)) : Cmd.none,
    )
  };
let stop = (~focus=true, model) => {
  let lifted = dragging(model) != None;
  let (next, command) = cancel(model);
  (next, focus ? command : Cmd.none, lifted ? [Cancelled] : []);
};
let drop = (containers, model, s) =>
  switch (
    repair_target(containers, s.key, s.target),
    find_item(containers, s.key),
  ) {
  | (Some(t), Some((source, index, item)))
      when !source.disabled && !item.disabled => (
      {
        ...model,
        session: None,
        text: model.labels.dropped,
        generation: model.generation + 1,
      },
      Browser.focus_cmd(handle_id(model, s.key)),
      [
        Moved({
          key: s.key,
          from_container: source.key,
          from_index: index,
          to_container: t.container,
          to_index: t.index,
        }),
      ],
    )
  | _ => stop(model)
  };
let measurement = model =>
  Sub.make(
    ~home="Tea_ui.Drag_and_drop.measure",
    ~key=
      model.id
      ++ ":"
      ++ string_of_int(model.generation)
      ++ ":"
      ++ string_of_int(model.revision),
    ~start=
      dispatch => {
        let frame =
          Ui_dom.raf(_ => {
            let boxes =
              List.filter_map(
                el =>
                  if (Ui_dom.attribute(el, "data-tea-drag-scope")
                      == Some(model.id)) {
                    let rect = Ui_dom.rect(el);
                    Some({
                      id: Ui_dom.id(el),
                      left: rect.left,
                      right: rect.right,
                      top: rect.top,
                      bottom: rect.bottom,
                    });
                  } else {
                    None;
                  },
                Array.to_list(
                  Ui_dom.query_all(Ui_dom.document, "[data-tea-drag-scope]"),
                ),
              )
              ->Array.of_list;
            dispatch(Measured(model.generation, model.revision, boxes));
          });
        () => Ui_dom.cancel_raf(frame);
      },
    ~tagger=Fun.id,
  );
let box_for = (boxes, id) =>
  Array.find_opt((b: box) => String.equal(b.id, id), boxes);
/* The last enabled container under the pointer, and the drop index: the
   number of its other items whose midpoint the pointer has passed. */
let hit_target = (containers, model, s, boxes) => {
  let { x, y, _ } = s.current;
  let inside = (b: box) =>
    x >= b.left && x <= b.right && y >= b.top && y <= b.bottom;
  let past = (b: box) =>
    switch (model.orientation) {
    | Vertical => y > (b.top +. b.bottom) /. 2.
    | Horizontal =>
      model.dir == Rtl
        ? x < (b.left +. b.right) /. 2. : x > (b.left +. b.right) /. 2.
    };
  let target = (c: container) => {
    container: c.key,
    index:
      Array.fold_left(
        (n, item: item) =>
          !String.equal(item.key, s.key)
          && Option.fold(
               ~none=false,
               ~some=past,
               box_for(boxes, item_id(model, item.key)),
             )
            ? n + 1 : n,
        0,
        c.items,
      ),
  };
  Array.fold_left(
    (over, c: container) =>
      !c.disabled
      && Option.fold(
           ~none=false,
           ~some=inside,
           box_for(boxes, container_id(model, c.key)),
         )
        ? Some(target(c)) : over,
    None,
    containers,
  );
};
let with_target = (containers, model, s, target) => {
  let changed =
    switch (s.target, target) {
    | (Some(a), Some(b)) =>
      !String.equal(a.container, b.container) || a.index != b.index
    | (None, None) => false
    | _ => true
    };
  let text =
    switch (target) {
    | Some(t) when changed =>
      switch (find_container(containers, t.container)) {
      | Some(c) =>
        model.labels.moved(
          s.label,
          c.label,
          t.index + 1,
          count(c, s.key) + 1,
        )
      | None => model.text
      }
    | _ => model.text
    };
  {
    ...model,
    session:
      Some({
        ...s,
        target,
      }),
    text,
  };
};
let vertical_delta = key =>
  switch (key) {
  | "ArrowUp" => Some(-1)
  | "ArrowDown" => Some(1)
  | _ => None
  };
let horizontal_delta = (model, key) =>
  Ui_nav.horizontal(~dir=model.dir == Rtl ? Ui_nav.Rtl : Ui_nav.Ltr, key);
/* Arrow movement along the list, and across to another container. */
let axis_delta = (model, key) =>
  model.orientation == Vertical
    ? vertical_delta(key) : horizontal_delta(model, key);
let cross_delta = (model, key) =>
  model.orientation == Vertical
    ? horizontal_delta(model, key) : vertical_delta(key);
let lift = (containers, model, key, start, keyboard) =>
  switch (find_item(containers, key)) {
  | Some((c, index, item)) when !c.disabled && !item.disabled =>
    let s = {
      key,
      label: item.label,
      source: c.key,
      source_index: index,
      start,
      current: start,
      keyboard,
      lifted: keyboard,
      ready: keyboard || !start.touch,
      target:
        Some({
          container: c.key,
          index,
        }),
      ending: false,
    };
    (
      {
        ...model,
        session: Some(s),
        generation: model.generation + 1,
        text:
          keyboard
            ? model.labels.picked_up(
                item.label,
                index + 1,
                Array.length(c.items),
              )
            : model.text,
      },
      Cmd.none,
      keyboard ? [Drag_started(key)] : [],
    );
  | _ => (model, Cmd.none, [])
  };
let zero = {
  x: 0.,
  y: 0.,
  pointer: (-1),
  touch: false,
};
let update = (~containers, message, model) =>
  switch (message) {
  | Guard_clear => (
      {
        ...model,
        guard: None,
      },
      Cmd.none,
      [],
    )
  | Guard_release(pointer) => (
      {
        ...model,
        guard:
          Option.map(
            guard =>
              guard.pointer == pointer
                ? {
                  ...guard,
                  released: true,
                }
                : guard,
            model.guard,
          ),
      },
      Cmd.none,
      [],
    )
  | _ =>
    let invalid =
      switch (model.session) {
      | None => false
      | Some(s) =>
        switch (find_item(containers, s.key)) {
        | Some((c, _, item)) => c.disabled || item.disabled
        | None => true
        }
      };
    if (invalid) {
      stop(model);
    } else {
      let model =
        switch (model.session) {
        | Some(s) => {
            ...model,
            session:
              Some({
                ...s,
                target:
                  switch (repair_target(containers, s.key, s.target)) {
                  | Some(t) => Some(t)
                  | None when s.keyboard =>
                    switch (find_item(containers, s.key)) {
                    | Some((c, i, _)) =>
                      Some({
                        container: c.key,
                        index: i,
                      })
                    | None => None
                    }
                  | None => None
                  },
              }),
          }
        | None => model
        };
      switch (message, model.session) {
      | (Press(key, p), None) => lift(containers, model, key, p, false)
      | (Activate(key), None) => lift(containers, model, key, zero, true)
      | (Activate(_), Some(s)) when s.keyboard => drop(containers, model, s)
      | (Key(key, event), None)
          when (event.key == " " || event.key == "Enter") && !event.repeat =>
        lift(containers, model, key, zero, true)
      | (Key(_, event), Some(_)) when event.key == "Escape" => stop(model)
      | (Key(_, event), Some(_)) when event.key == "Tab" =>
        stop(~focus=false, model)
      | (Key(_, event), Some(s)) when s.keyboard =>
        if ((event.key == " " || event.key == "Enter") && !event.repeat) {
          drop(containers, model, s);
        } else {
          let delta =
            event.shift
              ? cross_delta(model, event.key) : axis_delta(model, event.key);
          let target =
            switch (delta, s.target) {
            | (Some(delta), Some(t)) when event.shift =>
              let from =
                Option.value(
                  ~default=-1,
                  Array.find_index(
                    (c: container) => String.equal(c.key, t.container),
                    containers,
                  ),
                );
              let flags =
                Array.map((c: container) => c.disabled, containers);
              let next =
                delta > 0
                  ? Ui_nav.next(~wrap=false, ~from, flags)
                  : Ui_nav.prev(~wrap=false, ~from, flags);
              switch (next) {
              | Some(i) =>
                Some({
                  container: containers[i].key,
                  index: min(t.index, count(containers[i], s.key)),
                })
              | None => None
              };
            | (Some(delta), Some(t)) =>
              repair_target(
                containers,
                s.key,
                Some({
                  ...t,
                  index: t.index + delta,
                }),
              )
            | _ => s.target
            };
          (with_target(containers, model, s, target), Cmd.none, []);
        }
      | (Move(p), Some(s))
          when !s.keyboard && !s.ending && p.pointer == s.start.pointer =>
        let distance =
          max(abs_float(p.x -. s.start.x), abs_float(p.y -. s.start.y));
        if (!s.ready && distance >= model.threshold) {
          stop(model);
        } else {
          let lifted = s.lifted || s.ready && distance >= model.threshold;
          let s2 = {
            ...s,
            current: p,
            lifted,
          };
          let next = {
            ...model,
            session: Some(s2),
            guard:
              lifted && !s.lifted
                ? Some({
                    pointer: s.start.pointer,
                    released: false,
                  })
                : model.guard,
            revision: model.revision + 1,
            text:
              lifted && !s.lifted
                ? model.labels.picked_up(
                    s.label,
                    s.source_index + 1,
                    switch (find_container(containers, s.source)) {
                    | Some(c) => Array.length(c.items)
                    | None => 0
                    },
                  )
                : model.text,
          };
          (
            next,
            Cmd.none,
            lifted && !s.lifted ? [Drag_started(s.key)] : [],
          );
        };
      | (Release(p), Some(s))
          when !s.keyboard && !s.ending && p.pointer == s.start.pointer =>
        if (!s.lifted) {
          (
            {
              ...model,
              session: None,
            },
            Cmd.none,
            [],
          );
        } else {
          let next = {
            ...model,
            session:
              Some({
                ...s,
                current: p,
                ending: true,
              }),
            guard:
              Option.map(
                guard =>
                  {
                    ...guard,
                    released: true,
                  },
                model.guard,
              ),
            revision: model.revision + 1,
          };
          (next, Cmd.none, []);
        }
      | (Ready(g), Some(s)) when g == model.generation => (
          {
            ...model,
            session:
              Some({
                ...s,
                ready: true,
              }),
          },
          Cmd.none,
          [],
        )
      | (Measured(g, r, boxes), Some(s))
          when g == model.generation && r == model.revision && s.lifted =>
        let target = hit_target(containers, model, s, boxes);
        let next = with_target(containers, model, s, target);
        s.ending
          ? drop(
              containers,
              next,
              {
                ...s,
                target,
              },
            )
          : (next, Cmd.none, []);
      | (Tick, Some(s)) when s.lifted && !s.keyboard && !s.ending => (
          model,
          Cmd.effect(dispatch =>
            if (auto_scroll(s.current.x, s.current.y)) {
              dispatch(Measure);
            }
          ),
          [],
        )
      | (Measure, Some(s)) when s.lifted && !s.keyboard =>
        let next = {
          ...model,
          revision: model.revision + 1,
        };
        (next, Cmd.none, []);
      | (Abort, Some(_)) =>
        stop({
          ...model,
          guard: None,
        })
      | _ => (model, Cmd.none, [])
      };
    };
  };
/* Document events while a pointer drag is active. */
type drag_event =
  | Pointer_move(Dom.event)
  | Pointer_up(Dom.event)
  | Pointer_cancel(Dom.event)
  | Touch_move(Dom.event)
  | Context_menu(Dom.event)
  | Window_blur;
/* Document events while the post-drop click guard is armed. */
type guard_event =
  | Guard_click(Dom.event)
  | Guard_pointer_up(Dom.event)
  | Guard_pointer_down(Dom.event)
  | Guard_pointer_cancel(Dom.event)
  | Guard_blur;
let subscriptions = model => {
  let interaction =
    switch (model.session) {
    | None => Sub.none
    | Some(s) =>
      let pointer =
        s.keyboard
          ? Sub.none
          : Sub.make(
              ~home="Tea_ui.Drag_and_drop.pointer",
              ~key=model.id,
              ~start=
                dispatch => {
                  let on = (~passive=?, target, name, tag) =>
                    Ui_dom.on(~capture=true, ~passive?, target, name, e =>
                      dispatch(tag(e))
                    );
                  let offs = [
                    on(Ui_dom.document, "pointermove", e => Pointer_move(e)),
                    on(Ui_dom.document, "pointerup", e => Pointer_up(e)),
                    on(Ui_dom.document, "pointercancel", e =>
                      Pointer_cancel(e)
                    ),
                    on(Ui_dom.document, "contextmenu", e => Context_menu(e)),
                    on(~passive=false, Ui_dom.document, "touchmove", e =>
                      Touch_move(e)
                    ),
                    Ui_dom.on(Ui_dom.window, "blur", _ =>
                      dispatch(Window_blur)
                    ),
                  ];
                  () => List.iter(off => off(), offs);
                },
              ~tagger=
                event => {
                  let point = e => {
                    x: Ui_dom.client_x(e),
                    y: Ui_dom.client_y(e),
                    pointer: Ui_dom.pointer_id(e),
                    touch: String.equal(Ui_dom.pointer_type(e), "touch"),
                  };
                  let held = s.ready && s.start.touch;
                  let own = e => {
                    let p = point(e);
                    if (s.lifted && p.pointer == s.start.pointer) {
                      Ui_dom.prevent(e);
                    };
                    p;
                  };
                  switch (event) {
                  | Window_blur => Abort
                  | Touch_move(e) =>
                    if (held && own_touch(e, item_id(model, s.key))) {
                      Ui_dom.prevent(e);
                    };
                    Tick;
                  | Context_menu(e) =>
                    if (held
                        && Ui_dom.within(
                             item_id(model, s.key),
                             Ui_dom.target(e),
                           )) {
                      Ui_dom.prevent(e);
                      Ui_dom.stop(e);
                    };
                    Tick;
                  | Pointer_move(e) => Move(own(e))
                  | Pointer_up(e) => Release(own(e))
                  | Pointer_cancel(e) =>
                    own(e).pointer == s.start.pointer ? Abort : Tick
                  };
                },
            );
      let delay =
        s.keyboard || s.ready
          ? Sub.none
          : Sub.make(
              ~home="Tea_ui.Drag_and_drop.touch",
              ~key=model.id ++ string_of_int(model.generation),
              ~start=
                dispatch => {
                  let timer =
                    Ui_dom.timeout(
                      () => dispatch(Ready(model.generation)),
                      model.touch_delay,
                    );
                  () => Ui_dom.clear_timeout(timer);
                },
              ~tagger=Fun.id,
            );
      let frames =
        s.lifted && !s.keyboard && !s.ending
          ? Sub.make(
              ~home="Tea_ui.Drag_and_drop.frames",
              ~key=model.id,
              ~start=
                dispatch => {
                  let live = ref(true);
                  let frame = ref(0);
                  let rec tick = _ =>
                    if (live^) {
                      dispatch(Tick);
                      if (live^) {
                        frame := Ui_dom.raf(tick);
                      };
                    };
                  frame := Ui_dom.raf(tick);
                  () => {
                    live := false;
                    Ui_dom.cancel_raf(frame^);
                  };
                },
              ~tagger=Fun.id,
            )
          : Sub.none;
      let keyboard =
        Sub.filter_map(
          event =>
            switch (
              Tea_dom.Dom_decode.run(Ui_keyboard.decoder, Ui_dom.raw(event))
            ) {
            | Ok(key)
                when Ui_keyboard.available(key) && Ui_keyboard.plain(key) =>
              let activation = key.key == " " || key.key == "Enter";
              let arrow =
                key.shift
                  ? cross_delta(model, key.key) != None
                  : axis_delta(model, key.key) != None;
              if (key.key == "Escape"
                  || key.key == "Tab"
                  || s.keyboard
                  && (activation || arrow)) {
                if (key.key != "Tab") {
                  Ui_dom.prevent(event);
                  Ui_dom.stop(event);
                };
                Some(Key(s.key, key));
              } else {
                None;
              };
            | _ => None
            },
          Sub.make(
            ~home="Tea_ui.Drag_and_drop.keys",
            ~key=model.id,
            ~start=
              dispatch =>
                Ui_dom.on(
                  ~capture=true,
                  Ui_dom.document,
                  "keydown",
                  dispatch,
                ),
            ~tagger=Fun.id,
          ),
        );
      Sub.batch([
        pointer,
        delay,
        frames,
        s.lifted && !s.keyboard ? measurement(model) : Sub.none,
        keyboard,
        Events.on_visibility_change(visible => visible ? Measure : Abort),
        Events.on_scroll(Measure),
        Events.on_resize(_ => Measure),
      ]);
    };
  let guard =
    switch (model.guard) {
    | None => Sub.none
    | Some(guard) =>
      let source =
        Sub.make(
          ~home="Tea_ui.Drag_and_drop.click_guard",
          ~key=model.id,
          ~tagger=Fun.id,
          ~start=dispatch => {
            let on = (name, tag) =>
              Ui_dom.on(~capture=true, Ui_dom.document, name, e =>
                dispatch(tag(e))
              );
            let offs = [
              on("click", e => Guard_click(e)),
              on("pointerup", e => Guard_pointer_up(e)),
              on("pointerdown", e => Guard_pointer_down(e)),
              on("pointercancel", e => Guard_pointer_cancel(e)),
              Ui_dom.on(Ui_dom.window, "blur", _ => dispatch(Guard_blur)),
            ];
            (() => List.iter(off => off(), offs));
          },
        );
      /* Clicks without a pointerId (keyboard, synthetic) match any pointer. */
      let ours = e =>
        switch (Js.Undefined.toOption(Ui_dom.optional_pointer_id(e))) {
        | Some(pointer) => pointer == guard.pointer
        | None => true
        };
      let events =
        Sub.filter_map(
          event =>
            switch (event) {
            | Guard_blur => Some(Guard_clear)
            | Guard_pointer_down(e) =>
              ours(e) || guard.released ? Some(Guard_clear) : None
            | Guard_click(e) when ours(e) && Ui_dom.detail(e) > 0 =>
              Ui_dom.prevent(e);
              Ui_dom.stop(e);
              Some(Guard_clear);
            | Guard_pointer_up(e) when ours(e) =>
              Some(Guard_release(guard.pointer))
            | Guard_pointer_cancel(e) when ours(e) => Some(Guard_clear)
            | Guard_click(_)
            | Guard_pointer_up(_)
            | Guard_pointer_cancel(_) => None
            },
          source,
        );
      let expiry =
        guard.released
          ? Sub.make(
              ~home="Tea_ui.Drag_and_drop.click_expiry",
              ~key=model.id,
              ~tagger=Fun.id,
              ~start=dispatch => {
                let frame = Ui_dom.raf(_ => dispatch(Guard_clear));
                (() => Ui_dom.cancel_raf(frame));
              },
            )
          : Sub.none;
      Sub.batch([events, expiry]);
    };
  Sub.batch([interaction, guard]);
};
type parts('parent) = {
  container: string => list(Html.attr('parent)),
  item: string => list(Html.attr('parent)),
  handle: string => list(Html.attr('parent)),
  instructions: list(Html.attr('parent)),
  live: list(Html.attr('parent)),
  instructions_text: string,
  live_text: string,
  preview: option(target),
  dragging: option(string),
  overlay: list(Html.attr('parent)),
};
let pointer_decoder =
  D.map3(
    (x, y, (pointer, touch)) =>
      {
        x,
        y,
        pointer,
        touch,
      },
    D.field("clientX", D.float),
    D.field("clientY", D.float),
    D.map2(
      (pointer, pointer_type) =>
        (pointer, String.equal(pointer_type, "touch")),
      D.field("pointerId", D.int),
      D.field("pointerType", D.string),
    ),
  );
let view = (~model, ~containers, ~to_parent, ~to_view, ()) => {
  let active = dragging(model);
  let target = preview(model);
  let geometry =
    switch (model.session) {
    | Some(s) when s.lifted && !s.keyboard => [
        Html.style(
          "--tea-drag-x",
          Js.Float.toString(s.current.x -. s.start.x) ++ "px",
        ),
        Html.style(
          "--tea-drag-y",
          Js.Float.toString(s.current.y -. s.start.y) ++ "px",
        ),
      ]
    | _ => [
        Html.style("--tea-drag-x", "0px"),
        Html.style("--tea-drag-y", "0px"),
      ]
    };
  let disabled = key =>
    switch (find_item(containers, key)) {
    | Some((c, _, item)) => c.disabled || item.disabled
    | None => true
    };
  let pointer = key =>
    Html.on(
      "pointerdown",
      V.Custom_optional(
        D.and_then(
          ((button, primary)) =>
            button != 0 || !primary || disabled(key)
              ? D.fail("disabled or nonprimary")
              : D.map(
                  p =>
                    {
                      V.message: Some(to_parent(Press(key, p))),
                      prevent_default: false,
                      stop_propagation: false,
                    },
                  pointer_decoder,
                ),
          D.map2(
            (button, primary) => (button, primary),
            D.field("button", D.int),
            D.field("isPrimary", D.bool),
          ),
        ),
      ),
    );
  let prevent = Ui_attr.prevent_default("dragstart");

  let instructions_id = Ui_attr.part_id(model.id, "instructions");
  to_view({
    container: key => [
      Html.id(container_id(model, key)),
      Html.data("tea-drag-scope", model.id),
      Html.aria(
        "label",
        switch (find_container(containers, key)) {
        | Some(c) => c.label
        | None => key
        },
      ),
      Ui_attr.data_flag(
        "over",
        switch (target) {
        | Some(t) => String.equal(t.container, key)
        | None => false
        },
      ),
      Html.data(
        "orientation",
        model.orientation == Vertical ? "vertical" : "horizontal",
      ),
      Ui_attr.data_flag(
        "disabled",
        switch (find_container(containers, key)) {
        | Some(c) => c.disabled
        | None => true
        },
      ),
    ],
    item: key =>
      [
        Html.id(item_id(model, key)),
        Html.data("tea-drag-scope", model.id),
        Ui_attr.data_flag("dragging", active == Some(key)),
        Ui_attr.data_flag("disabled", disabled(key)),
        prevent,
        model.drag_from_item ? pointer(key) : Html.none,
      ]
      @ geometry,
    handle: key => [
      Html.id(handle_id(model, key)),
      Html.type_("button"),
      Html.disabled(disabled(key)),
      Html.aria("describedby", instructions_id),
      Ui_attr.data_flag("dragging", active == Some(key)),
      Ui_attr.data_flag("disabled", disabled(key)),
      pointer(key),
      prevent,
      Html.on(
        "keydown",
        V.Custom_optional(
          Ui_keyboard.custom(event => {
            let activation = event.key == " " || event.key == "Enter";
            if (!disabled(key) && Ui_keyboard.plain(event) && activation) {
              Some((
                activation && event.repeat
                  ? None : Some(to_parent(Key(key, event))),
                event.key != "Tab",
              ));
            } else {
              None;
            };
          }),
        ),
      ),
      Html.on(
        "keyup",
        V.Custom_optional(
          Ui_keyboard.custom(event =>
            Ui_keyboard.plain(event)
            && (event.key == " " || event.key == "Enter")
              ? Some((None, true)) : None
          ),
        ),
      ),
      Html.on(
        "click",
        V.Custom_optional(
          D.map2(
            (detail, plain) =>
              {
                V.message:
                  detail == 0 && plain && !disabled(key)
                    ? Some(to_parent(Activate(key))) : None,
                prevent_default: true,
                stop_propagation: true,
              },
            D.field("detail", D.int),
            D.map3(
              (ctrl, alt, meta) => !ctrl && !alt && !meta,
              D.field("ctrlKey", D.bool),
              D.field("altKey", D.bool),
              D.field("metaKey", D.bool),
            ),
          ),
        ),
      ),
    ],
    instructions: [Html.id(instructions_id)],
    live: [
      Html.id(Ui_attr.part_id(model.id, "live")),
      Html.aria("live", model.assertive ? "assertive" : "polite"),
      Html.aria("atomic", "true"),
    ],
    instructions_text: model.labels.instructions,
    live_text: model.text,
    preview: target,
    dragging: active,
    overlay: [Html.aria("hidden", "true")] @ geometry,
  });
};
