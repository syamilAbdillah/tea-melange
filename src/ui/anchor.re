module Html = Tea_html.Html;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
type side =
  | Top
  | Bottom
  | Left
  | Right;
type alignment =
  | Start
  | Center
  | End;
type placement = (side, alignment);
type direction =
  | Ltr
  | Rtl;
type rect = {
  x: float,
  y: float,
  width: float,
  height: float,
};
type size = {
  width: float,
  height: float,
};
type hidden_reason =
  | Reference_outside_viewport
  | Reference_empty;
type position = {
  x: float,
  y: float,
  width: float,
  height: float,
  placement,
  anchor_width: float,
  anchor_height: float,
  available_width: float,
  available_height: float,
  reference_hidden: bool,
  hidden_reason: option(hidden_reason),
  arrow_offset: option(float),
};
let finite = value =>
  switch (classify_float(value)) {
  | FP_nan
  | FP_infinite => 0.
  | _ => value
  };
let positive = value => max(0., finite(value));
let clamp = (low, high, value) => max(low, min(max(low, high), value));
let normalize_rect = (r: rect): rect => {
  x: finite(r.x),
  y: finite(r.y),
  width: positive(r.width),
  height: positive(r.height),
};
let opposite = side =>
  switch (side) {
  | Top => Bottom
  | Bottom => Top
  | Left => Right
  | Right => Left
  };
let vertical = side =>
  switch (side) {
  | Top
  | Bottom => true
  | _ => false
  };
let placement_name = ((side, alignment)) =>
  (
    switch (side) {
    | Top => "top"
    | Bottom => "bottom"
    | Left => "left"
    | Right => "right"
    }
  )
  ++ "-"
  ++ (
    switch (alignment) {
    | Start => "start"
    | Center => "center"
    | End => "end"
    }
  );
let compute =
    (
      ~anchor,
      ~floating: size,
      ~viewport,
      ~placement,
      ~offset=0.,
      ~flip=true,
      ~shift_padding=Some(8.),
      ~match_width=false,
      ~dir=Ltr,
      ~arrow_size=?,
      (),
    ) => {
  let anchor = normalize_rect(anchor);
  let viewport = normalize_rect(viewport);
  let width = match_width ? anchor.width : positive(floating.width);
  let height = positive(floating.height);
  let offset = finite(offset);
  let padding =
    Option.value(~default=0., Option.map(positive, shift_padding));
  let px = min(padding, viewport.width /. 2.);
  let py = min(padding, viewport.height /. 2.);
  let room = side =>
    switch (side) {
    | Top =>
      clamp(
        0.,
        viewport.height -. 2. *. py,
        anchor.y -. offset -. viewport.y -. py,
      )
    | Bottom =>
      clamp(
        0.,
        viewport.height -. 2. *. py,
        viewport.y
        +. viewport.height
        -. py
        -. anchor.y
        -. anchor.height
        -. offset,
      )
    | Left =>
      clamp(
        0.,
        viewport.width -. 2. *. px,
        anchor.x -. offset -. viewport.x -. px,
      )
    | Right =>
      clamp(
        0.,
        viewport.width -. 2. *. px,
        viewport.x
        +. viewport.width
        -. px
        -. anchor.x
        -. anchor.width
        -. offset,
      )
    };
  let (preferred, alignment) = placement;
  let other = opposite(preferred);
  let extent = vertical(preferred) ? height : width;
  let side =
    flip && room(preferred) < extent && room(other) > room(preferred)
      ? other : preferred;
  let v = vertical(side);
  let alignment =
    switch (alignment, dir) {
    | (Start, Rtl) when v => End
    | (End, Rtl) when v => Start
    | _ => alignment
    };
  let cross_start = v ? anchor.x : anchor.y;
  let cross_anchor = v ? anchor.width : anchor.height;
  let cross_size = v ? width : height;
  let cross =
    cross_start
    +. (
      switch (alignment) {
      | Start => 0.
      | Center => (cross_anchor -. cross_size) /. 2.
      | End => cross_anchor -. cross_size
      }
    );
  let cross =
    switch (shift_padding) {
    | None => cross
    | Some(_) =>
      v
        ? clamp(
            viewport.x +. px,
            viewport.x +. viewport.width -. px -. width,
            cross,
          )
        : clamp(
            viewport.y +. py,
            viewport.y +. viewport.height -. py -. height,
            cross,
          )
    };
  let (x, y) =
    switch (side) {
    | Top => (cross, anchor.y -. offset -. height)
    | Bottom => (cross, anchor.y +. anchor.height +. offset)
    | Left => (anchor.x -. offset -. width, cross)
    | Right => (anchor.x +. anchor.width +. offset, cross)
    };
  let hidden_reason =
    if (anchor.width <= 0. || anchor.height <= 0.) {
      Some(Reference_empty);
    } else if (viewport.width <= 0.
               || viewport.height <= 0.
               || anchor.x
               +. anchor.width <= viewport.x
               || anchor.x >= viewport.x
               +. viewport.width
               || anchor.y
               +. anchor.height <= viewport.y
               || anchor.y >= viewport.y
               +. viewport.height) {
      Some(Reference_outside_viewport);
    } else {
      None;
    };
  let (_, original_alignment) = placement;
  {
    x,
    y,
    width,
    height,
    placement: (side, original_alignment),
    anchor_width: anchor.width,
    anchor_height: anchor.height,
    available_width: v ? max(0., viewport.width -. 2. *. px) : room(side),
    available_height: v ? room(side) : max(0., viewport.height -. 2. *. py),
    reference_hidden: Option.is_some(hidden_reason),
    hidden_reason,
    arrow_offset:
      Option.map(
        arrow_size =>
          clamp(
            0.,
            cross_size -. positive(arrow_size),
            cross_start
            +. cross_anchor
            /. 2.
            -. cross
            -. positive(arrow_size)
            /. 2.,
          ),
        arrow_size,
      ),
  };
};
/* Manual popovers put the floating element in the top layer. */
module Popover = {
  let supported = element =>
    Ui_dom.has_method(element, "showPopover")
    && Ui_dom.has_method(element, "hidePopover");
  let close = element =>
    if (supported(element)
        && Ui_dom.connected(element)
        && Ui_dom.matches(element, ":popover-open")) {
      Ui_dom.hide_popover(element);
    };
};
type options = {
  placement,
  offset: float,
  flip: bool,
  shift_padding: option(float),
  match_width: bool,
  dir: direction,
  top_layer: bool,
  arrow_size: option(float),
};
type model = {
  anchor_id: string,
  floating_id: string,
  options,
  active: bool,
  revision: int,
  position: option(position),
  ratio: float,
};
type msg =
  | Measured(int, option(position), float);
type out_msg =
  | Positioned(position)
  | Unavailable;
let init =
    (
      ~anchor_id,
      ~floating_id,
      ~placement=(Bottom, Start),
      ~offset=0.,
      ~flip=true,
      ~shift_padding=Some(8.),
      ~match_width=false,
      ~dir=Ltr,
      ~top_layer=false,
      ~arrow_size=?,
      (),
    ) => {
  anchor_id,
  floating_id,
  options: {
    placement,
    offset,
    flip,
    shift_padding,
    match_width,
    dir,
    top_layer,
    arrow_size,
  },
  active: false,
  revision: 0,
  position: None,
  ratio: 1.,
};
let active = model => model.active;
let position = model => model.position;
let placed = position =>
  switch (position) {
  | Some(p) => !p.reference_hidden
  | None => false
  };
let invalidate = model => {
  ...model,
  revision: model.revision + 1,
  position: None,
};
let start = model => (
  model.active
    ? model
    : invalidate({
        ...model,
        active: true,
      }),
  Cmd.none,
);
let stop = model => (
  model.active
    ? invalidate({
        ...model,
        active: false,
      })
    : model,
  Cmd.none,
);
let refresh = model => (invalidate(model), Cmd.none);
let configure =
    (
      ~placement=?,
      ~offset=?,
      ~flip=?,
      ~shift_padding=?,
      ~match_width=?,
      ~dir=?,
      ~top_layer=?,
      ~arrow_size=?,
      model,
    ) => {
  let o = model.options;
  (
    invalidate({
      ...model,
      options: {
        placement: Option.value(~default=o.placement, placement),
        offset: Option.value(~default=o.offset, offset),
        flip: Option.value(~default=o.flip, flip),
        shift_padding: Option.value(~default=o.shift_padding, shift_padding),
        match_width: Option.value(~default=o.match_width, match_width),
        dir: Option.value(~default=o.dir, dir),
        top_layer: Option.value(~default=o.top_layer, top_layer),
        arrow_size: Option.value(~default=o.arrow_size, arrow_size),
      },
    }),
    Cmd.none,
  );
};
let update = (Measured(revision, position, ratio), model) =>
  if (!model.active || revision != model.revision) {
    (model, Cmd.none, []);
  } else {
    (
      {
        ...model,
        position,
        ratio,
      },
      Cmd.none,
      [
        switch (position) {
        | Some(p) => Positioned(p)
        | None => Unavailable
        },
      ],
    );
  };
/* Resize/scroll sources run inside this one leaf and call [measure]
   directly, so a scroll burst does not round-trip through update. */
let subscriptions = model =>
  !model.active
    ? Sub.none
    : Sub.make(
        ~home="Tea_ui.Anchor",
        ~key=
          Ui_sub.prefix(model.anchor_id)
          ++ model.floating_id
          ++ ":"
          ++ string_of_int(model.revision),
        ~tagger=x => x,
        ~start=
          dispatch => {
            let alive = ref(true);
            let pending = ref(false);
            let first = ref(true);
            let previous = ref(model.position);
            let previous_ratio = ref(model.ratio);
            let promoted = ref(None);
            let measure = () =>
              if (alive^ && ! pending^) {
                pending := true;
                let anchor = ref(None);
                let floating = ref(None);
                let finish = () =>
                  switch (anchor^, floating^) {
                  | (Some(a), Some(f)) =>
                    pending := false;
                    if (alive^) {
                      let o = model.options;
                      let p =
                        switch (a, f) {
                        | (
                            Ok(a: Browser.element_info),
                            Ok(f: Browser.element_info),
                          ) =>
                          Some(
                            compute(
                              ~anchor={
                                x: a.rect.left,
                                y: a.rect.top,
                                width: a.rect.width,
                                height: a.rect.height,
                              },
                              ~floating={
                                width: f.rect.width,
                                height: f.rect.height,
                              },
                              ~viewport={
                                x: 0.,
                                y: 0.,
                                width: a.viewport_width,
                                height: a.viewport_height,
                              },
                              ~placement=o.placement,
                              ~offset=o.offset,
                              ~flip=o.flip,
                              ~shift_padding=o.shift_padding,
                              ~match_width=o.match_width,
                              ~dir=o.dir,
                              ~arrow_size=?o.arrow_size,
                              (),
                            ),
                          )
                        | _ => None
                        };
                      let ratio = finite(Ui_dom.pixel_ratio(Ui_dom.window));
                      let ratio = ratio > 0. ? ratio : 1.;
                      if (first^ || previous^ != p || ratio != previous_ratio^) {
                        first := false;
                        previous := p;
                        previous_ratio := ratio;
                        dispatch(Measured(model.revision, p, ratio));
                      };
                    };
                  | _ => ()
                  };
                Browser.get_element(
                  model.anchor_id,
                  result => {
                    anchor := Some(result);
                    finish();
                  },
                );
                Browser.get_element(
                  model.floating_id,
                  result => {
                    floating := Some(result);
                    finish();
                  },
                );
              };
            Tea_dom.Browser_platform.request_animation_frame(_ =>
              if (alive^ && model.options.top_layer) {
                Option.iter(
                  element =>
                    if (Popover.supported(element)
                        && Ui_dom.connected(element)) {
                      promoted := Some(element);
                      if (!Ui_dom.matches(element, ":popover-open")) {
                        Ui_dom.show_popover(element);
                      };
                      /* beforetoggle may synchronously stop this lifecycle
                         before showPopover finishes entering the top layer. */
                      if (! alive^) {
                        Popover.close(element);
                      };
                    },
                  Ui_dom.find(model.floating_id),
                );
              }
            );
            measure();
            let dispose =
              Ui_sub.connect(
                Sub.batch([
                  Events.on_resize(_ => ()),
                  Events.on_scroll(),
                  Events.on_element_resize(~id=model.anchor_id, _ => ()),
                  Events.on_element_resize(~id=model.floating_id, _ => ()),
                ]),
                measure,
              );
            () => {
              alive := false;
              dispose();
              Option.iter(Popover.close, promoted^);
            };
          },
      );
let attrs = model => {
  let p = model.position;
  let value = f => Option.value(~default=0., Option.map(f, p));
  let px = value => Js.Float.toString(value) ++ "px";
  let rounded = value => Float.round(value *. model.ratio) /. model.ratio;
  [
    Html.id(model.floating_id),
    Html.hidden(!model.active),
    model.options.top_layer ? Html.attribute("popover", "manual") : Html.none,
    Html.data(
      "placement",
      placement_name(
        Option.value(
          ~default=model.options.placement,
          Option.map((p: position) => p.placement, p),
        ),
      ),
    ),
    Ui_attr.data_flag("reference-hidden", Option.is_some(p) && !placed(p)),
    Html.style("position", "fixed"),
    Html.style("inset", "auto"),
    Html.style("margin", "0"),
    Html.style("box-sizing", "border-box"),
    Html.style("left", px(rounded(value(p => p.x)))),
    Html.style("top", px(rounded(value(p => p.y)))),
    Html.style(
      "width",
      model.options.match_width && Option.is_some(p)
        ? px(value(p => p.width)) : "",
    ),
    Html.style(
      "visibility",
      model.active && placed(p) ? "visible" : "hidden",
    ),
    Html.style("--tea-anchor-width", px(value(p => p.anchor_width))),
    Html.style("--tea-anchor-height", px(value(p => p.anchor_height))),
    Html.style("--tea-available-width", px(value(p => p.available_width))),
    Html.style("--tea-available-height", px(value(p => p.available_height))),
  ];
};
type parts('parent) = {
  floating: list(Html.attr('parent)),
  position: option(position),
};
let view = (~model, ~to_parent as _, ~to_view, ()) =>
  to_view({
    floating: attrs(model),
    position: model.position,
  });
