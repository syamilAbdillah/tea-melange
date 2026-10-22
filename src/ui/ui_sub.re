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
/** Start a subscription outside the runtime; the result stops it. For
    composing Browser_events sources inside one hand-written leaf. */
let rec connect: type a. (Sub.t(a), a => unit, unit) => unit =
        (sub, dispatch) =>
  switch (sub) {
  | Sub.None => (() => ())
  | Sub.Batch(subs) =>
    let stops = List.map(sub => connect(sub, dispatch), subs);
    (() => List.iter(stop => stop(), stops));
  | Sub.Leaf(leaf) => leaf.start(value => dispatch(leaf.tagger(value)))
  | Sub.Filtered(leaf) =>
    leaf.start(value => Option.iter(dispatch, leaf.tagger(value)))
  };
