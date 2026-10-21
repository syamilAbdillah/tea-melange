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

let none = None;
let batch = subs => Batch(subs);
let rec map: type a b. (a => b, t(a)) => t(b) =
  (f, sub) =>
    switch (sub) {
    | None => None
    | Batch(subs) => Batch(List.map(map(f), subs))
    | Leaf(leaf) => Leaf({...leaf, tagger: x => f(leaf.tagger(x))})
    };
let make = (~home, ~key, ~start, ~tagger) => Leaf({home, key, start, tagger});
