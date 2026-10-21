/** Number of currently attached framework event listeners, across roots. */
let active_listeners: unit => int;
/** Cumulative Patch.apply calls (including empty plans), for batching tests. */
let patch_count: unit => int;
