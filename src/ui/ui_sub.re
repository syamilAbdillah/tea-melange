module Sub = Tea_core.Sub;
/** Length-prefix an id so joined identity keys cannot alias. */
let prefix = id => string_of_int(String.length(id)) ++ ":" ++ id;
/** Re-key every leaf under [key]. Browser_events sources have global
    identities; scoping keeps independent instances from sharing one. */
let rec scoped = (~key, sub) => {
  let scope = leaf_key => prefix(key) ++ leaf_key;
  switch (sub) {
  | Sub.None => Sub.none
  | Sub.Batch(subs) => Sub.batch(List.map(scoped(~key), subs))
  | Sub.Leaf(leaf) =>
    Sub.Leaf({
      ...leaf,
      key: scope(leaf.key),
    })
  | Sub.Filtered(leaf) =>
    Sub.Filtered({
      ...leaf,
      key: scope(leaf.key),
    })
  };
};
