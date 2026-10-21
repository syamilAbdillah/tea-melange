open Tea_core;

let assertions = ref(0);
let equal = (label, actual, expected) => {
  incr(assertions);
  if (actual != expected) {
    failwith(label);
  };
};
let check = (label, condition) => equal(label, condition, true);

module Fake = {
  let frames = Queue.create();
  let requests = ref(0);
  let clock = ref(0.0);
  let now = () => clock^;
  let request_animation_frame = callback => {
    incr(requests);
    Queue.add(callback, frames);
  };
  let flush_frame = () => {
    check("a frame is scheduled", !Queue.is_empty(frames));
    let callback = Queue.take(frames);
    clock := clock^ +. 16.0;
    callback(clock^);
  };
  let reset = () => {
    Queue.clear(frames);
    requests := 0;
    clock := 0.0;
  };
};
module R = Runtime.Make(Fake);

let counter = {
  Program.init: () => (0, Cmd.none),
  update: (amount, model) => (model + amount, Cmd.none),
  view: model => model,
  subscriptions: _ => Sub.none,
};

let test_animator = () => {
  Fake.reset();
  let draws = ref(0);
  let a =
    Runtime.Animator.make(~raf=Fake.request_animation_frame, ~draw=() =>
      incr(draws)
    );
  equal(
    "animator initially idle",
    Runtime.Animator.state(a),
    Runtime.Animator.No_request,
  );
  Runtime.Animator.request(a);
  Runtime.Animator.request(a);
  equal(
    "request enters Pending",
    Runtime.Animator.state(a),
    Runtime.Animator.Pending,
  );
  equal("coalesced raf", Fake.requests^, 1);
  Fake.flush_frame();
  equal("frame draws once", draws^, 1);
  equal(
    "frame leaves spare",
    Runtime.Animator.state(a),
    Runtime.Animator.Extra_request,
  );
  Runtime.Animator.request(a);
  equal("spare reused", Fake.requests^, 2);
  Fake.flush_frame();
  equal("reused frame draws", draws^, 2);
  Fake.flush_frame();
  equal("spare frame is clean", draws^, 2);
  equal(
    "spare returns idle",
    Runtime.Animator.state(a),
    Runtime.Animator.No_request,
  );
};

let test_counter = () => {
  Fake.reset();
  let renders = ref([]);
  let log = ref([]);
  let h =
    R.start(
      ~program=counter,
      ~flags=(),
      ~render=(view, _) => renders := [view, ...renders^],
      ~on_msg=(msg, model) => log := [(msg, model), ...log^],
      (),
    );
  equal("synchronous initial render", renders^, [0]);
  for (_i in 1 to 100) {
    h.dispatch(1);
  };
  equal("counter commits synchronously", h.model(), 100);
  equal("one raf for 100 messages", Fake.requests^, 1);
  equal("no eager render", renders^, [0]);
  Fake.flush_frame();
  equal("one render for 100 messages", renders^, [100, 0]);
  Fake.flush_frame();
  equal("spare doesn't render", renders^, [100, 0]);
  let ordered = List.rev(log^);
  equal("complete log", List.length(ordered), 100);
  let replay =
    List.fold_left(
      (model, (msg, committed)) => {
        let (next, _) = counter.update(msg, model);
        equal("on_msg sees committed model", next, committed);
        next;
      },
      fst(counter.init()),
      ordered,
    );
  equal("replayed model", replay, h.model());
  h.shutdown();
};

let test_effect_frame_order = () =>
  List.iter(
    state => {
      Fake.reset();
      let events = ref([]);
      let add = event => events := events^ @ [event];
      let program = {
        ...counter,
        update: (amount, model) => (
          model + amount,
          amount == 1
            ? Cmd.effect(_ =>
                Fake.request_animation_frame(_ => add("effect"))
              )
            : Cmd.none,
        ),
      };
      let h =
        R.start(~program, ~flags=(), ~render=(_, _) => add("draw"), ());
      switch (state) {
      | Runtime.Animator.No_request => ()
      | Runtime.Animator.Pending => h.dispatch(0)
      | Runtime.Animator.Extra_request =>
        h.dispatch(0);
        Fake.flush_frame();
      };
      events := [];
      h.dispatch(1);
      while (!Queue.is_empty(Fake.frames)) {
        Fake.flush_frame();
      };
      equal(
        "draw precedes effect callback in every animator state",
        events^,
        ["draw", "effect"],
      );
      h.shutdown();
    },
    [
      Runtime.Animator.No_request,
      Runtime.Animator.Pending,
      Runtime.Animator.Extra_request,
    ],
  );

let test_filtered_subscriptions = () => {
  Fake.reset();
  let send = ref(_ => ());
  let starts = ref(0);
  let stops = ref(0);
  let source =
    Sub.make(
      ~home="filtered",
      ~key="one",
      ~start=
        dispatch => {
          incr(starts);
          send := dispatch;
          () => incr(stops);
        },
      ~tagger=x => x,
    );
  let h =
    R.start(
      ~program={
        ...counter,
        subscriptions: model =>
          Sub.batch([
            Sub.filter_map(raw => raw > model ? Some(raw) : None, source),
            Sub.filter_map(
              _ => failwith("duplicate filter started"),
              source,
            ),
          ]),
      },
      ~flags=(),
      ~render=(_, _) => (),
      (),
    );
  send^(0);
  equal("rejected payload never updates", h.model(), 0);
  send^(2);
  equal("accepted filtered payload", h.model(), 2);
  send^(1);
  equal("retained filter is fresh", h.model(), 2);
  send^(3);
  equal("new filter accepts", h.model(), 5);
  equal("filter changes do not restart source", starts^, 1);
  h.shutdown();
  equal("filtered source stopped", stops^, 1);
  send^(100);
  equal("stopped filtered callback is inert", h.model(), 5);
  let nested =
    Sub.filter_map(
      x => x == 4 ? Some(x + 1) : None,
      Sub.map(
        x => x + 1,
        Sub.filter_map(x => x > 0 ? Some(x + 1) : None, source),
      ),
    );
  switch (nested) {
  | Sub.Filtered(leaf) =>
    let observed = ref(Some(-1));
    let stop = leaf.start(raw => observed := leaf.tagger(raw));
    send^(0);
    equal("filter composition rejected", observed^, None);
    send^(2);
    equal("filter composition mapped", observed^, Some(5));
    stop();
  | _ => failwith("expected filtered leaf")
  };
  equal("filter none", Sub.filter_map(x => Some(x), Sub.none), Sub.none);
  ignore(Sub.map(x => x + 1, Sub.batch([nested])));
};

let test_render_dispatch = () => {
  Fake.reset();
  let rendering = ref(false);
  let renders = ref([]);
  let program = {
    ...counter,
    update: (msg, model) => {
      check("update never re-enters render", ! rendering^);
      counter.update(msg, model);
    },
  };
  let h =
    R.start(
      ~program,
      ~flags=(),
      ~render=
        (view, dispatch) => {
          rendering := true;
          renders := [view, ...renders^];
          if (view == 1) {
            dispatch(1);
          };
          rendering := false;
        },
      (),
    );
  h.dispatch(1);
  Fake.flush_frame();
  equal("render message committed after draw", h.model(), 2);
  equal("only spare raf scheduled", Fake.requests^, 2);
  equal("one frame still pending", Queue.length(Fake.frames), 1);
  Fake.flush_frame();
  equal("render message not lost", renders^, [2, 1, 0]);
  Fake.flush_frame();
  equal("eventual clean spare", Queue.length(Fake.frames), 0);
  h.dispatch(1);
  let before = renders^;
  h.shutdown();
  h.dispatch(100);
  Fake.flush_frame();
  equal("shutdown ignores frames", renders^, before);
  equal("shutdown ignores messages", h.model(), 3);
};

let test_commands_and_init = () => {
  Fake.reset();
  let events = ref([]);
  let add = event => events := List.append(events^, [event]);
  let hook_dispatch = ref(_ => ());
  let initial_sub =
    Sub.make(
      ~home="init",
      ~key="one",
      ~start=
        dispatch => {
          add("sub");
          dispatch(2);
          () => ();
        },
      ~tagger=x => x,
    );
  let program = {
    Program.init: () => (
      0,
      Cmd.effect(dispatch => {
        add("init cmd");
        dispatch(1);
      }),
    ),
    update: (msg, model) => {
      add("update " ++ string_of_int(msg));
      (
        model + msg,
        msg == 10
          ? Cmd.batch([
              Cmd.effect(dispatch => {
                add("cmd a");
                dispatch(20);
                add("cmd a end");
              }),
              Cmd.batch([
                Cmd.map(x => x + 1, Cmd.msg(29)),
                Cmd.effect(_ => add("cmd b")),
              ]),
              Cmd.msg(40),
            ])
          : Cmd.none,
      );
    },
    view: x => x,
    subscriptions: _ => initial_sub,
  };
  let h =
    R.start(
      ~program,
      ~flags=(),
      ~render=
        (view, dispatch) => {
          add("render " ++ string_of_int(view));
          if (view == 0) {
            dispatch(3);
          };
        },
      ~on_msg=
        (msg, _) => {
          add("hook " ++ string_of_int(msg));
          if (msg == 10) {
            hook_dispatch^(50);
          };
        },
      (),
    );
  equal(
    "initialization order and FIFO",
    events^,
    [
      "render 0",
      "init cmd",
      "sub",
      "update 3",
      "hook 3",
      "update 1",
      "hook 1",
      "update 2",
      "hook 2",
    ],
  );
  hook_dispatch := h.dispatch;
  events := [];
  h.dispatch(10);
  equal(
    "commands depth-first and queued; hook is before commands",
    events^,
    [
      "update 10",
      "hook 10",
      "cmd a",
      "cmd a end",
      "cmd b",
      "update 50",
      "hook 50",
      "update 20",
      "hook 20",
      "update 30",
      "hook 30",
      "update 40",
      "hook 40",
    ],
  );
  equal("all command models committed", h.model(), 156);
  h.shutdown();
};

type timer_msg =
  | Advance
  | Tick(int)
  | Second
  | Remove;
type timer_model = {
  count: int,
  last: int,
  second: bool,
  enabled: bool,
};
let test_subscriptions = () => {
  Fake.reset();
  let starts = ref([]);
  let stops = ref([]);
  let senders = Hashtbl.create(4);
  let timer = (key, offset) =>
    Sub.map(
      raw => Tick(raw + offset),
      Sub.make(
        ~home="Time.every",
        ~key,
        ~start=
          dispatch => {
            starts := List.append(starts^, [key]);
            Hashtbl.replace(senders, key, dispatch);
            () => stops := List.append(stops^, [key]);
          },
        ~tagger=raw => raw,
      ),
    );
  let program = {
    Program.init: () => (
      {
        count: 0,
        last: 0,
        second: false,
        enabled: true,
      },
      Cmd.none,
    ),
    update: (msg, model) => (
      switch (msg) {
      | Advance => {
          ...model,
          count: model.count + 1,
        }
      | Tick(last) => {
          ...model,
          last,
        }
      | Second => {
          ...model,
          second: true,
        }
      | Remove => {
          ...model,
          enabled: false,
        }
      },
      Cmd.none,
    ),
    view: x => x,
    subscriptions: model =>
      model.enabled
        ? Sub.batch([
            Sub.batch([timer("1000", model.count)]),
            /* First duplicate wins, even with a different raw payload type. */
            Sub.make(
              ~home="Time.every",
              ~key="1000",
              ~start=_ => failwith("duplicate started"),
              ~tagger=(_raw: string) => Tick(-999),
            ),
            model.second
              ? Sub.map(x => x, timer("2000", model.count)) : Sub.none,
          ])
        : Sub.none,
  };
  let h = R.start(~program, ~flags=(), ~render=(_, _) => (), ());
  for (_i in 1 to 100) {
    h.dispatch(Advance);
  };
  equal("timer never restarted", starts^, ["1000"]);
  Hashtbl.find(senders, "1000", 7);
  equal("retained timer gets latest tagger", h.model().last, 107);
  h.dispatch(Second);
  equal("only new key starts", starts^, ["1000", "2000"]);
  equal("kept subscriptions not stopped", stops^, []);
  h.dispatch(Remove);
  equal("removed subscriptions stopped", stops^, ["1000", "2000"]);
  let before = h.model();
  Hashtbl.find(senders, "1000", 9);
  equal("stale removed source ignored", h.model(), before);
  h.shutdown();
  equal("removed sources not stopped twice", stops^, ["1000", "2000"]);
  let h2 = R.start(~program, ~flags=(), ~render=(_, _) => (), ());
  h2.dispatch(Second);
  let before = h2.model();
  h2.shutdown();
  h2.shutdown();
  equal(
    "shutdown stops every source exactly once",
    stops^,
    ["1000", "2000", "1000", "2000"],
  );
  Hashtbl.find(senders, "1000", 12);
  h2.dispatch(Advance);
  equal("shutdown callbacks inert", h2.model(), before);
};

let test_subscription_start_dispatch = () => {
  Fake.reset();
  let old_send = ref(_ => ());
  let starts = ref(0);
  let stops = ref(0);
  let in_start = ref(false);
  let log = ref([]);
  let program = {
    ...counter,
    update: (msg, model) => {
      check("subscription start never re-enters update", ! in_start^);
      counter.update(msg, model);
    },
    subscriptions: model => {
      let old =
        Sub.make(
          ~home="old",
          ~key="same",
          ~start=
            send => {
              incr(starts);
              old_send := send;
              () => incr(stops);
            },
          ~tagger=raw => raw + model,
        );
      let added =
        Sub.make(
          ~home="new",
          ~key="same",
          ~start=
            send => {
              in_start := true;
              incr(starts);
              old_send^(10);
              send(20);
              in_start := false;
              () => incr(stops);
            },
          ~tagger=raw => raw,
        );
      /* A new source precedes the retained one in the flattened tree. */
      model == 0 ? old : Sub.batch([added, old]);
    },
  };
  let h =
    R.start(
      ~program,
      ~flags=(),
      ~render=(_, _) => (),
      ~on_msg=(msg, _) => log := List.append(log^, [msg]),
      (),
    );
  h.dispatch(1);
  equal(
    "all retained taggers refreshed before added start",
    log^,
    [1, 11, 20],
  );
  equal("subscription messages queued FIFO", h.model(), 32);
  equal("same key in different homes is independent", starts^, 2);
  h.shutdown();
  equal("both homes stopped", stops^, 2);
};

exception Expected;
let test_update_exception = () => {
  Fake.reset();
  let callback = ref(_ => ());
  let log = ref([]);
  let program = {
    ...counter,
    update: (msg, model) => {
      if (msg == (-1)) {
        callback^(5);
        raise(Expected);
      };
      (model + msg, Cmd.none);
    },
  };
  let h =
    R.start(
      ~program,
      ~flags=(),
      ~render=(_, _) => (),
      ~on_msg=(msg, _) => log := List.append(log^, [msg]),
      (),
    );
  callback := h.dispatch;
  let raised =
    try(
      {
        h.dispatch(-1);
        false;
      }
    ) {
    | Expected => true
    };
  check("update exception propagates", raised);
  equal("failed update not committed", h.model(), 0);
  h.dispatch(2);
  equal("guard released and queued messages retained", h.model(), 7);
  equal("failed update not logged; FIFO preserved", log^, [5, 2]);
  Fake.flush_frame();
  h.shutdown();
};

let test_tasks = () => {
  let result_of = task => {
    let outcome = ref(None);
    task(result => outcome := Some(result));
    outcome^;
  };
  equal("task succeed", result_of(Task.succeed(3)), Some(Ok(3)));
  equal("task fail", result_of(Task.fail("bad")), Some(Error("bad")));
  equal(
    "task map",
    result_of(Task.map(x => x + 1, Task.succeed(3))),
    Some(Ok(4)),
  );
  equal(
    "map preserves errors",
    result_of(Task.map(x => x + 1, Task.fail("bad"))),
    Some(Error("bad")),
  );
  equal(
    "map_error",
    result_of(Task.map_error(String.length, Task.fail("bad"))),
    Some(Error(3)),
  );
  equal(
    "map_error preserves success",
    result_of(Task.map_error(String.length, Task.succeed(2))),
    Some(Ok(2)),
  );
  equal(
    "and_then",
    result_of(Task.and_then(x => Task.succeed(x + 1), Task.succeed(4))),
    Some(Ok(5)),
  );
  equal(
    "and_then short circuits",
    result_of(
      Task.and_then(_ => failwith("unexpected"), Task.fail("bad")),
    ),
    Some(Error("bad")),
  );
  equal(
    "on_error",
    result_of(
      Task.on_error(
        error => Task.succeed(String.length(error)),
        Task.fail("bad"),
      ),
    ),
    Some(Ok(3)),
  );
  equal(
    "on_error preserves success",
    result_of(Task.on_error(_ => failwith("unexpected"), Task.succeed(8))),
    Some(Ok(8)),
  );
  let order = ref([]);
  let tracked = (n, done_) => {
    order := List.append(order^, [n]);
    done_(Ok(n));
  };
  let task = Task.sequence([tracked(1), tracked(2), tracked(3)]);
  equal("sequence lazy", order^, []);
  equal("sequence values", result_of(task), Some(Ok([1, 2, 3])));
  equal("sequence order", order^, [1, 2, 3]);
  equal("sequence empty", result_of(Task.sequence([])), Some(Ok([])));
  equal(
    "sequence stops on error",
    result_of(
      Task.sequence([Task.fail("bad"), _ => failwith("unexpected")]),
    ),
    Some(Error("bad")),
  );
  let resume = ref(_ => ());
  let observed = ref(None);
  Task.map(
    x => x + 1,
    done_ => resume := done_,
    value => observed := Some(value),
  );
  equal("async task waits", observed^, None);
  resume^(Ok(9));
  equal("async task result", observed^, Some(Ok(10)));
  Fake.reset();
  let h =
    R.start(
      ~program={
        ...counter,
        init: () => (
          0,
          Cmd.batch([
            Task.perform(x => x, Task.succeed(4)),
            Task.perform(x => x, Task.fail()),
            Task.attempt(
              result =>
                switch (result) {
                | Ok(x) => x
                | Error(x) => x
                },
              Task.fail(5),
            ),
            Task.attempt(
              result =>
                switch (result) {
                | Ok(x) => x
                | Error(x) => x
                },
              Task.succeed(6),
            ),
          ]),
        ),
      },
      ~flags=(),
      ~render=(_, _) => (),
      (),
    );
  equal("perform/attempt dispatch via runtime", h.model(), 15);
  h.shutdown();
};

let () = {
  test_animator();
  test_counter();
  test_effect_frame_order();
  test_filtered_subscriptions();
  test_render_dispatch();
  test_commands_and_init();
  test_subscriptions();
  test_subscription_start_dispatch();
  test_update_exception();
  test_tasks();
  Printf.printf("P3 runtime + Task: %d assertions passed\n", assertions^);
};
