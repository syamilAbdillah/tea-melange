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
[@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
/** Whether [node] is inside the element with [id]; false when either is missing. */
let within = (id, node) =>
  switch (find(id), node) {
  | (Some(root), Some(node)) => contains(root, node)
  | _ => false
  };

[@mel.get] external target_: Dom.event => Js.Nullable.t(Dom.node) = "target";
let target = event => Js.Nullable.toOption(target_(event));
[@mel.get]
external related_: Dom.event => Js.Nullable.t(Dom.node) = "relatedTarget";
let related = event => Js.Nullable.toOption(related_(event));
[@mel.send] external prevent: Dom.event => unit = "preventDefault";
[@mel.send] external stop: Dom.event => unit = "stopPropagation";
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
