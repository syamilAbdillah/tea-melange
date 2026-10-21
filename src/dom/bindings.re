/* Small typed FFI surface. No webapi dependency. */
type raw = Js.Json.t;
[@mel.get_index] external get: ('a, string) => raw;
[@mel.set_index] external put: ('a, string, 'b) => unit;
external cast: 'a => 'b = "%identity";
[@mel.scope "Reflect"] external has: ('a, string) => bool = "has";

external document: Dom.document = "document";
[@mel.send]
external text: (Dom.document, string) => Dom.node = "createTextNode";
[@mel.send]
external element: (Dom.document, string) => Dom.node = "createElement";
[@mel.send]
external element_ns: (Dom.document, string, string) => Dom.node =
  "createElementNS";
[@mel.send] external append: (Dom.node, Dom.node) => unit = "appendChild";
[@mel.send] external remove: (Dom.node, Dom.node) => unit = "removeChild";
[@mel.send]
external replace: (Dom.node, Dom.node, Dom.node) => unit = "replaceChild";
[@mel.send]
external insert: (Dom.node, Dom.node, Js.Nullable.t(Dom.node)) => unit =
  "insertBefore";
[@mel.get]
external parent: Dom.node => Js.Nullable.t(Dom.node) = "parentNode";
[@mel.get] external children: Dom.node => array(Dom.node) = "childNodes";
[@mel.set] external set_text: (Dom.node, string) => unit = "textContent";
[@mel.send]
external attribute: (Dom.node, string, string) => unit = "setAttribute";
[@mel.send]
external remove_attribute: (Dom.node, string) => unit = "removeAttribute";
[@mel.send]
external attribute_ns: (Dom.node, string, string, string) => unit =
  "setAttributeNS";
[@mel.send]
external remove_attribute_ns: (Dom.node, string, string) => unit =
  "removeAttributeNS";
[@mel.get] external style: Dom.node => Dom.cssStyleDeclaration = "style";
[@mel.send]
external style_set: (Dom.cssStyleDeclaration, string, string) => unit =
  "setProperty";
[@mel.send]
external style_remove: (Dom.cssStyleDeclaration, string) => unit =
  "removeProperty";
[@mel.send]
external listen: (Dom.node, string, raw => unit) => unit = "addEventListener";
[@mel.send]
external unlisten: (Dom.node, string, raw => unit) => unit =
  "removeEventListener";
[@mel.send] external stop: raw => unit = "stopPropagation";
[@mel.send] external prevent: raw => unit = "preventDefault";
[@mel.send]
external query: (Dom.document, string) => Js.Nullable.t(Dom.node) =
  "querySelector";
[@mel.get] external body: Dom.document => Dom.node = "body";
[@mel.set] external title: (Dom.document, string) => unit = "title";
