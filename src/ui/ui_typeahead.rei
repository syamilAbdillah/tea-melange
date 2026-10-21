/** Private pure typeahead. Event timeStamp only; no wall clock.
    Matching lowercases, applies Unicode NFD and drops combining marks
    (U+0300–U+036F), so precomposed and decomposed accents match ("É",
    "E\u0301" and "e" are equal). Letters without a canonical decomposition
    are folded explicitly: æ→ae, œ→oe, ø→o, ß→ss, þ→th, ð/đ→d, ł→l, ı→i.
    Other scripts are compared after lowercasing only; no locale-specific
    folding is applied. */
/** Buffer and previous event timestamp. */
type t;
/** Empty search state. */
let empty: t;
/** Current normalized buffer. */
let buffer: t => string;
/** Last accepted event timestamp, or None. */
let last_time_stamp: t => option(float);
/** Lowercase, NFD without combining marks, plus the documented folds. */
let normalize: string => string;
/** Feed one Unicode scalar key (one UTF-16 unit or a surrogate pair).
    Non-character keys and lone combining marks leave state unchanged.
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
