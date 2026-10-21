/** Private pure navigation over disabled flags. No DOM or clock. */
/** Configured writing direction. */
type direction =
  | Ltr
  | Rtl;
/** First enabled index, or None (including empty/all-disabled). */
let first: array(bool) => option(int);
/** Last enabled index, or None. */
let last: array(bool) => option(int);
/** Next enabled index. Wrap defaults to true. Without wrap, retains an enabled
    current index at the boundary; invalid indices start before the array. */
let next: (~wrap: bool=?, ~from: int, array(bool)) => option(int);
/** Previous enabled index; invalid indices start after the array. */
let prev: (~wrap: bool=?, ~from: int, array(bool)) => option(int);
/** Jump at least one index, clamped to the array. Search past a disabled target
    in the requested direction, then back toward the current index if needed.
    Oversized pages saturate; empty/all-disabled returns None. */
let page:
  (~size: int, ~forward: bool, ~from: int, array(bool)) => option(int);
/** Keep an enabled current index, otherwise return the first enabled index. */
let repair: (~from: int, array(bool)) => option(int);
/** Horizontal arrow intent, swapping left/right for RTL; other keys are None. */
let horizontal: (~dir: direction, string) => option(int);
