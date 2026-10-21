/** Build a real DOM subtree. Thunks are forced; taggers create mutable lexical
    message scopes without adding DOM wrapper nodes. */
let create: ('msg => unit, Tea_vdom.Vdom.t('msg)) => Dom.node;
/** Detach all managed listeners and release controlled-property registrations.
    Call before externally discarding a root. Idempotent. */
let dispose: Dom.node => unit;
