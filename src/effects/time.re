type posix = float;
let now_task = done_ => done_(Ok(Clock.now()));
let now = tagger => Tea_core.Task.perform(tagger, now_task);
let every = (ms, tagger) => {
  if (ms <= 0. || !Float.is_finite(ms)) {
    invalid_arg("Time.every: positive finite interval required");
  };
  Tea_core.Sub.make(
    ~home="Time.every",
    ~key=string_of_float(ms),
    ~tagger,
    ~start=dispatch => {
      let id = Clock.set_interval(() => dispatch(Clock.now()), ms);
      () => Clock.clear_interval(id);
    },
  );
};
