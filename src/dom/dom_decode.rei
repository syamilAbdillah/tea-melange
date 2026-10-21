/** Raw JavaScript values, including browser events with prototype getters. */
type raw = Js.Json.t;
/** Interpret a decoder directly over a raw value. Field access includes
    inherited properties; arrays and indexed array-like objects are supported.
    Decoder failure never dispatches a message. Value decodes JSON-shaped data
    only (not cyclic browser objects). */
let run: (Tea_core.Decode.t('a), raw) => result('a, string);
