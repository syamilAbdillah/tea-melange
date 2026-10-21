/** Alias of the typed HTML attribute helpers. */
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
