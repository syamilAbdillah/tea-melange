/** Element selectors. Text matches exact concatenated descendant text.
    Searches are depth-first pre-order, including the root; text leaves are not
    selectable. Returned nodes preserve all message taggers on their path. */
type selector =
  | Tag(string)
  | Id(string)
  | Class(string)
  | Text(string);
/** Concatenate text descendants, transparently forcing thunks and taggers. */
let text_content: Tea_vdom.Vdom.t('msg) => string;
/** All matching elements, in document order. */
let find_all:
  (Tea_vdom.Vdom.t('msg), ~selector: selector) =>
  list(Tea_vdom.Vdom.t('msg));
/** First matching element, or None. */
let find:
  (Tea_vdom.Vdom.t('msg), ~selector: selector) =>
  option(Tea_vdom.Vdom.t('msg));
