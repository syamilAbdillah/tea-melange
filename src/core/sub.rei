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
  | /** A source whose newest tagger may reject a payload without dispatch. */
    Filtered(
      leaf('a, option('msg)),
    )
    : t('msg)
and leaf('a, 'msg) = {
  home: string,
  key: string,
  start: ('a => unit, unit) => unit,
  tagger: 'a => 'msg,
};

/** No active event sources. */
let none: t('msg);
/** Combine event sources; duplicate identities keep their first leaf. */
let batch: list(t('msg)) => t('msg);
/** Transform messages without changing source identities. */
let map: ('a => 'b, t('a)) => t('b);
/** Transform or reject payloads without restarting sources. Like [map], the
    newest filter/tagger is used for every retained identity. */
let filter_map: ('a => option('b), t('a)) => t('b);
/** [make(~home, ~key, ~start, ~tagger)] builds a leaf. */
let make:
  (
    ~home: string,
    ~key: string,
    ~start: ('a => unit, unit) => unit,
    ~tagger: 'a => 'msg
  ) =>
  t('msg);
