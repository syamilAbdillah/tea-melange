type t('msg) =
  | None
  | Batch(list(t('msg)))
  | Leaf(leaf('a, 'msg)): t('msg)
  | Filtered(leaf('a, option('msg))): t('msg)
and leaf('a, 'msg) = {
  home: string,
  key: string,
  start: ('a => unit, unit) => unit,
  tagger: 'a => 'msg,
};

let none = None;
let batch = subs => Batch(subs);
let rec map: type a b. (a => b, t(a)) => t(b) = (f, sub) =>
  switch (sub) {
  | None => None
  | Batch(subs) => Batch(List.map(map(f), subs))
  | Leaf(leaf) =>
    Leaf({
      ...leaf,
      tagger: x => f(leaf.tagger(x)),
    })
  | Filtered(leaf) =>
    Filtered({
      ...leaf,
      tagger: x => Option.map(f, leaf.tagger(x)),
    })
  };
let rec filter_map: type a b. (a => option(b), t(a)) => t(b) = (f, sub) =>
  switch (sub) {
  | None => None
  | Batch(subs) => Batch(List.map(filter_map(f), subs))
  | Leaf(leaf) =>
    Filtered({
      ...leaf,
      tagger: x => f(leaf.tagger(x)),
    })
  | Filtered(leaf) =>
    Filtered({
      ...leaf,
      tagger: x => Option.bind(leaf.tagger(x), f),
    })
  };
let make = (~home, ~key, ~start, ~tagger) =>
  Leaf({
    home,
    key,
    start,
    tagger,
  });
