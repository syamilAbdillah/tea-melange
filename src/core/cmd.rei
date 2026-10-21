/** Commands: effects described as data. [update] returns them; the runtime
    runs them after the model is committed and feeds results back as messages.

    [dispatch] given to an effect may be called synchronously or later; either
    way the message goes through the single runtime queue, never re-entrantly
    into [update]. */

type t('msg) =
  | None
  | Batch(list(t('msg)))
  | Effect(('msg => unit) => unit);

/** No effects. */
let none: t('msg);
/** Run commands depth-first, left-to-right. */
let batch: list(t('msg)) => t('msg);
/** Transform every emitted message. */
let map: ('a => 'b, t('a)) => t('b);
/** Low-level constructor used by effect modules (Http, Time, Random, ...). */
let effect: (('msg => unit) => unit) => t('msg);
/** Emit a message as a command (delivered via the queue, after the current update). */
let msg: 'msg => t('msg);
