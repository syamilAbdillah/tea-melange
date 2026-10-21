type t('msg) =
  | None
  | Batch(list(t('msg)))
  | Effect(('msg => unit) => unit);

let none = None;
let batch = cmds => Batch(cmds);
let rec map = (f, cmd) =>
  switch (cmd) {
  | None => None
  | Batch(cmds) => Batch(List.map(map(f), cmds))
  | Effect(run) => Effect(dispatch => run(x => dispatch(f(x))))
  };
let effect = run => Effect(run);
let msg = m => Effect(dispatch => dispatch(m));
