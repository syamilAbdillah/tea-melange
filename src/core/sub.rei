/** Subscriptions: long-lived event sources described as data.

    Each leaf has a [home] (the manager, e.g. ["Time.every"]) and a [key]
    unique within that home (e.g. the interval in ms). After every update the
    runtime diffs the flattened leaves by [(home, key)]:
    - a leaf that disappears has its stop function called;
    - a new leaf is started with [start(dispatch)];
    - a leaf whose [(home, key)] persists is NOT restarted, but the runtime
      must route its messages through the newest [tagger] (so [Sub.map] and
      closures over the model stay fresh without restart).

    [start] receives a dispatch for the leaf's raw payload type and returns a
    stop function. */

type t('msg) =
  | None
  | Batch(list(t('msg)))
  | Leaf(leaf('a, 'msg)): t('msg)
and leaf('a, 'msg) = {
  home: string,
  key: string,
  start: ('a => unit) => (unit => unit),
  tagger: 'a => 'msg,
};

let none: t('msg);
let batch: list(t('msg)) => t('msg);
let map: ('a => 'b, t('a)) => t('b);
/** [make(~home, ~key, ~start, ~tagger)] builds a leaf. */
let make:
  (~home: string, ~key: string, ~start: ('a => unit) => (unit => unit), ~tagger: 'a => 'msg) => t('msg);
