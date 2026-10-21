module Html = Tea_html.Html;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module D = Tea_core.Decode;
module Sizes = Map.Make(String);
let finite = x => Js.Float.isFinite(x);
let positive = x => finite(x) && x > 0. ? x : 1.;
let nonnegative = x => finite(x) ? max(0., x) : 0.;
module Geometry = {
  type tree =
    | Empty
    | Node(float, tree, tree);
  type t = {
    count: int,
    estimate: float,
    tree,
  };
  type range = {
    first: int,
    last: int,
    operations: int,
  };
  let create = (~count, ~estimate) => {
    count: max(0, count),
    estimate: positive(estimate),
    tree: Empty,
  };
  let delta = t =>
    switch (t) {
    | Empty => 0.
    | Node(sum, _, _) => sum
    };
  let children = t =>
    switch (t) {
    | Empty => (Empty, Empty)
    | Node(_, left, right) => (left, right)
    };
  let rec set = (tree, lo, hi, index, value) =>
    if (hi - lo == 1) {
      value == 0. ? Empty : Node(value, Empty, Empty);
    } else {
      let mid = lo + (hi - lo) / 2;
      let (left, right) = children(tree);
      let (left, right) =
        index < mid
          ? (set(left, lo, mid, index, value), right)
          : (left, set(right, mid, hi, index, value));
      switch (left, right) {
      | (Empty, Empty) => Empty
      | _ => Node(delta(left) +. delta(right), left, right)
      };
    };
  let measure = (~index, ~size, t) =>
    index < 0 || index >= t.count || !finite(size) || size <= 0.
      ? t
      : {
        ...t,
        tree: set(t.tree, 0, t.count, index, size -. t.estimate),
      };
  let rec prefix = (tree, lo, hi, index) =>
    if (index <= lo) {
      0.;
    } else if (index >= hi) {
      delta(tree);
    } else {
      let (left, right) = children(tree);
      let mid = lo + (hi - lo) / 2;
      prefix(left, lo, mid, index) +. prefix(right, mid, hi, index);
    };
  let offset = (~index, t) => {
    let index = max(0, min(t.count, index));
    float_of_int(index) *. t.estimate +. prefix(t.tree, 0, t.count, index);
  };
  let total = t => float_of_int(t.count) *. t.estimate +. delta(t.tree);
  let locate = (y, t) => {
    let operations = ref(0);
    let rec walk = (tree, lo, hi, y) => {
      incr(operations);
      if (hi - lo <= 1) {
        lo;
      } else {
        let mid = lo + (hi - lo) / 2;
        let (left, right) = children(tree);
        let size = float_of_int(mid - lo) *. t.estimate +. delta(left);
        y < size ? walk(left, lo, mid, y) : walk(right, mid, hi, y -. size);
      };
    };
    let index = t.count == 0 ? 0 : walk(t.tree, 0, t.count, max(0., y));
    (index, operations^);
  };
  let range = (~viewport, ~scroll, ~overscan=2, t) =>
    if (t.count == 0 || nonnegative(viewport) == 0.) {
      {
        first: 0,
        last: 0,
        operations: 0,
      };
    } else {
      let scroll =
        min(
          nonnegative(scroll),
          max(0., total(t) -. nonnegative(viewport)),
        );
      let (first, a) = locate(scroll, t);
      let (end_, b) = locate(scroll +. nonnegative(viewport), t);
      {
        /* End is exclusive, including a boundary row is intentional overscan. */
        first: max(0, first - max(0, overscan)),
        last: end_ + 1 + min(max(0, overscan), t.count - end_ - 1),
        operations: a + b,
      };
    };
};
type items = {
  token: int,
  keys: array(string),
  indices: Js.Dict.t(int),
};
/* Arbitrary keys (including __proto__/constructor) must not touch a prototype. */
[@mel.scope "Object"]
external empty_indices: Js.Nullable.t(unit) => Js.Dict.t(int) = "create";
let serial = ref(0);
let items = keys => {
  let keys = Array.copy(keys);
  let indices = empty_indices(Js.Nullable.null);
  Array.iteri(
    (index, key) => {
      if (Js.Dict.get(indices, key) != None) {
        invalid_arg("Virtual_list.items: duplicate key " ++ key);
      };
      Js.Dict.set(indices, key, index);
    },
    keys,
  );
  incr(serial);
  {
    token: serial^,
    keys,
    indices,
  };
};
let count = items => Array.length(items.keys);
let key_at = (~index, items) =>
  index < 0 || index >= count(items) ? None : Some(items.keys[index]);
type row_size =
  | Fixed(float)
  | Estimated(float);
type align =
  | Start
  | Center
  | End
  | Auto;
type target = {
  key: string,
  align,
};
type model = {
  id: string,
  token: int,
  generation: int,
  row_size,
  geometry: Geometry.t,
  measured: Sizes.t(float),
  scroll: float,
  viewport: float,
  overscan: int,
  target: option(target),
};
type sample = {
  top: float,
  height: float,
  sizes: list((string, float)),
  scrolled: bool,
  generation: int,
};
type msg =
  | Frame(sample);
type out_msg =
  | Viewport_changed(Geometry.range);
let estimate = size =>
  switch (size) {
  | Fixed(height)
  | Estimated(height) => positive(height)
  };
let init = (~id, ~items: items, ~row_size, ~viewport=0., ~overscan=2, ()) => {
  id,
  token: items.token,
  generation: 0,
  row_size,
  geometry:
    Geometry.create(~count=count(items), ~estimate=estimate(row_size)),
  measured: Sizes.empty,
  scroll: 0.,
  viewport: nonnegative(viewport),
  overscan: max(0, overscan),
  target: None,
};
let scroll_offset = model => model.scroll;
let viewport_height = model => model.viewport;
let total_size = model => Geometry.total(model.geometry);
let measurement_count = model => Sizes.cardinal(model.measured);
let visible_range = model =>
  Geometry.range(
    ~viewport=model.viewport,
    ~scroll=model.scroll,
    ~overscan=model.overscan,
    model.geometry,
  );
let row_offset = (~index, model) => Geometry.offset(~index, model.geometry);
let row_id = (~key, model) => Ui_attr.item_id(model.id, key);
let clamp = (scroll, viewport, geometry) =>
  min(nonnegative(scroll), max(0., Geometry.total(geometry) -. viewport));
let synchronize = (~items: items, model) =>
  if (model.token == items.token) {
    model;
  } else {
    let geometry =
      ref(
        Geometry.create(
          ~count=count(items),
          ~estimate=estimate(model.row_size),
        ),
      );
    let measured =
      Sizes.filter(
        (key, size) =>
          switch (Js.Dict.get(items.indices, key)) {
          | None => false
          | Some(index) =>
            geometry := Geometry.measure(~index, ~size, geometry^);
            true;
          },
        model.measured,
      );
    {
      ...model,
      token: items.token,
      generation: model.generation + 1,
      measured,
      geometry: geometry^,
      scroll: clamp(model.scroll, model.viewport, geometry^),
      target:
        Option.bind(model.target, target =>
          Js.Dict.get(items.indices, target.key) == None
            ? None : Some(target)
        ),
    };
  };
module B = {
  type element;
  type observer;
  type entry;
  type event = Js.Json.t;
  external global: Js.Json.t = "globalThis";
  external document: Dom.document = "document";
  [@mel.get_index] external get: ('a, string) => 'b;
  [@mel.send]
  external find: (Dom.document, string) => Js.Nullable.t(element) =
    "getElementById";
  [@mel.get] external top: element => float = "scrollTop";
  [@mel.set] external set_top: (element, float) => unit = "scrollTop";
  [@mel.get] external height: element => float = "clientHeight";
  [@mel.get] external children: element => array(element) = "children";
  [@mel.send]
  external attr: (element, string) => Js.Nullable.t(string) = "getAttribute";
  [@mel.send]
  external listen: (element, string, event => unit) => unit =
    "addEventListener";
  [@mel.send]
  external unlisten: (element, string, event => unit) => unit =
    "removeEventListener";
  [@mel.new]
  external resize: (array(entry) => unit) => observer = "ResizeObserver";
  [@mel.new]
  external mutations: (unit => unit) => observer = "MutationObserver";
  [@mel.send]
  external observe_size: (observer, element, Js.t({..})) => unit = "observe";
  [@mel.send] external unobserve: (observer, element) => unit = "unobserve";
  [@mel.send]
  external observe_children: (observer, element, Js.t({..})) => unit =
    "observe";
  [@mel.send] external disconnect: observer => unit = "disconnect";
  [@mel.get] external target: entry => element = "target";
  [@mel.send]
  external rect: element => Js.t({. "height": float }) =
    "getBoundingClientRect";
  let after = f => Tea_dom.Browser_platform.request_animation_frame(_ => f());
  let find = id => Js.Nullable.toOption(find(document, id));
};
let scroll_command = (model: model) =>
  Cmd.effect(_ =>
    B.after(() =>
      Option.iter(
        el =>
          if (Js.Nullable.toOption(B.attr(el, "data-tea-virtual-generation"))
              == Some(string_of_int(model.generation))) {
            B.set_top(el, model.scroll);
          },
        B.find(Ui_attr.part_id(model.id, "viewport")),
      )
    )
  );
let aligned = (~index, ~align, model) => {
  let start = row_offset(~index, model);
  let end_ = row_offset(~index=index + 1, model);
  let desired =
    switch (align) {
    | Start => start
    | Center => (start +. end_ -. model.viewport) /. 2.
    | End => end_ -. model.viewport
    | Auto =>
      start < model.scroll
        ? start
        : end_ > model.scroll +. model.viewport
            ? end_ -. start > model.viewport ? start : end_ -. model.viewport
            : model.scroll
    };
  clamp(desired, model.viewport, model.geometry);
};
let scroll_to_index = (~items, ~index, ~align=Auto, model) => {
  let model = synchronize(~items, model);
  switch (key_at(~index, items)) {
  | None => (model, Cmd.none)
  | Some(key) =>
    let model = {
      ...model,
      scroll: aligned(~index, ~align, model),
      generation: model.generation + 1,
      target:
        Some({
          key,
          align,
        }),
    };
    (model, scroll_command(model));
  };
};
let scroll_to_key = (~items, ~key, ~align=Auto, model) =>
  switch (Js.Dict.get(items.indices, key)) {
  | None => (synchronize(~items, model), Cmd.none)
  | Some(index) => scroll_to_index(~items, ~index, ~align, model)
  };
let refresh = (~items, model) => {
  let model = synchronize(~items, model);
  (model, scroll_command(model));
};
let update = (~items, Frame(sample), model) => {
  let model = synchronize(~items, model);
  let viewport = nonnegative(sample.height);
  /* A command invalidates scroll samples captured before its render. Their
     keyed measurements remain valid, but must not undo the newer intent. */
  let stale = sample.generation != model.generation;
  let scroll =
    clamp(stale ? model.scroll : sample.top, viewport, model.geometry);
  let (anchor, _) = Geometry.locate(scroll, model.geometry);
  let within = scroll -. row_offset(~index=anchor, model);
  let geometry = ref(model.geometry);
  let measured = ref(model.measured);
  switch (model.row_size) {
  | Fixed(_) => ()
  | Estimated(_) =>
    List.iter(
      ((key, size)) => {
        switch (Js.Dict.get(items.indices, key)) {
        | Some(index) when finite(size) && size > 0. =>
          geometry := Geometry.measure(~index, ~size, geometry^);
          measured := Sizes.add(key, size, measured^);
        | _ => ()
        }
      },
      sample.sizes,
    )
  };
  let target =
    !stale && sample.scrolled && abs_float(sample.top -. model.scroll) > 1.
      ? None : model.target;
  let model = {
    ...model,
    viewport,
    geometry: geometry^,
    measured: measured^,
    target,
  };
  let desired =
    switch (
      Option.bind(target, target =>
        Option.map(
          index => (target.align, index),
          Js.Dict.get(items.indices, target.key),
        )
      )
    ) {
    | Some((align, index)) => aligned(~index, ~align, model)
    | None =>
      clamp(
        row_offset(~index=anchor, model) +. within,
        viewport,
        model.geometry,
      )
    };
  let model = {
    ...model,
    scroll: desired,
    generation:
      abs_float(desired -. sample.top) > 0.01
        ? model.generation + 1 : model.generation,
  };
  (
    model,
    abs_float(desired -. sample.top) > 0.01
      ? scroll_command(model) : Cmd.none,
    [Viewport_changed(visible_range(model))],
  );
};
let subscriptions = model =>
  Sub.make(
    ~home="Virtual_list.rows",
    ~key=model.id,
    ~tagger=sample => Frame(sample),
    ~start=
      dispatch => {
        let active = ref(true);
        let cleanup = ref(() => ());
        B.after(() =>
          if (active^) {
            switch (
              B.find(Ui_attr.part_id(model.id, "viewport")),
              B.find(Ui_attr.part_id(model.id, "content")),
            ) {
            | (Some(container), Some(content)) =>
              let read_generation = () =>
                Option.value(
                  ~default=-1,
                  Option.bind(
                    Js.Nullable.toOption(
                      B.attr(container, "data-tea-virtual-generation"),
                    ),
                    int_of_string_opt,
                  ),
                );
              let generation = ref(read_generation());
              let queued = ref(false);
              let sizes = ref(Sizes.empty);
              let scrolled = ref(false);
              let top = ref(B.top(container));
              let height = ref(B.height(container));
              let queue = () =>
                if (! queued^) {
                  queued := true;
                  B.after(() =>
                    if (active^) {
                      queued := false;
                      let sample = {
                        top: top^,
                        height: height^,
                        sizes: Sizes.bindings(sizes^),
                        scrolled: scrolled^,
                        generation: generation^,
                      };
                      sizes := Sizes.empty;
                      scrolled := false;
                      dispatch(sample);
                    }
                  );
                };
              let decoder =
                D.map2(
                  (top_, height_) => (top_, height_),
                  D.at(["target", "scrollTop"], D.float),
                  D.at(["target", "clientHeight"], D.float),
                );
              let on_scroll = event =>
                switch (Tea_dom.Dom_decode.run(decoder, event)) {
                | Ok((top_, height_)) =>
                  top := top_;
                  height := height_;
                  generation := read_generation();
                  scrolled := true;
                  queue();
                | Error(_) => ()
                };
              B.listen(container, "scroll", on_scroll);
              let observed = ref([||]);
              let observer =
                Js.typeof(B.get(B.global, "ResizeObserver")) == "function"
                  ? Some(
                      B.resize(entries => {
                        top := B.top(container);
                        generation := read_generation();
                        Array.iter(
                          entry => {
                            let element = B.target(entry);
                            if (element === container) {
                              height := B.height(container);
                            } else {
                              switch (
                                Js.Nullable.toOption(
                                  B.attr(element, "data-tea-virtual-key"),
                                )
                              ) {
                              | Some(key) =>
                                sizes :=
                                  Sizes.add(
                                    key,
                                    B.get(B.rect(element), "height"),
                                    sizes^,
                                  )
                              | None => ()
                              };
                            };
                          },
                          entries,
                        );
                        queue();
                      }),
                    )
                  : None;
              let measure_rows =
                switch (model.row_size) {
                | Fixed(_) => false
                | Estimated(_) => true
                };
              let scan = () =>
                if (measure_rows) {
                  Option.iter(
                    observer => {
                      let current = B.children(content);
                      Array.iter(
                        el =>
                          if (!Array.exists(other => el === other, current)) {
                            B.unobserve(observer, el);
                          },
                        observed^,
                      );
                      Array.iter(
                        el =>
                          if (!Array.exists(other => el === other, observed^)) {
                            B.observe_size(
                              observer,
                              el,
                              { "box": "border-box" },
                            );
                          },
                        current,
                      );
                      observed :=
                        Array.init(Array.length(current), i => current[i]);
                    },
                    observer,
                  );
                };
              Option.iter(
                observer =>
                  B.observe_size(
                    observer,
                    container,
                    { "box": "border-box" },
                  ),
                observer,
              );
              scan();
              let mutations =
                if (measure_rows) {
                  let observer = B.mutations(scan);
                  B.observe_children(
                    observer,
                    content,
                    { "childList": true },
                  );
                  Some(observer);
                } else {
                  None;
                };
              queue();
              cleanup :=
                (
                  () => {
                    B.unlisten(container, "scroll", on_scroll);
                    Option.iter(B.disconnect, observer);
                    Option.iter(B.disconnect, mutations);
                    sizes := Sizes.empty;
                    observed := [||];
                  }
                );
            | _ => ()
            };
          }
        );
        () => {
          active := false;
          cleanup^();
        };
      },
  );
type semantics =
  | Set
  | Grid;
type row('parent) = {
  key: string,
  index: int,
  offset: float,
  attrs: list(Html.attr('parent)),
};
type parts('parent) = {
  container: list(Html.attr('parent)),
  content: list(Html.attr('parent)),
  rows: list(row('parent)),
  range: Geometry.range,
  total: float,
};
let view =
    (~model, ~items, ~pinned=[], ~semantics=Set, ~to_parent, ~to_view, ()) => {
  ignore(to_parent);
  let model = synchronize(~items, model);
  let range = visible_range(model);
  let indices = ref(List.filter(i => i >= 0 && i < count(items), pinned));
  for (index in range.first to range.last - 1) {
    indices := [index, ...indices^];
  };
  let indices = List.sort_uniq(Int.compare, indices^);
  let px = x => Js.Float.toString(x) ++ "px";
  to_view({
    container: [
      Html.id(Ui_attr.part_id(model.id, "viewport")),
      Html.data("tea-virtual-generation", string_of_int(model.generation)),
      Html.style("overflow", "auto"),
      Html.style("padding", "0"),
      Html.style("overflow-anchor", "none"),
      Ui_attr.data_flag("virtual-list", true),
      semantics == Grid
        ? Html.aria("rowcount", string_of_int(count(items))) : Html.none,
    ],
    content: [
      Html.id(Ui_attr.part_id(model.id, "content")),
      Html.style("position", "relative"),
      Html.style("padding", "0"),
      Html.style("margin", "0"),
      Html.style("height", px(total_size(model))),
      Html.style("overflow-anchor", "none"),
    ],
    rows:
      List.map(
        index => {
          let key = items.keys[index];
          let offset = row_offset(~index, model);
          {
            key,
            index,
            offset,
            attrs: [
              Html.id(row_id(~key, model)),
              semantics == Set
                ? Html.aria("setsize", string_of_int(count(items)))
                : Html.none,
              Html.aria(
                semantics == Grid ? "rowindex" : "posinset",
                string_of_int(index + 1),
              ),
              Html.data("tea-virtual-key", key),
              Ui_attr.data_flag("pinned", List.mem(index, pinned)),
              Html.style("position", "absolute"),
              Html.style("margin", "0"),
              Html.style("top", px(offset)),
              Html.style("left", "0"),
              Html.style("width", "100%"),
              Html.style("box-sizing", "border-box"),
              switch (model.row_size) {
              | Fixed(height) => Html.style("height", px(positive(height)))
              | Estimated(_) => Html.none
              },
            ],
          };
        },
        indices,
      ),
    range,
    total: total_size(model),
  });
};
