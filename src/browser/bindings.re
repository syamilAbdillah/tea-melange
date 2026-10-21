/* Typed platform boundary. Feature detection precedes optional browser APIs. */
type raw = Js.Json.t;
external global: Js.Dict.t(raw) = "globalThis";
external document: Dom.document = "document";
external window: Dom.window = "window";
[@mel.get_index] external get: ('a, string) => raw;
external cast: 'a => 'b = "%identity";
let method_ = (value, name) => Js.typeof(get(value, name)) == "function";
let available = name =>
  switch (Js.Dict.get(global, name)) {
  | Some(value) => Js.typeof(value) == "function"
  | None => false
  };
[@mel.send]
external by_id: (Dom.document, string) => Js.Nullable.t(Dom.node) =
  "getElementById";
let find = id => Js.Nullable.toOption(by_id(document, id));
[@mel.get]
external active: Dom.document => Js.Nullable.t(Dom.node) = "activeElement";
[@mel.get] external body: Dom.document => Dom.node = "body";
[@mel.get] external root: Dom.document => Dom.node = "documentElement";
[@mel.get] external id: Dom.node => string = "id";
[@mel.get] external connected: Dom.node => bool = "isConnected";
[@mel.get]
external parent: Dom.node => Js.Nullable.t(Dom.node) = "parentElement";
[@mel.get] external local_name: Dom.node => string = "localName";
[@mel.get] external tab_index: Dom.node => int = "tabIndex";
[@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
[@mel.send] external matches: (Dom.node, string) => bool = "matches";
[@mel.send] external has_attr: (Dom.node, string) => bool = "hasAttribute";
[@mel.send]
external attr: (Dom.node, string) => Js.Nullable.t(string) = "getAttribute";
[@mel.send]
external set_attr: (Dom.node, string, string) => unit = "setAttribute";
[@mel.send]
external remove_attr: (Dom.node, string) => unit = "removeAttribute";
[@mel.send]
external query_all: (Dom.node, string) => array(Dom.node) =
  "querySelectorAll";
[@mel.send]
external focus: (Dom.node, {. "preventScroll": bool }) => unit = "focus";
[@mel.send] external blur: Dom.node => unit = "blur";
[@mel.send]
external scroll_into_view: (Dom.node, {. "block": string }) => unit =
  "scrollIntoView";
[@mel.send] external rect: Dom.node => raw = "getBoundingClientRect";
[@mel.send]
external computed_style: (Dom.window, Dom.node) => Dom.cssStyleDeclaration =
  "getComputedStyle";
[@mel.get] external style: Dom.node => Dom.cssStyleDeclaration = "style";
[@mel.send]
external style_get: (Dom.cssStyleDeclaration, string) => string =
  "getPropertyValue";
[@mel.send]
external style_set: (Dom.cssStyleDeclaration, string, string) => unit =
  "setProperty";
[@mel.get] external inner_width: Dom.window => float = "innerWidth";
[@mel.get] external inner_height: Dom.window => float = "innerHeight";
[@mel.get] external scroll_x: Dom.window => float = "scrollX";
[@mel.get] external scroll_y: Dom.window => float = "scrollY";
[@mel.get] external client_width: Dom.node => float = "clientWidth";
[@mel.send]
external scroll_to: (Dom.window, float, float) => unit = "scrollTo";
[@mel.send] external show_modal: Dom.node => unit = "showModal";
[@mel.send] external show: Dom.node => unit = "show";
[@mel.send] external close: Dom.node => unit = "close";
[@mel.send]
external animations: (Dom.node, {. "subtree": bool }) => array(raw) =
  "getAnimations";
[@mel.get] external finished: raw => Js.Promise.t(unit) = "finished";
[@mel.scope "Promise"]
external all_settled: array(Js.Promise.t(unit)) => Js.Promise.t(array(raw)) =
  "allSettled";
external timeout: (unit => unit, int) => int = "setTimeout";
external clear_timeout: int => unit = "clearTimeout";
type observer;
[@mel.new]
external mutation_observer: (unit => unit) => observer = "MutationObserver";
[@mel.send]
external observe_mutations:
  (
    observer,
    Dom.node,
    {
      .
      "childList": bool,
      "subtree": bool,
    }
  ) =>
  unit =
  "observe";
[@mel.new]
external resize_observer: (array(raw) => unit) => observer = "ResizeObserver";
[@mel.send] external observe_size: (observer, Dom.node) => unit = "observe";
[@mel.send] external disconnect: observer => unit = "disconnect";
[@mel.send]
external listen: ('a, string, raw => unit, bool) => unit = "addEventListener";
[@mel.send]
external unlisten: ('a, string, raw => unit, bool) => unit =
  "removeEventListener";
type media;
[@mel.send] external match_media: (Dom.window, string) => media = "matchMedia";
[@mel.get] external media_matches: media => bool = "matches";
[@mel.send]
external media_listen: (media, raw => unit) => unit = "addListener";
[@mel.send]
external media_unlisten: (media, raw => unit) => unit = "removeListener";
let after_render = f =>
  Tea_dom.Browser_platform.request_animation_frame(_ => f());
let number = (value, name): float => cast(get(value, name));
let bool = (value, name): bool => cast(get(value, name));
let string = (value, name): string => cast(get(value, name));
let restore_style = (node, previous) =>
  switch (previous) {
  | Some(value) => set_attr(node, "style", value)
  | None =>
    remove_attr(node, "style");
    /* Chromium may lazily materialize an empty attribute from a previously
       read CSSStyleDeclaration. Flush that serialization before final removal. */
    switch (Js.Nullable.toOption(attr(node, "style"))) {
    | None => ()
    | Some(_) => remove_attr(node, "style")
    };
  };
let is_ios = () =>
  switch (Js.Dict.get(global, "navigator")) {
  | None => false
  | Some(nav) =>
    let platform = string(nav, "platform");
    String.starts_with(~prefix="iP", platform)
    || String.equal(platform, "MacIntel")
    && number(nav, "maxTouchPoints") > 1.;
  };
