module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Dom = Tea_browser.Browser_dom;
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
module Bindings = {
  type element;
  type event = Js.Json.t;
  external document: element = "document";
  external window: element = "window";
  [@mel.send]
  external by_id: (element, string) => Js.Nullable.t(element) =
    "getElementById";
  [@mel.send]
  external rect: element => Tea_browser.Browser_dom.rect =
    "getBoundingClientRect";
  [@mel.get] external client_x: event => float = "clientX";
  [@mel.get] external client_y: event => float = "clientY";
  [@mel.get] external pointer_id: event => int = "pointerId";
  [@mel.get] external pointer_type: event => string = "pointerType";
  [@mel.send]
  external add: (element, string, event => unit, bool) => unit =
    "addEventListener";
  [@mel.send]
  external add_active: (element, string, event => unit, Js.t({..})) => unit =
    "addEventListener";
  [@mel.send]
  external remove: (element, string, event => unit, bool) => unit =
    "removeEventListener";
  [@mel.send] external prevent: event => unit = "preventDefault";
  [@mel.send] external stop: event => unit = "stopPropagation";
  [@mel.get] external detail: event => int = "detail";
  [@mel.get] external target: event => element = "target";
  [@mel.get] external element_id: element => string = "id";
  [@mel.send] external contains: (element, element) => bool = "contains";
  [@mel.send]
  external query_all: (element, string) => array(element) =
    "querySelectorAll";
  [@mel.send]
  external attribute: (element, string) => Js.Nullable.t(string) =
    "getAttribute";
  [@mel.get]
  external optional_pointer: event => Js.Undefined.t(int) = "pointerId";
  let own_touch: (event, string) => bool = [%mel.raw
    {|function(e,id) {
    var el=document.getElementById(id);
    return !!el && e.touches && e.touches.length===1 && e.changedTouches && Array.from(e.changedTouches).some(function(t){return el.contains(t.target);});
  }|}
  ];
  external timeout: (unit => unit, int) => int = "setTimeout";
  external clear_timeout: int => unit = "clearTimeout";
  external raf: (float => unit) => int = "requestAnimationFrame";
  external cancel_raf: int => unit = "cancelAnimationFrame";
  /* The platform has no nearest-scroll-ancestor or elementFromPoint API.
     This private binding performs one bounded scroll step; the subscription
     owns frame scheduling and cancellation. No listeners or timers here. */
  let auto_scroll: (float, float) => bool = [%mel.raw
    {|function(x,y) {
    var el = document.elementFromPoint ? document.elementFromPoint(x,y) : null;
    while (el) {
      var s = window.getComputedStyle(el), r = el.getBoundingClientRect();
      var sx = /(auto|scroll)/.test(s.overflowX) && el.scrollWidth > el.clientWidth;
      var sy = /(auto|scroll)/.test(s.overflowY) && el.scrollHeight > el.clientHeight;
      if (sx || sy) {
        var dx = sx ? (x < r.left+32 ? -12 : x > r.right-32 ? 12 : 0) : 0;
        var dy = sy ? (y < r.top+32 ? -12 : y > r.bottom-32 ? 12 : 0) : 0;
        var ox=el.scrollLeft, oy=el.scrollTop;
        el.scrollLeft += dx; el.scrollTop += dy;
        if (ox !== el.scrollLeft || oy !== el.scrollTop) return true;
      }
      el=el.parentElement;
    }
    var dx=x<32?-12:x>window.innerWidth-32?12:0;
    var dy=y<32?-12:y>window.innerHeight-32?12:0;
    var root=document.scrollingElement || document.documentElement;
    if (root.scrollWidth <= window.innerWidth) dx=0;
    if (root.scrollHeight <= window.innerHeight) dy=0;
    if (dx || dy) { var ox=window.scrollX, oy=window.scrollY; window.scrollBy(dx,dy); return ox!==window.scrollX || oy!==window.scrollY; }
    return false;
  }|}
  ];
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
  List.find_opt(
    (c: container) => String.equal(c.key, key),
    Array.to_list(containers),
  );
let find_item = (containers: array(container), key) => {
  let found = ref(None);
  Array.iter(
    (c: container) =>
      Array.iteri(
        (i, item: item) =>
          if (String.equal(item.key, key)) {
            found := Some((c, i, item));
          },
        c.items,
      ),
    containers,
  );
  found^;
};
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
      s.lifted ? Dom.focus_cmd(handle_id(model, s.key)) : Cmd.none,
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
      Dom.focus_cmd(handle_id(model, s.key)),
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
          Bindings.raf(_ => {
            let boxes =
              List.filter_map(
                el =>
                  if (Js.Nullable.toOption(
                        Bindings.attribute(el, "data-tea-drag-scope"),
                      )
                      == Some(model.id)) {
                    let rect = Bindings.rect(el);
                    Some({
                      id: Bindings.element_id(el),
                      left: rect.left,
                      right: rect.right,
                      top: rect.top,
                      bottom: rect.bottom,
                    });
                  } else {
                    None;
                  },
                Array.to_list(
                  Bindings.query_all(
                    Bindings.document,
                    "[data-tea-drag-scope]",
                  ),
                ),
              )
              ->Array.of_list;
            dispatch(Measured(model.generation, model.revision, boxes));
          });
        () => Bindings.cancel_raf(frame);
      },
    ~tagger=Fun.id,
  );
let box_for = (boxes, id) =>
  List.find_opt((b: box) => String.equal(b.id, id), Array.to_list(boxes));
let hit_target = (containers, model, s, boxes) => {
  let over = ref(None);
  Array.iter(
    (c: container) =>
      if (!c.disabled) {
        switch (box_for(boxes, container_id(model, c.key))) {
        | Some(b)
            when
              s.current.x >= b.left
              && s.current.x <= b.right
              && s.current.y >= b.top
              && s.current.y <= b.bottom =>
          let index = ref(0);
          Array.iter(
            (item: item) =>
              if (!String.equal(item.key, s.key)) {
                switch (box_for(boxes, item_id(model, item.key))) {
                | Some(b) =>
                  let after =
                    switch (model.orientation) {
                    | Vertical => s.current.y > (b.top +. b.bottom) /. 2.
                    | Horizontal =>
                      model.dir == Rtl
                        ? s.current.x < (b.left +. b.right) /. 2.
                        : s.current.x > (b.left +. b.right) /. 2.
                    };
                  if (after) {
                    incr(index);
                  };
                | None => ()
                };
              },
            c.items,
          );
          over :=
            Some({
              container: c.key,
              index: index^,
            });
        | _ => ()
        };
      },
    containers,
  );
  over^;
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
let axis_delta = (model, key) =>
  switch (model.orientation) {
  | Vertical =>
    switch (key) {
    | "ArrowUp" => Some(-1)
    | "ArrowDown" => Some(1)
    | _ => None
    }
  | Horizontal =>
    Ui_nav.horizontal(~dir=model.dir == Rtl ? Ui_nav.Rtl : Ui_nav.Ltr, key)
  };
let cross_delta = (model, key) =>
  switch (model.orientation) {
  | Vertical =>
    Ui_nav.horizontal(~dir=model.dir == Rtl ? Ui_nav.Rtl : Ui_nav.Ltr, key)
  | Horizontal =>
    switch (key) {
    | "ArrowUp" => Some(-1)
    | "ArrowDown" => Some(1)
    | _ => None
    }
  };
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
              let from = ref(-1);
              Array.iteri(
                (i, c: container) =>
                  if (String.equal(c.key, t.container)) {
                    from := i;
                  },
                containers,
              );
              let flags =
                Array.map((c: container) => c.disabled, containers);
              let next =
                delta > 0
                  ? Ui_nav.next(~wrap=false, ~from=from^, flags)
                  : Ui_nav.prev(~wrap=false, ~from=from^, flags);
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
            if (Bindings.auto_scroll(s.current.x, s.current.y)) {
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
                  let move = e => dispatch((0, Some(e)));
                  let release = e => dispatch((1, Some(e)));
                  let abort = _ => dispatch((2, None));
                  let pointer_cancel = e => dispatch((2, Some(e)));
                  let context_menu = e => dispatch((6, Some(e)));
                  let touch_move = e => dispatch((3, Some(e)));
                  Bindings.add_active(
                    Bindings.document,
                    "touchmove",
                    touch_move,
                    {
                      "capture": true,
                      "passive": false,
                    },
                  );
                  Bindings.add(Bindings.document, "pointermove", move, true);
                  Bindings.add(Bindings.document, "pointerup", release, true);
                  Bindings.add(
                    Bindings.document,
                    "pointercancel",
                    pointer_cancel,
                    true,
                  );
                  Bindings.add(
                    Bindings.document,
                    "contextmenu",
                    context_menu,
                    true,
                  );
                  Bindings.add(Bindings.window, "blur", abort, false);
                  () => {
                    Bindings.remove(
                      Bindings.document,
                      "pointermove",
                      move,
                      true,
                    );
                    Bindings.remove(
                      Bindings.document,
                      "pointerup",
                      release,
                      true,
                    );
                    Bindings.remove(
                      Bindings.document,
                      "pointercancel",
                      pointer_cancel,
                      true,
                    );
                    Bindings.remove(Bindings.window, "blur", abort, false);
                    Bindings.remove(
                      Bindings.document,
                      "touchmove",
                      touch_move,
                      true,
                    );
                    Bindings.remove(
                      Bindings.document,
                      "contextmenu",
                      context_menu,
                      true,
                    );
                  };
                },
              ~tagger=
                ((kind, event)) =>
                  switch (event) {
                  | None => Abort
                  | Some(e) when kind == 3 =>
                    if (s.ready
                        && s.start.touch
                        && Bindings.own_touch(e, item_id(model, s.key))) {
                      Bindings.prevent(e);
                    };
                    Tick;
                  | Some(e) when kind == 6 =>
                    switch (
                      Js.Nullable.toOption(
                        Bindings.by_id(
                          Bindings.document,
                          item_id(model, s.key),
                        ),
                      )
                    ) {
                    | Some(el)
                        when
                          s.ready
                          && s.start.touch
                          && Bindings.contains(el, Bindings.target(e)) =>
                      Bindings.prevent(e);
                      Bindings.stop(e);
                    | _ => ()
                    };
                    Tick;
                  | Some(e) =>
                    let p = {
                      x: Bindings.client_x(e),
                      y: Bindings.client_y(e),
                      pointer: Bindings.pointer_id(e),
                      touch: String.equal(Bindings.pointer_type(e), "touch"),
                    };
                    if (s.lifted && p.pointer == s.start.pointer) {
                      Bindings.prevent(e);
                    };
                    kind == 0
                      ? Move(p)
                      : kind == 2
                          ? p.pointer == s.start.pointer ? Abort : Tick
                          : Release(p);
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
                    Bindings.timeout(
                      () => dispatch(Ready(model.generation)),
                      model.touch_delay,
                    );
                  () => Bindings.clear_timeout(timer);
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
                        frame := Bindings.raf(tick);
                      };
                    };
                  frame := Bindings.raf(tick);
                  () => {
                    live := false;
                    Bindings.cancel_raf(frame^);
                  };
                },
              ~tagger=Fun.id,
            )
          : Sub.none;
      let keyboard =
        Sub.filter_map(
          event =>
            switch (Tea_dom.Dom_decode.run(Ui_keyboard.decoder, event)) {
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
                  Bindings.prevent(event);
                  Bindings.stop(event);
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
              dispatch => {
                Bindings.add(Bindings.document, "keydown", dispatch, true);
                () =>
                  Bindings.remove(
                    Bindings.document,
                    "keydown",
                    dispatch,
                    true,
                  );
              },
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
            let click = e => dispatch((0, Some(e)));
            let release = e => dispatch((1, Some(e)));
            let reset = e => dispatch((2, Some(e)));
            let abort = e => dispatch((3, Some(e)));
            let blur = _ => dispatch((2, None));
            Bindings.add(Bindings.document, "click", click, true);
            Bindings.add(Bindings.document, "pointerup", release, true);
            Bindings.add(Bindings.document, "pointerdown", reset, true);
            Bindings.add(Bindings.document, "pointercancel", abort, true);
            Bindings.add(Bindings.window, "blur", blur, false);
            (
              () => {
                Bindings.remove(Bindings.document, "click", click, true);
                Bindings.remove(
                  Bindings.document,
                  "pointerup",
                  release,
                  true,
                );
                Bindings.remove(
                  Bindings.document,
                  "pointerdown",
                  reset,
                  true,
                );
                Bindings.remove(
                  Bindings.document,
                  "pointercancel",
                  abort,
                  true,
                );
                Bindings.remove(Bindings.window, "blur", blur, false);
              }
            );
          },
        );
      let events =
        Sub.filter_map(
          ((kind, event)) =>
            switch (event) {
            | None => Some(Guard_clear)
            | Some(e) =>
              let matches =
                switch (Js.Undefined.toOption(Bindings.optional_pointer(e))) {
                | Some(pointer) => pointer == guard.pointer
                | None => true
                };
              if (kind == 2) {
                matches || guard.released ? Some(Guard_clear) : None;
              } else if (!matches) {
                None;
              } else if (kind == 0) {
                if (Bindings.detail(e) > 0) {
                  Bindings.prevent(e);
                  Bindings.stop(e);
                  Some(Guard_clear);
                } else {
                  None;
                };
              } else if (kind == 1) {
                Some(Guard_release(guard.pointer));
              } else {
                Some(Guard_clear);
              };
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
                let frame = Bindings.raf(_ => dispatch(Guard_clear));
                (() => Bindings.cancel_raf(frame));
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
  let prevent =
    Html.on(
      "dragstart",
      V.Custom_optional(
        D.succeed({
          V.message: None,
          prevent_default: true,
          stop_propagation: false,
        }),
      ),
    );

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
