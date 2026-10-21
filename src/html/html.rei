/** Pure HTML nodes with message-typed facts. */
/** All HTML attribute helpers, re-exported for convenience. */
/** Typed HTML facts. [value] is a string Property. [checked], [selected],
    [disabled], [hidden], [readonly], [required], [autofocus], and [multiple]
    are boolean Properties (DOM spellings [readOnly] and [autoFocus]).
    All other named helpers produce Attributes, except [style], which is Style.
    Boolean false must clear the DOM property, not emit an attribute "false". */
/** An untyped attribute escape hatch. */
let attribute: (string, string) => Tea_vdom.Vdom.fact('msg);
/** A typed DOM-property escape hatch. */
let property: (string, Tea_vdom.Vdom.prop_value) => Tea_vdom.Vdom.fact('msg);
/** No-op sentinel [Attribute("", "")]. Html.node/keyed and Svg.node filter it.
    Direct Vdom consumers must filter empty-name Attributes before DOM calls. */
let none: Tea_vdom.Vdom.fact('msg);
/** Set the CSS class attribute. */
let class_: string => Tea_vdom.Vdom.fact('msg);
/** Join enabled classes in declaration order. */
let classList: list((string, bool)) => Tea_vdom.Vdom.fact('msg);
/** One CSS declaration. */
let style: (string, string) => Tea_vdom.Vdom.fact('msg);
/** Prefix an attribute name with [data-]. */
let data: (string, string) => Tea_vdom.Vdom.fact('msg);
/** Prefix an attribute name with [aria-]. */
let aria: (string, string) => Tea_vdom.Vdom.fact('msg);
/** Set the [id] attribute. */
let id: string => Tea_vdom.Vdom.fact('msg);
/** Set the [href] attribute. */
let href: string => Tea_vdom.Vdom.fact('msg);
/** Set the [src] attribute. */
let src: string => Tea_vdom.Vdom.fact('msg);
/** Set the [alt] attribute. */
let alt: string => Tea_vdom.Vdom.fact('msg);
/** Set the [type] attribute. */
let type_: string => Tea_vdom.Vdom.fact('msg);
/** Set the [placeholder] attribute. */
let placeholder: string => Tea_vdom.Vdom.fact('msg);
/** Set the [name] attribute. */
let name: string => Tea_vdom.Vdom.fact('msg);
/** Set the [for] attribute. */
let for_: string => Tea_vdom.Vdom.fact('msg);
/** Set the [title] attribute. */
let title: string => Tea_vdom.Vdom.fact('msg);
/** Set the [role] attribute. */
let role: string => Tea_vdom.Vdom.fact('msg);
/** Set the integer [width] attribute. */
let width: int => Tea_vdom.Vdom.fact('msg);
/** Set the integer [height] attribute. */
let height: int => Tea_vdom.Vdom.fact('msg);
/** Set the integer [tabindex] attribute. */
let tabindex: int => Tea_vdom.Vdom.fact('msg);
/** Set the live input value property. */
let value: string => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [checked] property. */
let checked: bool => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [selected] property. */
let selected: bool => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [disabled] property. */
let disabled: bool => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [hidden] property. */
let hidden: bool => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [readOnly] property. */
let readonly: bool => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [required] property. */
let required: bool => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [autoFocus] property. */
let autofocus: bool => Tea_vdom.Vdom.fact('msg);
/** Set the boolean [multiple] property. */
let multiple: bool => Tea_vdom.Vdom.fact('msg);

/** All event helpers, re-exported for convenience. */
/** Construct an event fact from a handler description. */
let on: (string, Tea_vdom.Vdom.handler('msg)) => Tea_vdom.Vdom.fact('msg);
/** Decode a message and whether to prevent the browser default. */
let prevent_default_on:
  (string, Tea_core.Decode.t(('msg, bool))) => Tea_vdom.Vdom.fact('msg);
/** Decode a message and whether to stop propagation. */
let stop_propagation_on:
  (string, Tea_core.Decode.t(('msg, bool))) => Tea_vdom.Vdom.fact('msg);
/** Decode a message plus both event flags. */
let custom:
  (string, Tea_core.Decode.t(Tea_vdom.Vdom.custom('msg))) =>
  Tea_vdom.Vdom.fact('msg);
/** Decode [target.value] on input. */
let on_input: (string => 'msg) => Tea_vdom.Vdom.fact('msg);
/** Decode [target.value] on change. */
let on_change: (string => 'msg) => Tea_vdom.Vdom.fact('msg);
/** Decode [target.checked] on change. */
let on_check: (bool => 'msg) => Tea_vdom.Vdom.fact('msg);
/** Submit a message and always prevent default. */
let on_submit: 'msg => Tea_vdom.Vdom.fact('msg);
/** Decode the event [key]; [None] rejects the event without a message. */
let on_key_down: (string => option('msg)) => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [click]. */
let on_click: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [dblclick]. */
let on_double_click: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [blur]. */
let on_blur: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [focus]. */
let on_focus: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mousedown]. */
let on_mouse_down: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseup]. */
let on_mouse_up: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseenter]. */
let on_mouse_enter: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseleave]. */
let on_mouse_leave: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mousemove]. */
let on_mouse_move: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseover]. */
let on_mouse_over: 'msg => Tea_vdom.Vdom.fact('msg);
/** Emit a message on [mouseout]. */
let on_mouse_out: 'msg => Tea_vdom.Vdom.fact('msg);

/** The underlying Vdom node type. */
type t('msg) = Tea_vdom.Vdom.t('msg);
/** The underlying Vdom fact type. */
type attr('msg) = Tea_vdom.Vdom.fact('msg);
/** A text node (escaped by the renderer). */
let text: string => t('msg);
/** Construct an HTML element, dropping empty-name attribute sentinels. */
let node: (string, list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a keyed HTML element, dropping no-op attributes. */
let keyed:
  (string, list(attr('msg)), list((string, t('msg)))) => t('msg);
/** Transform messages throughout a subtree. */
let map: ('a => 'msg, t('a)) => t('msg);
/** Memoize a one-argument view using Vdom physical identity. */
let lazy1: ('a => t('msg), 'a) => t('msg);
/** Memoize a two-argument view. */
let lazy2: (('a, 'b) => t('msg), 'a, 'b) => t('msg);
/** Memoize a three-argument view. */
let lazy3: (('a, 'b, 'c) => t('msg), 'a, 'b, 'c) => t('msg);
/** Construct a [div] element. */
let div: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [span] element. */
let span: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [p] element. */
let p: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [a] element. */
let a: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [button] element. */
let button: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [input] element. */
let input: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [textarea] element. */
let textarea: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [select] element. */
let select: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [option] element. */
let option: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [label] element. */
let label: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [form] element. */
let form: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [ul] element. */
let ul: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [ol] element. */
let ol: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [li] element. */
let li: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [h1] element. */
let h1: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [h2] element. */
let h2: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [h3] element. */
let h3: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [h4] element. */
let h4: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [h5] element. */
let h5: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [h6] element. */
let h6: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [header] element. */
let header: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [footer] element. */
let footer: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [main] element. */
let main: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [nav] element. */
let nav: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [section] element. */
let section: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [article] element. */
let article: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [aside] element. */
let aside: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [table] element. */
let table: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [thead] element. */
let thead: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [tbody] element. */
let tbody: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [tfoot] element. */
let tfoot: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [tr] element. */
let tr: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [th] element. */
let th: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [td] element. */
let td: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [caption] element. */
let caption: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [colgroup] element. */
let colgroup: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [col] element. */
let col: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [img] element. */
let img: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [br] element. */
let br: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [hr] element. */
let hr: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [pre] element. */
let pre: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [code] element. */
let code: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [strong] element. */
let strong: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [em] element. */
let em: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [small] element. */
let small: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [fieldset] element. */
let fieldset: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [legend] element. */
let legend: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [details] element. */
let details: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [summary] element. */
let summary: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [html] element. */
let html: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [head] element. */
let head: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [body] element. */
let body: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [title] element. */
let title_: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [meta] element. */
let meta: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [link] element. */
let link: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [style] element. */
let style_: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [script] element. */
let script: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [noscript] element. */
let noscript: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [address] element. */
let address: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [blockquote] element. */
let blockquote: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [dl] element. */
let dl: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [dt] element. */
let dt: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [dd] element. */
let dd: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [figure] element. */
let figure: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [figcaption] element. */
let figcaption: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [video] element. */
let video: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [audio] element. */
let audio: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [source] element. */
let source: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [track] element. */
let track: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [canvas] element. */
let canvas: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [picture] element. */
let picture: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [iframe] element. */
let iframe: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [progress] element. */
let progress: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [meter] element. */
let meter: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [output] element. */
let output: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [datalist] element. */
let datalist: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [optgroup] element. */
let optgroup: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [abbr] element. */
let abbr: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [b] element. */
let b: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [i] element. */
let i: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [u] element. */
let u: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [s] element. */
let s: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [sub] element. */
let sub: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [sup] element. */
let sup: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [mark] element. */
let mark: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [time] element. */
let time: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [wbr] element. */
let wbr: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [area] element. */
let area: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [base] element. */
let base: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [embed] element. */
let embed: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [param] element. */
let param: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [dialog] element. */
let dialog: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [menu] element. */
let menu: (list(attr('msg)), list(t('msg))) => t('msg);
/** Construct a [search] element. */
let search: (list(attr('msg)), list(t('msg))) => t('msg);
/** Common keyed list containers. */
module Keyed: {
  /** Construct a keyed [ul] container. */
  let ul: (list(attr('msg)), list((string, t('msg)))) => t('msg);
  /** Construct a keyed [ol] container. */
  let ol: (list(attr('msg)), list((string, t('msg)))) => t('msg);
  /** Construct a keyed [div] container. */
  let div: (list(attr('msg)), list((string, t('msg)))) => t('msg);
};
