/** Decode the named handler on the first matching element.
    Applies all ancestor and local message taggers. Returns None for missing
    elements/handlers or failed decoders. This simulates a single listener,
    not DOM bubbling, browser defaults, or propagation effects; handler flags
    are decoded but discarded. Duplicate event facts use the last handler. */
let event:
  (
    Tea_vdom.Vdom.t('msg),
    ~selector: Query.selector,
    ~event_name: string,
    ~event: Tea_core.Decode.Value.t
  ) =>
  option('msg);
