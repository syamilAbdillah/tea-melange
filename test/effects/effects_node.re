let () = Random_cases.Random_test.run();
module E = Tea_effects;
module C = Tea_core;
type timer;
external later: (unit => unit, int) => timer = "setTimeout";
external clear: timer => unit = "clearTimeout";
let now: unit => float = [%mel.raw {|function() { return Date.now(); }|}];
module P = {
  let now = now;
  let request_animation_frame = callback => {
    ignore(later(() => callback(now()), 1));
  };
};
module R = C.Runtime.Make(P);
type model = {
  ticks: int,
  frames: int,
  deltas: int,
  fresh: bool,
};
type msg =
  | Tick(int)
  | Frame
  | Delta(float);
let complete = m => m.ticks >= 5 && m.frames >= 2 && m.deltas >= 2;
let program: C.Program.t(unit, model, msg, unit) = {
  init: () => (
    {
      ticks: 0,
      frames: 0,
      deltas: 0,
      fresh: false,
    },
    C.Cmd.none,
  ),
  update: (model, msg) => {
    let model =
      switch (msg) {
      | Tick(n) => {
          ...model,
          ticks: model.ticks + 1,
          fresh: model.fresh || n == 1,
        }
      | Frame => {
          ...model,
          frames: model.frames + 1,
        }
      | Delta(dt) =>
        assert(dt >= 0.);
        {
          ...model,
          deltas: model.deltas + 1,
        };
      };
    (model, C.Cmd.none);
  },
  view: _ => (),
  subscriptions: model =>
    complete(model)
      ? C.Sub.none
      : C.Sub.batch([
          E.Time.every(5., _ => Tick(model.ticks == 0 ? 0 : 1)),
          E.Animation_frame.on_animation_frame(_ => Frame),
          E.Animation_frame.on_animation_frame_delta(dt => Delta(dt)),
        ]),
};
let handle = R.start(~program, ~flags=(), ~render=(_, _) => (), ());
let () = {
  let watchdog = later(() => failwith("effects timed out"), 2000);
  E.Time.now_task(result =>
    assert(
      switch (result) {
      | Ok(t) => t > 0.
      | _ => false
      },
    )
  );
  switch (E.Random.generate(x => x, C.Random.int(1, 10))) {
  | C.Cmd.Effect(run) => run(x => assert(x >= 1 && x <= 10))
  | _ => assert(false)
  };
  let rec await_stop = () =>
    if (complete(handle.model())) {
      let snapshot = handle.model();
      assert(snapshot.fresh);
      ignore(
        later(
          () => {
            assert(handle.model() == snapshot);
            handle.shutdown();
            handle.shutdown();
            clear(watchdog);
            print_endline(
              "effects: runtime intervals, newest tagger, unsubscribe and animation fallback passed",
            );
          },
          70,
        ),
      );
    } else {
      ignore(later(await_stop, 10));
    };
  await_stop();
};
