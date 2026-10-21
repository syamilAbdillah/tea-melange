/** The SVG namespace URI. */
let namespace: string;
/** Construct a namespaced SVG node; empty-name no-op attributes are removed. */
let node:
  (string, list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [svg] element. */
let svg:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [g] element. */
let g:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [circle] element. */
let circle:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [rect] element. */
let rect:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [line] element. */
let line:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [path] element. */
let path:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [polyline] element. */
let polyline:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [polygon] element. */
let polygon:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [ellipse] element. */
let ellipse:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [text] element. */
let text_:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [tspan] element. */
let tspan:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [defs] element. */
let defs:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [use] element. */
let use:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [linearGradient] element. */
let linearGradient:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [radialGradient] element. */
let radialGradient:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [stop] element. */
let stop:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [clipPath] element. */
let clipPath:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [mask] element. */
let mask:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [pattern] element. */
let pattern:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [symbol] element. */
let symbol:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [marker] element. */
let marker:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [title] element. */
let title:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** Construct an SVG [desc] element. */
let desc:
  (list(Tea_vdom.Vdom.fact('msg)), list(Tea_vdom.Vdom.t('msg))) =>
  Tea_vdom.Vdom.t('msg);
/** SVG attributes always use Attribute or Attribute_ns, never Property.
    Numeric values are strings to support SVG units and percentages. */
module Attr: {
  /** Arbitrary SVG attribute. */
  let attribute: (string, string) => Tea_vdom.Vdom.fact('msg);
  /** Namespaced [xlink:href] with its qualified name. */
  let xlink_href: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [viewBox] attribute. */
  let viewBox: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [cx] attribute. */
  let cx: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [cy] attribute. */
  let cy: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [r] attribute. */
  let r: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [rx] attribute. */
  let rx: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [ry] attribute. */
  let ry: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [x] attribute. */
  let x: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [y] attribute. */
  let y: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [x1] attribute. */
  let x1: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [x2] attribute. */
  let x2: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [y1] attribute. */
  let y1: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [y2] attribute. */
  let y2: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [width] attribute. */
  let width: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [height] attribute. */
  let height: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [d] attribute. */
  let d: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [fill] attribute. */
  let fill: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [stroke] attribute. */
  let stroke: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [stroke-width] attribute. */
  let stroke_width: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [transform] attribute. */
  let transform: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [points] attribute. */
  let points: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [offset] attribute. */
  let offset: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [stop-color] attribute. */
  let stop_color: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [stop-opacity] attribute. */
  let stop_opacity: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [fill-opacity] attribute. */
  let fill_opacity: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [stroke-opacity] attribute. */
  let stroke_opacity: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [opacity] attribute. */
  let opacity: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [preserveAspectRatio] attribute. */
  let preserveAspectRatio: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [id] attribute. */
  let id: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [class] attribute. */
  let class_: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [href] attribute. */
  let href: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [xmlns] attribute. */
  let xmlns: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [font-size] attribute. */
  let font_size: string => Tea_vdom.Vdom.fact('msg);
  /** Set the SVG [text-anchor] attribute. */
  let text_anchor: string => Tea_vdom.Vdom.fact('msg);
};
