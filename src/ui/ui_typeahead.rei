/** Private pure typeahead. Event timeStamp only; no wall clock.
    Matching folds ASCII case and Latin-1 accented letters/ligatures.
    It does not perform full Unicode case folding or normalization; combining
    sequences and scripts outside Latin-1 are compared as-is. */
/** Buffer and previous event timestamp. */
type t;
/** Empty search state. */
let empty: t;
/** Current normalized buffer. */
let buffer: t => string;
/** Last accepted event timestamp, or None. */
let last_time_stamp: t => option(float);
/** ASCII lowercase plus the documented Latin-1 fold. */
let normalize: string => string;
/** Feed one Unicode scalar key. Non-character keys leave state unchanged.
    A gap >= timeout (default 500ms), or a backwards timestamp, resets.
    Repeated same characters cycle from the next item; growing prefixes retain
    the current match. Missing disabled flags count as disabled. Searches wrap,
    skip disabled items and return None for no match/empty/all-disabled.
    A failed longer prefix retries this key as a fresh single-character search. */
let feed:
  (
    ~timeout: float=?,
    ~time_stamp: float,
    ~key: string,
    ~from: int,
    ~labels: array(string),
    ~disabled: array(bool),
    t
  ) =>
  (t, option(int));
