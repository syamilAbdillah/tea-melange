[@mel.scope "Math"] external random: unit => float = "random";
let seed_cmd = tagger =>
  Tea_core.Cmd.effect(dispatch =>
    dispatch(
      tagger(
        Tea_core.Random.initial_seed(int_of_float(random() *. 2147483647.)),
      ),
    )
  );
let generate = (tagger, generator) =>
  Tea_core.Cmd.map(
    seed => tagger(fst(Tea_core.Random.step(generator, seed))),
    seed_cmd(seed => seed),
  );
