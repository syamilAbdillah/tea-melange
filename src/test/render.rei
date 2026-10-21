/** Deterministic snapshot HTML; not an SSR/hydration protocol.
    Escapes text and values, omits events, renders HTML void tags without end
    tags, and makes keyed nodes, taggers and memoized thunks transparent.
    Facts retain declaration order; duplicate attributes/properties use the last
    value in the first position. Boolean properties are present or absent;
    readOnly/autoFocus/className/htmlFor/tabIndex use HTML spellings.
    Styles form one attribute, preserving declaration order, with semicolons.
    SVG stays namespaced in Vdom; serialization preserves qualified xlink names
    but does not synthesize xmlns declarations. */
let to_string: Tea_vdom.Vdom.t('msg) => string;
