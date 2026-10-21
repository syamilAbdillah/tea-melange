module V = Tea_vdom.Vdom;
let namespace = "http://www.w3.org/2000/svg";
let node = (tag, attrs, kids) =>
  V.node(
    ~namespace,
    tag,
    List.filter(
      fun
      | V.Attribute("", _) => false
      | _ => true,
      attrs,
    ),
    kids,
  );
let svg = (attrs, kids) => node("svg", attrs, kids);
let g = (attrs, kids) => node("g", attrs, kids);
let circle = (attrs, kids) => node("circle", attrs, kids);
let rect = (attrs, kids) => node("rect", attrs, kids);
let line = (attrs, kids) => node("line", attrs, kids);
let path = (attrs, kids) => node("path", attrs, kids);
let polyline = (attrs, kids) => node("polyline", attrs, kids);
let polygon = (attrs, kids) => node("polygon", attrs, kids);
let ellipse = (attrs, kids) => node("ellipse", attrs, kids);
let text_ = (attrs, kids) => node("text", attrs, kids);
let tspan = (attrs, kids) => node("tspan", attrs, kids);
let defs = (attrs, kids) => node("defs", attrs, kids);
let use = (attrs, kids) => node("use", attrs, kids);
let linearGradient = (attrs, kids) => node("linearGradient", attrs, kids);
let radialGradient = (attrs, kids) => node("radialGradient", attrs, kids);
let stop = (attrs, kids) => node("stop", attrs, kids);
let clipPath = (attrs, kids) => node("clipPath", attrs, kids);
let mask = (attrs, kids) => node("mask", attrs, kids);
let pattern = (attrs, kids) => node("pattern", attrs, kids);
let symbol = (attrs, kids) => node("symbol", attrs, kids);
let marker = (attrs, kids) => node("marker", attrs, kids);
let title = (attrs, kids) => node("title", attrs, kids);
let desc = (attrs, kids) => node("desc", attrs, kids);
module Attr = {
  let attribute = V.attribute;
  let xlink_href = value =>
    V.Attribute_ns("http://www.w3.org/1999/xlink", "xlink:href", value);
  let viewBox = value => attribute("viewBox", value);
  let cx = value => attribute("cx", value);
  let cy = value => attribute("cy", value);
  let r = value => attribute("r", value);
  let rx = value => attribute("rx", value);
  let ry = value => attribute("ry", value);
  let x = value => attribute("x", value);
  let y = value => attribute("y", value);
  let x1 = value => attribute("x1", value);
  let x2 = value => attribute("x2", value);
  let y1 = value => attribute("y1", value);
  let y2 = value => attribute("y2", value);
  let width = value => attribute("width", value);
  let height = value => attribute("height", value);
  let d = value => attribute("d", value);
  let fill = value => attribute("fill", value);
  let stroke = value => attribute("stroke", value);
  let stroke_width = value => attribute("stroke-width", value);
  let transform = value => attribute("transform", value);
  let points = value => attribute("points", value);
  let offset = value => attribute("offset", value);
  let stop_color = value => attribute("stop-color", value);
  let stop_opacity = value => attribute("stop-opacity", value);
  let fill_opacity = value => attribute("fill-opacity", value);
  let stroke_opacity = value => attribute("stroke-opacity", value);
  let opacity = value => attribute("opacity", value);
  let preserveAspectRatio = value => attribute("preserveAspectRatio", value);
  let id = value => attribute("id", value);
  let class_ = value => attribute("class", value);
  let href = value => attribute("href", value);
  let xmlns = value => attribute("xmlns", value);
  let font_size = value => attribute("font-size", value);
  let text_anchor = value => attribute("text-anchor", value);
};
