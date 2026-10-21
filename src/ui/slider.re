module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Tea_core.Decode;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Task = Tea_core.Task;
module Dom = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
type orientation =
  | Horizontal
  | Vertical;
type direction =
  | Ltr
  | Rtl;
type label =
  | Label(string)
  | Labelled_by(list(string));
type thumb = {
  key: string,
  label,
  initial: float,
  disabled: bool,
};
type values = list((string, float));
type out_msg =
  | Changed(values)
  | Committed(values);
type point = {
  pointer: int,
  x: float,
  y: float,
};
type drag = {
  serial: int,
  point,
  key: option(string),
  rect: option(Dom.rect),
  ended: bool,
  changed: bool,
  capture: bool,
};
type model = {
  id: string,
  min: float,
  max: float,
  step: float,
  ticks: float,
  precision: int,
  min_distance: int,
  large_step: int,
  orientation,
  dir: direction,
  disabled: bool,
  values,
  drag: option(drag),
  pending: list(drag),
  serial: int,
};
type msg =
  | Key(string, Ui_keyboard.event)
  | Down(point, bool)
  | Move(int, point)
  | End(int, point)
  | Measured(int, result(Dom.element_info, Dom.error))
  | Measure
  | Cancel(int);
[@mel.scope "Number"] external finite: float => bool = "isFinite";
[@mel.scope "Math"] external round: float => float = "round";
[@mel.scope "Math"] external floor: float => float = "floor";
[@mel.send] external fixed: (float, int) => string = "toFixed";
let decimal_places = number => {
  let pieces = String.split_on_char('e', Js.Float.toString(number));
  let (base, exponent) =
    switch (pieces) {
    | [base, exponent] => (base, int_of_string(exponent))
    | [base] => (base, 0)
    | _ => ("0", 0)
    };
  let decimals =
    switch (String.split_on_char('.', base)) {
    | [_, fraction] => String.length(fraction)
    | _ => 0
    };
  max(0, decimals - exponent);
};
let clamp = (lo, hi, value) => max(lo, min(hi, value));
let clean = (model, value) =>
  float_of_string(fixed(value, model.precision));
let tick = (model, value) =>
  clamp(0., model.ticks, round((value -. model.min) /. model.step +. 1e-9));
let number = (model, tick) => clean(model, model.min +. tick *. model.step);
let find = (key, values, fallback) =>
  switch (List.find_opt(((k, _)) => String.equal(k, key), values)) {
  | Some((_, value)) => value
  | None => fallback
  };
let validate_items = (items: array(thumb), model) => {
  if (float_of_int(max(0, Array.length(items) - 1))
      *. float_of_int(model.min_distance) > model.ticks) {
    invalid_arg("Slider: insufficient range for min_distance");
  };
  Array.iteri(
    (i, item: thumb) => {
      if (!finite(item.initial)) {
        invalid_arg("Slider: nonfinite initial value");
      };
      for (j in 0 to i - 1) {
        if (String.equal(item.key, items[j].key)) {
          invalid_arg("Slider: duplicate thumb key");
        };
      };
      let named =
        switch (item.label) {
        | Label(label) => String.trim(label) != ""
        | Labelled_by(ids) =>
          ids != [] && List.for_all(id => String.trim(id) != "", ids)
        };
      if (!named) {
        invalid_arg("Slider: each thumb needs a name");
      };
    },
    items,
  );
};
let repair = (~items, model) => {
  validate_items(items, model);
  let previous = ref(-. float_of_int(model.min_distance));
  let values =
    Array.mapi(
      (i, item: thumb) => {
        let lo = previous^ +. float_of_int(model.min_distance);
        let hi =
          model.ticks
          -. float_of_int(Array.length(items) - 1 - i)
          *. float_of_int(model.min_distance);
        let value =
          clamp(
            lo,
            hi,
            tick(model, find(item.key, model.values, item.initial)),
          );
        previous := value;
        (item.key, number(model, value));
      },
      items,
    )
    |> Array.to_list;
  let drag =
    switch (model.drag) {
    | Some(_) when model.disabled || Array.length(items) == 0 => None
    | Some({ key: Some(key), _ }) =>
      if (Array.exists(
            (item: thumb) => String.equal(item.key, key) && !item.disabled,
            items,
          )) {
        model.drag;
      } else {
        None;
      }
    | _ => model.drag
    };
  {
    ...model,
    values,
    drag,
  };
};
let init =
    (
      ~id,
      ~items,
      ~min=0.,
      ~max=100.,
      ~step=1.,
      ~min_distance=0,
      ~large_step=10,
      ~orientation=Horizontal,
      ~dir=Ltr,
      ~disabled=false,
      (),
    ) => {
  if (!finite(min)
      || !finite(max)
      || !finite(step)
      || step <= 0.
      || min >= max
      || min_distance < 0
      || large_step < 1) {
    invalid_arg("Slider: invalid configuration");
  };
  let precision = Stdlib.max(decimal_places(min), decimal_places(step));
  let ticks = floor((max -. min) /. step +. 1e-9);
  if (precision > 100
      || !finite(ticks)
      || ticks < 1.
      || ticks > 9007199254740991.
      || min
      +. step == min) {
    invalid_arg("Slider: unrepresentable step/range");
  };
  let aligned_max = float_of_string(fixed(min +. ticks *. step, precision));
  let ticks = aligned_max > max ? ticks -. 1. : ticks;
  if (ticks < 1.) {
    invalid_arg("Slider: grid has no complete step");
  };
  repair(
    ~items,
    {
      id,
      min,
      max,
      step,
      ticks,
      precision,
      min_distance,
      large_step,
      orientation,
      dir,
      disabled,
      values: [],
      drag: None,
      pending: [],
      serial: 0,
    },
  );
};
let values = (~items, model) => repair(~items, model).values;
let set_values = (~items, ~values, model) => {
  List.iter(
    ((_, value)) =>
      if (!finite(value)) {
        invalid_arg("Slider: nonfinite value");
      },
    values,
  );
  (
    repair(
      ~items,
      {
        ...model,
        values,
        drag: None,
        pending: [],
      },
    ),
    Cmd.none,
  );
};
let set_disabled = (disabled, model) => (
  {
    ...model,
    disabled,
    drag: None,
    pending: [],
  },
  Cmd.none,
);
let bounds = (model, items: array(thumb), index) => {
  let gap = float_of_int(model.min_distance);
  let lo =
    index == 0
      ? 0.
      : tick(model, find(items[index - 1].key, model.values, model.min))
        +. gap;
  let hi =
    index == Array.length(items) - 1
      ? model.ticks
      : tick(model, find(items[index + 1].key, model.values, model.max))
        -. gap;
  (lo, hi);
};
let change = (model, items: array(thumb), key, target) => {
  let index = ref(None);
  Array.iteri(
    (i, item: thumb) =>
      if (String.equal(item.key, key) && !item.disabled) {
        index := Some(i);
      },
    items,
  );
  switch (index^) {
  | None => (model, [])
  | Some(i) =>
    let (lo, hi) = bounds(model, items, i);
    let value = number(model, clamp(lo, hi, target));
    let before = find(key, model.values, model.min);
    if (value == before) {
      (model, []);
    } else {
      let values =
        List.map(
          ((k, v)) => (k, String.equal(k, key) ? value : v),
          model.values,
        );
      (
        {
          ...model,
          values,
        },
        [Changed(values)],
      );
    };
  };
};
let delta = (model, event: Ui_keyboard.event) => {
  let amount = event.shift ? model.large_step : 1;
  switch (event.key) {
  | "ArrowUp" => Some(float_of_int(amount))
  | "ArrowDown" => Some(-. float_of_int(amount))
  | "PageUp" => Some(float_of_int(model.large_step))
  | "PageDown" => Some(-. float_of_int(model.large_step))
  | key =>
    Option.map(
      d => float_of_int(d * amount),
      Ui_nav.horizontal(~dir=model.dir == Rtl ? Ui_nav.Rtl : Ui_nav.Ltr, key),
    )
  };
};
let measure = model =>
  Task.attempt(
    result => Measured(model.serial, result),
    Dom.get_element(Ui_attr.part_id(model.id, "track")),
  );
let position = (model, rect: Dom.rect, point) => {
  let fraction =
    switch (model.orientation) {
    | Horizontal =>
      rect.width <= 0. ? 0. : (point.x -. rect.left) /. rect.width
    | Vertical =>
      rect.height <= 0. ? 0. : (rect.bottom -. point.y) /. rect.height
    };
  let fraction =
    model.orientation == Horizontal && model.dir == Rtl
      ? 1. -. fraction : fraction;
  clamp(0., model.ticks, fraction *. (model.max -. model.min) /. model.step);
};
let nearest = (model, items: array(thumb), target) => {
  let best = ref(None);
  Array.iteri(
    (i, item: thumb) =>
      if (!item.disabled) {
        let current =
          tick(model, find(item.key, model.values, item.initial));
        let distance = abs_float(target -. current);
        let (lo, hi) = bounds(model, items, i);
        let can_move =
          target > current ? hi > current : target < current && lo < current;
        switch (best^) {
        | None => best := Some((item.key, distance, can_move))
        | Some((_, d, movable))
            when distance < d || distance == d && can_move && !movable =>
          best := Some((item.key, distance, can_move))
        | _ => ()
        };
      },
    items,
  );
  Option.map(((key, _, _)) => key, best^);
};
let apply_drag = (model, items, drag) =>
  switch (drag.rect) {
  | None => (
      {
        ...model,
        drag: Some(drag),
      },
      Cmd.none,
      [],
    )
  | Some(rect) =>
    let target = position(model, rect, drag.point);
    let key =
      switch (drag.key) {
      | Some(_) as key => key
      | None => nearest(model, items, target)
      };
    switch (key) {
    | None => (
        {
          ...model,
          drag: None,
        },
        Cmd.none,
        [],
      )
    | Some(key) =>
      let (next, out) = change(model, items, key, round(target +. 1e-9));
      let changed = drag.changed || out != [];
      let command =
        drag.key == None
          ? Dom.focus_cmd(Ui_attr.item_id(model.id, key)) : Cmd.none;
      (
        {
          ...next,
          drag:
            drag.ended
              ? None
              : Some({
                  ...drag,
                  key: Some(key),
                  changed,
                }),
        },
        command,
        drag.ended && changed ? out @ [Committed(next.values)] : out,
      );
    };
  };
let update = (~items, message, model) => {
  let model = repair(~items, model);
  switch (message) {
  | Key(key, event) when !model.disabled && Ui_keyboard.plain(event) =>
    let index = ref(None);
    Array.iteri(
      (i, item: thumb) =>
        if (String.equal(item.key, key) && !item.disabled) {
          index := Some(i);
        },
      items,
    );
    switch (index^) {
    | None => (model, Cmd.none, [])
    | Some(i) =>
      let (lo, hi) = bounds(model, items, i);
      let current = tick(model, find(key, model.values, model.min));
      let target =
        switch (event.key) {
        | "Home" => lo
        | "End" => hi
        | _ => current +. Option.value(~default=0., delta(model, event))
        };
      let (next, out) = change(model, items, key, target);
      (next, Cmd.none, out == [] ? [] : out @ [Committed(next.values)]);
    };
  | Down(point, capture)
      when
        !model.disabled
        && (
          switch (model.drag) {
          | None => true
          | Some(drag) => drag.ended
          }
        )
        && Array.exists((item: thumb) => !item.disabled, items) =>
    let model = {
      ...model,
      serial: model.serial + 1,
      pending:
        switch (model.drag) {
        | Some(drag) => model.pending @ [drag]
        | None => model.pending
        },
      drag:
        Some({
          serial: model.serial + 1,
          point,
          key: None,
          rect: None,
          ended: false,
          changed: false,
          capture,
        }),
    };
    (model, measure(model), []);
  | Measured(serial, result) =>
    switch (model.drag, result) {
    | (Some(drag), Ok(info)) when drag.serial == serial =>
      apply_drag(
        model,
        items,
        {
          ...drag,
          rect: Some(info.rect),
        },
      )
    | (Some(drag), Error(_)) when drag.serial == serial => (
        {
          ...model,
          drag: None,
        },
        Cmd.none,
        [],
      )
    | _ =>
      let waiting =
        List.find_opt((drag: drag) => drag.serial == serial, model.pending);
      let pending =
        List.filter((drag: drag) => drag.serial != serial, model.pending);
      switch (waiting, result) {
      | (Some(drag), Ok(info)) when !model.disabled =>
        let (next, command, out) =
          apply_drag(
            {
              ...model,
              pending,
            },
            items,
            {
              ...drag,
              rect: Some(info.rect),
            },
          );
        (
          {
            ...next,
            drag: model.drag,
          },
          command,
          out,
        );
      | _ => (
          {
            ...model,
            pending,
          },
          Cmd.none,
          [],
        )
      };
    }
  | Move(serial, point)
  | End(serial, point) =>
    switch (model.drag) {
    | Some(drag)
        when
          !drag.ended
          && serial == drag.serial
          && point.pointer == drag.point.pointer =>
      apply_drag(
        model,
        items,
        {
          ...drag,
          point,
          ended:
            switch (message) {
            | End(_, _) => true
            | _ => false
            },
        },
      )
    | _ => (model, Cmd.none, [])
    }
  | Cancel(serial) =>
    switch (model.drag) {
    | Some(drag) when drag.serial == serial => (
        {
          ...model,
          drag: None,
        },
        Cmd.none,
        drag.changed ? [Committed(model.values)] : [],
      )
    | _ => (model, Cmd.none, [])
    }
  | Measure => (model, model.drag == None ? Cmd.none : measure(model), [])
  | _ => (model, Cmd.none, [])
  };
};
/* Private document sources cover release outside the track and pointer capture
   retargeting. Install synchronously after Down, before the deferred rect read. */
module B = {
  type event;
  type target;
  type frame =
    | Animation(int)
    | Timer(int);
  external global: Js.Dict.t(Js.Json.t) = "globalThis";
  external raf: (float => unit) => int = "requestAnimationFrame";
  external caf: int => unit = "cancelAnimationFrame";
  external timeout: (unit => unit, int) => int = "setTimeout";
  external clear: int => unit = "clearTimeout";
  let schedule = callback =>
    switch (Js.Dict.get(global, "requestAnimationFrame")) {
    | Some(f) when Js.typeof(f) == "function" =>
      Animation(raf(_ => callback()))
    | _ => Timer(timeout(callback, 16))
    };
  let cancel = frame =>
    switch (frame) {
    | Animation(id) => caf(id)
    | Timer(id) => clear(id)
    };
  external document: target = "document";
  external window: target = "window";
  [@mel.send]
  external listen: (target, string, event => unit, bool) => unit =
    "addEventListener";
  [@mel.send]
  external unlisten: (target, string, event => unit, bool) => unit =
    "removeEventListener";
  [@mel.get] external x: event => float = "clientX";
  [@mel.get] external y: event => float = "clientY";
  [@mel.get] external pointer: event => int = "pointerId";
  [@mel.get] external event_target: event => target = "target";
  [@mel.get_index] external get: (target, string) => Js.Json.t;
  [@mel.send]
  external find: (target, string) => Js.Nullable.t(target) = "getElementById";
  [@mel.send] external capture: (target, int) => unit = "setPointerCapture";
  [@mel.send]
  external has_capture: (target, int) => bool = "hasPointerCapture";
  [@mel.send]
  external release: (target, int) => unit = "releasePointerCapture";
};
let subscriptions = model =>
  switch (model.drag) {
  | None => Sub.none
  | Some(drag) when drag.ended => Sub.none
  | Some(drag) =>
    Sub.batch([
      Events.on_scroll(Measure),
      Events.on_resize(_ => Measure),
      Sub.make(
        ~home="Tea_ui.Slider.drag",
        ~key=model.id ++ ":" ++ string_of_int(drag.serial),
        ~tagger=x => x,
        ~start=
          dispatch => {
            let captured =
              if (drag.capture) {
                switch (
                  Js.Nullable.toOption(
                    B.find(B.document, Ui_attr.part_id(model.id, "track")),
                  )
                ) {
                | Some(track)
                    when
                      Js.typeof(B.get(track, "setPointerCapture"))
                      == "function" =>
                  B.capture(track, drag.point.pointer);
                  Some(track);
                | _ => None
                };
              } else {
                None;
              };
            let pending = ref(None);
            let frame = ref(None);
            let alive = ref(true);
            let cancel_frame = () => {
              Option.iter(B.cancel, frame^);
              frame := None;
            };
            let point = event => {
              pointer: B.pointer(event),
              x: B.x(event),
              y: B.y(event),
            };
            let move = event =>
              if (B.pointer(event) == drag.point.pointer) {
                pending := Some(point(event));
                if (frame^ == None) {
                  frame :=
                    Some(
                      B.schedule(() => {
                        frame := None;
                        if (alive^) {
                          Option.iter(
                            p => dispatch(Move(drag.serial, p)),
                            pending^,
                          );
                          pending := None;
                        };
                      }),
                    );
                };
              };
            let end_ = event =>
              if (B.pointer(event) == drag.point.pointer) {
                cancel_frame();
                pending := None;
                dispatch(End(drag.serial, point(event)));
              };
            let cancel = event =>
              if (B.pointer(event) == drag.point.pointer) {
                cancel_frame();
                dispatch(Cancel(drag.serial));
              };
            let blur = event =>
              if (B.event_target(event) === B.window) {
                dispatch(Cancel(drag.serial));
              };
            let lost_capture = event =>
              Option.iter(
                track =>
                  if (B.event_target(event) === track) {
                    cancel(event);
                  },
                captured,
              );
            B.listen(B.document, "lostpointercapture", lost_capture, true);
            B.listen(B.document, "pointermove", move, true);
            B.listen(B.document, "pointerup", end_, true);
            B.listen(B.document, "pointercancel", cancel, true);
            B.listen(B.window, "blur", blur, true);
            () => {
              alive := false;
              cancel_frame();
              B.unlisten(B.document, "pointermove", move, true);
              B.unlisten(B.document, "pointerup", end_, true);
              B.unlisten(B.document, "pointercancel", cancel, true);
              B.unlisten(B.window, "blur", blur, true);
              B.unlisten(
                B.document,
                "lostpointercapture",
                lost_capture,
                true,
              );
              Option.iter(
                track =>
                  if (B.has_capture(track, drag.point.pointer)) {
                    B.release(track, drag.point.pointer);
                  },
                captured,
              );
            };
          },
      ),
    ])
  };
type thumb_parts('parent) = {
  key: string,
  value: float,
  percent: float,
  root: list(Html.attr('parent)),
  input: list(Html.attr('parent)),
};
type parts('parent) = {
  root: list(Html.attr('parent)),
  track: list(Html.attr('parent)),
  range: list(Html.attr('parent)),
  thumbs: array(thumb_parts('parent)),
};
let view =
    (
      ~model,
      ~items,
      ~to_parent,
      ~name=?,
      ~format_value=(_, value) => Js.Float.toString(value),
      ~on_key_down=_ => None,
      ~to_view,
      (),
    ) => {
  let model = repair(~items, model);
  let orientation =
    model.orientation == Horizontal ? "horizontal" : "vertical";
  let common = [
    Html.data("orientation", orientation),
    Ui_attr.data_flag("disabled", model.disabled),
    Ui_attr.data_flag("dragging", model.drag != None),
  ];
  let percent = value =>
    (value -. model.min) /. (model.max -. model.min) *. 100.;
  let thumbs =
    Array.mapi(
      (i, item: thumb) => {
        let value = find(item.key, model.values, item.initial);
        let (lo, hi) = bounds(model, items, i);
        let disabled = model.disabled || item.disabled;
        let keydown = event =>
          if (!Ui_keyboard.plain(event) || event.key == "Tab") {
            None;
          } else if (event.key == "Home"
                     || event.key == "End"
                     || delta(model, event) != None) {
            Some((
              disabled ? None : Some(Key(item.key, event) |> to_parent),
              true,
            ));
          } else {
            Option.map(msg => (Some(msg), false), on_key_down(event.key));
          };
        {
          key: item.key,
          value,
          percent: percent(value),
          root: [
            Html.id(Ui_attr.item_id(model.id, item.key)),
            Html.role("slider"),
            Html.tabindex(disabled ? (-1) : 0),
            Html.aria("valuenow", Js.Float.toString(value)),
            Html.aria("valuemin", Js.Float.toString(number(model, lo))),
            Html.aria("valuemax", Js.Float.toString(number(model, hi))),
            Html.aria("valuetext", format_value(item.key, value)),
            Html.aria("orientation", orientation),
            Ui_attr.aria_bool("disabled", disabled),
            switch (item.label) {
            | Label(label) => Html.aria("label", label)
            | Labelled_by(ids) => Ui_attr.aria_ids("labelledby", ids)
            },
            Html.style(
              "--tea-slider-percent",
              Js.Float.toString(percent(value)) ++ "%",
            ),
            Html.data("orientation", orientation),
            Ui_attr.data_flag("disabled", disabled),
            Ui_attr.data_flag(
              "dragging",
              switch (model.drag) {
              | Some({ key: Some(key), _ }) => String.equal(key, item.key)
              | _ => false
              },
            ),
            Html.on(
              "keydown",
              V.Custom_optional(Ui_keyboard.custom(keydown)),
            ),
          ],
          input: [
            Html.type_("hidden"),
            Html.value(Js.Float.toString(value)),
            Html.disabled(disabled),
            switch (name) {
            | None => Html.none
            | Some(name) =>
              Html.name(
                Array.length(items) == 1
                  ? name : name ++ "[" ++ item.key ++ "]",
              )
            },
          ],
        };
      },
      items,
    );
  let down =
    D.and_then(
      button =>
        if (button != 0
            || model.disabled
            || !Array.exists((item: thumb) => !item.disabled, items)) {
          D.fail("inactive pointer");
        } else {
          D.map2(
            (point, trusted) => (to_parent(Down(point, trusted)), true),
            D.map3(
              (pointer, x, y) =>
                {
                  pointer,
                  x,
                  y,
                },
              D.one_of([D.field("pointerId", D.int), D.succeed(0)]),
              D.field("clientX", D.float),
              D.field("clientY", D.float),
            ),
            D.one_of([D.field("isTrusted", D.bool), D.succeed(false)]),
          );
        },
      D.field("button", D.int),
    );
  let start = Array.length(thumbs) <= 1 ? 0. : thumbs[0].percent;
  let end_ =
    Array.length(thumbs) == 0
      ? 0. : thumbs[Array.length(thumbs) - 1].percent;
  to_view({
    root:
      [
        Html.id(model.id),
        Html.attribute("dir", model.dir == Rtl ? "rtl" : "ltr"),
      ]
      @ common,
    track:
      [
        Html.id(Ui_attr.part_id(model.id, "track")),
        Html.style("touch-action", "none"),
        Html.prevent_default_on("pointerdown", down),
      ]
      @ common,
    range:
      [
        Html.style("--tea-slider-start", Js.Float.toString(start) ++ "%"),
        Html.style("--tea-slider-end", Js.Float.toString(end_) ++ "%"),
      ]
      @ common,
    thumbs,
  });
};
