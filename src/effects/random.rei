/** Obtain an unpredictable initial seed when the command executes. Not cryptographic. */
let seed_cmd: (Tea_core.Random.seed => 'msg) => Tea_core.Cmd.t('msg);
/** Generate a value using a fresh seed; pure generators live in Tea_core.Random. */
let generate:
  ('a => 'msg, Tea_core.Random.generator('a)) => Tea_core.Cmd.t('msg);
