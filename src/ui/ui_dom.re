type timer;
external document: Dom.document = "document";
external window: Dom.window = "window";
external timeout: (unit => unit, int) => timer = "setTimeout";
external clear_timeout: timer => unit = "clearTimeout";
[@mel.scope "performance"] external now: unit => float = "now";
[@mel.get] external hidden: Dom.document => bool = "hidden";

[@mel.send]
external by_id: (Dom.document, string) => Js.Nullable.t(Dom.node) =
  "getElementById";
let find = id => Js.Nullable.toOption(by_id(document, id));
[@mel.get]
external active_: Dom.document => Js.Nullable.t(Dom.node) = "activeElement";
let active = () => Js.Nullable.toOption(active_(document));
[@mel.get_index] external get: ('a, string) => Js.Json.t;
let has_method = (value, name) => Js.typeof(get(value, name)) == "function";
[@mel.get] external body: Dom.document => Dom.node = "body";
[@mel.get] external connected: Dom.node => bool = "isConnected";
[@mel.get] external pixel_ratio: Dom.window => float = "devicePixelRatio";
[@mel.send] external show_popover: Dom.node => unit = "showPopover";
[@mel.send] external hide_popover: Dom.node => unit = "hidePopover";
[@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
[@mel.send] external matches: (Dom.node, string) => bool = "matches";
[@mel.send]
external focus_: (Dom.node, {. "preventScroll": bool }) => unit = "focus";
let focus = (~prevent_scroll=false, node) =>
  focus_(node, { "preventScroll": prevent_scroll });
let is_active = node =>
  switch (active()) {
  | Some(active) => active === node
  | None => false
  };
type rect = {
  left: float,
  right: float,
  top: float,
  bottom: float,
  width: float,
  height: float,
};
[@mel.send] external rect: Dom.node => rect = "getBoundingClientRect";
/** Whether [node] is inside the element with [id]; false when either is missing. */
let within = (id, node) =>
  switch (find(id), node) {
  | (Some(root), Some(node)) => contains(root, node)
  | _ => false
  };
let within_any = (ids, node) => List.exists(id => within(id, node), ids);

[@mel.get] external target_: Dom.event => Js.Nullable.t(Dom.node) = "target";
let target = event => Js.Nullable.toOption(target_(event));
[@mel.get]
external related_: Dom.event => Js.Nullable.t(Dom.node) = "relatedTarget";
let related = event => Js.Nullable.toOption(related_(event));
[@mel.send] external prevent: Dom.event => unit = "preventDefault";
[@mel.get] external prevented: Dom.event => bool = "defaultPrevented";
[@mel.send] external stop: Dom.event => unit = "stopPropagation";
[@mel.get] external pointer_type: Dom.event => string = "pointerType";
external raw: Dom.event => Tea_dom.Dom_decode.raw = "%identity";

[@mel.send]
external listen: ('target, string, Dom.event => unit, bool) => unit =
  "addEventListener";
[@mel.send]
external unlisten: ('target, string, Dom.event => unit, bool) => unit =
  "removeEventListener";
/** Add a listener; the result removes it. */
let on = (~capture=false, target, name, handler) => {
  listen(target, name, handler, capture);
  () => unlisten(target, name, handler, capture);
};
