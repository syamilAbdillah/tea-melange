/** Milliseconds since the Unix epoch. */
type posix = float;
/** Read the clock when the command runs. */
let now: (posix => 'msg) => Tea_core.Cmd.t('msg);
/** Lazy clock task. */
let now_task: Tea_core.Task.t(posix, unit);
/** Subscribe to a positive finite interval in milliseconds. */
let every: (float, posix => 'msg) => Tea_core.Sub.t('msg);
