/** CPS tasks. Calling a task starts it; combinators are lazy and execute on
    the same scheduler as their inputs. Producers should complete exactly once. */
type t('ok, 'err) = (result('ok, 'err) => unit) => unit;
/** A task that immediately succeeds. */
let succeed: 'ok => t('ok, 'err);
/** A task that immediately fails. */
let fail: 'err => t('ok, 'err);
/** Transform a success without changing errors. */
let map: ('a => 'b, t('a, 'err)) => t('b, 'err);
/** Transform an error without changing successes. */
let map_error: ('a => 'b, t('ok, 'a)) => t('ok, 'b);
/** Start the next task after success; propagate errors. */
let and_then: ('a => t('b, 'err), t('a, 'err)) => t('b, 'err);
/** Recover from an error with another task. */
let on_error: ('a => t('ok, 'b), t('ok, 'a)) => t('ok, 'b);
/** Execute left-to-right, collecting successes; stop at the first error. */
let sequence: list(t('ok, 'err)) => t(list('ok), 'err);
/** Send successes as messages. Unit errors are deliberately ignored; use
    [attempt] when errors need to be represented in the program's model. */
let perform: ('ok => 'msg, t('ok, unit)) => Cmd.t('msg);
/** Send either outcome as a message. */
let attempt: (result('ok, 'err) => 'msg, t('ok, 'err)) => Cmd.t('msg);
