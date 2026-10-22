/* The DOM bindings tea.ui components use outside tea.browser's public API.
   Nullable results are returned as options. */

/* Globals, clocks and frames */
type timer;
external global: Js.Json.t = "globalThis";
external document: Dom.document = "document";
external window: Dom.window = "window";
external timeout: (unit => unit, int) => timer = "setTimeout";
external clear_timeout: timer => unit = "clearTimeout";
external raf: (float => unit) => int = "requestAnimationFrame";
external cancel_raf: int => unit = "cancelAnimationFrame";
[@mel.scope "performance"] external now: unit => float = "now";
[@mel.get] external hidden: Dom.document => bool = "hidden";
[@mel.get] external pixel_ratio: Dom.window => float = "devicePixelRatio";

/* Untyped access */
[@mel.get_index] external get: ('a, string) => Js.Json.t;
[@mel.get] external length: Js.Json.t => int = "length";
external to_raw: 'a => Js.Json.t = "%identity";
external to_node: Js.Json.t => Dom.node = "%identity";
let has_method = (value, name) => Js.typeof(get(value, name)) == "function";
/** Run [callback] on the next animation frame, or after 16ms without
    requestAnimationFrame. The result cancels it. */
let next_frame = callback =>
  if (has_method(global, "requestAnimationFrame")) {
    let id = raf(_ => callback());
    () => cancel_raf(id);
  } else {
    let timer = timeout(callback, 16);
    () => clear_timeout(timer);
  };

/* Elements */
[@mel.send]
external by_id: (Dom.document, string) => Js.Nullable.t(Dom.node) =
  "getElementById";
let find = id => Js.Nullable.toOption(by_id(document, id));
[@mel.get] external body: Dom.document => Dom.node = "body";
[@mel.get] external id: Dom.node => string = "id";
[@mel.get] external connected: Dom.node => bool = "isConnected";
[@mel.get]
external parent_: Dom.node => Js.Nullable.t(Dom.node) = "parentElement";
let parent = node => Js.Nullable.toOption(parent_(node));
[@mel.get] external children: Dom.node => array(Dom.node) = "children";
[@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
[@mel.send] external matches: (Dom.node, string) => bool = "matches";
[@mel.send]
external closest_: (Dom.node, string) => Js.Nullable.t(Dom.node) = "closest";
let closest = (node, selector) =>
  Js.Nullable.toOption(closest_(node, selector));
[@mel.send]
external query_all: ('root, string) => array(Dom.node) = "querySelectorAll";
[@mel.send]
external attribute_: (Dom.node, string) => Js.Nullable.t(string) =
  "getAttribute";
let attribute = (node, name) =>
  Js.Nullable.toOption(attribute_(node, name));
/** Whether [node] is inside the element with [id]; false when either is missing. */
let within = (id, node) =>
  switch (find(id), node) {
  | (Some(root), Some(node)) => contains(root, node)
  | _ => false
  };
let within_any = (ids, node) => List.exists(id => within(id, node), ids);

/* Focus */
[@mel.get]
external active_: Dom.document => Js.Nullable.t(Dom.node) = "activeElement";
let active = () => Js.Nullable.toOption(active_(document));
let is_active = node =>
  switch (active()) {
  | Some(active) => active === node
  | None => false
  };
[@mel.send]
external focus_: (Dom.node, {. "preventScroll": bool }) => unit = "focus";
let focus = (~prevent_scroll=false, node) =>
  focus_(node, { "preventScroll": prevent_scroll });
/** Focus the element with [id] now, without scrolling; missing ids no-op. */
let focus_id = id => Option.iter(focus(~prevent_scroll=true), find(id));

/* Form controls */
[@mel.get] external value: Dom.node => string = "value";
[@mel.set] external set_value: (Dom.node, string) => unit = "value";
[@mel.send] external click: Dom.node => unit = "click";
[@mel.send]
external set_selection_range: (Dom.node, int, int) => unit =
  "setSelectionRange";

/* Popover API */
[@mel.send] external show_popover: Dom.node => unit = "showPopover";
[@mel.send] external hide_popover: Dom.node => unit = "hidePopover";

/* Pointer capture */
[@mel.send]
external set_pointer_capture: (Dom.node, int) => unit = "setPointerCapture";
[@mel.send]
external has_pointer_capture: (Dom.node, int) => bool = "hasPointerCapture";
[@mel.send]
external release_pointer_capture: (Dom.node, int) => unit =
  "releasePointerCapture";

/* Events */
[@mel.get] external target_: Dom.event => Js.Nullable.t(Dom.node) = "target";
let target = event => Js.Nullable.toOption(target_(event));
/** Whether the event targets [value], which need not be a node (window). */
let is_target = (event, value) => to_raw(target_(event)) === to_raw(value);
[@mel.get]
external related_: Dom.event => Js.Nullable.t(Dom.node) = "relatedTarget";
let related = event => Js.Nullable.toOption(related_(event));
[@mel.send] external prevent: Dom.event => unit = "preventDefault";
[@mel.get] external prevented: Dom.event => bool = "defaultPrevented";
[@mel.send] external stop: Dom.event => unit = "stopPropagation";
[@mel.get] external detail: Dom.event => int = "detail";
[@mel.get] external pointer_type: Dom.event => string = "pointerType";
[@mel.get] external pointer_id: Dom.event => int = "pointerId";
[@mel.get]
external optional_pointer_id: Dom.event => Js.Undefined.t(int) = "pointerId";
[@mel.get] external client_x: Dom.event => float = "clientX";
[@mel.get] external client_y: Dom.event => float = "clientY";
external raw: Dom.event => Tea_dom.Dom_decode.raw = "%identity";
[@mel.send]
external listen: ('target, string, Dom.event => unit, bool) => unit =
  "addEventListener";
[@mel.send]
external listen_with:
  (
    'target,
    string,
    Dom.event => unit,
    {
      .
      "capture": bool,
      "passive": bool,
    }
  ) =>
  unit =
  "addEventListener";
[@mel.send]
external unlisten: ('target, string, Dom.event => unit, bool) => unit =
  "removeEventListener";
/** Add a listener; the result removes it. [passive] defaults to the
    browser's choice. */
let on = (~capture=false, ~passive=?, target, name, handler) => {
  switch (passive) {
  | None => listen(target, name, handler, capture)
  | Some(passive) =>
    listen_with(
      target,
      name,
      handler,
      {
        "capture": capture,
        "passive": passive,
      },
    )
  };
  () => unlisten(target, name, handler, capture);
};

/* Observers */
type observer;
type resize_entry;
[@mel.new]
external resize_observer: (array(resize_entry) => unit) => observer =
  "ResizeObserver";
[@mel.get] external entry_target: resize_entry => Dom.node = "target";
[@mel.send]
external observe_border_box: (observer, Dom.node, {. "box": string }) => unit =
  "observe";
let observe_size = (observer, node) =>
  observe_border_box(observer, node, { "box": "border-box" });
[@mel.send] external unobserve: (observer, Dom.node) => unit = "unobserve";
[@mel.new]
external mutation_observer: (unit => unit) => observer = "MutationObserver";
[@mel.send]
external observe_mutations:
  (observer, Dom.node, {. "childList": bool }) => unit =
  "observe";
let observe_children = (observer, node) =>
  observe_mutations(observer, node, { "childList": true });
[@mel.send] external disconnect: observer => unit = "disconnect";

/* Geometry and scrolling */
type rect = {
  left: float,
  right: float,
  top: float,
  bottom: float,
  width: float,
  height: float,
};
[@mel.send] external rect: Dom.node => rect = "getBoundingClientRect";
[@mel.send]
external element_from_point_:
  (Dom.document, float, float) => Js.Nullable.t(Dom.node) =
  "elementFromPoint";
let element_from_point = (x, y) =>
  has_method(document, "elementFromPoint")
    ? Js.Nullable.toOption(element_from_point_(document, x, y)) : None;
[@mel.send]
external computed_style: (Dom.window, Dom.node) => Dom.cssStyleDeclaration =
  "getComputedStyle";
[@mel.get]
external overflow_x: Dom.cssStyleDeclaration => string = "overflowX";
[@mel.get]
external overflow_y: Dom.cssStyleDeclaration => string = "overflowY";
[@mel.get] external scroll_width: Dom.node => float = "scrollWidth";
[@mel.get] external scroll_height: Dom.node => float = "scrollHeight";
[@mel.get] external client_width: Dom.node => float = "clientWidth";
[@mel.get] external client_height: Dom.node => float = "clientHeight";
[@mel.get] external scroll_left: Dom.node => float = "scrollLeft";
[@mel.set] external set_scroll_left: (Dom.node, float) => unit = "scrollLeft";
[@mel.get] external scroll_top: Dom.node => float = "scrollTop";
[@mel.set] external set_scroll_top: (Dom.node, float) => unit = "scrollTop";
[@mel.get] external inner_width: Dom.window => float = "innerWidth";
[@mel.get] external inner_height: Dom.window => float = "innerHeight";
[@mel.get] external scroll_x: Dom.window => float = "scrollX";
[@mel.get] external scroll_y: Dom.window => float = "scrollY";
[@mel.send]
external scroll_by: (Dom.window, float, float) => unit = "scrollBy";
[@mel.get]
external scrolling_element_: Dom.document => Js.Nullable.t(Dom.node) =
  "scrollingElement";
[@mel.get]
external document_element: Dom.document => Dom.node = "documentElement";
let scrolling_element = () =>
  switch (Js.Nullable.toOption(scrolling_element_(document))) {
  | Some(root) => root
  | None => document_element(document)
  };
