/** Pure message loop; see docs/runtime-contract.md.

    Dispatch uses one non-reentrant FIFO, including during initialization and
    rendering. Each message runs update, commits the model, calls on_msg, runs
    commands depth-first left-to-right, diffs subscriptions, then requests a
    frame. Exceptions release the drain guard; queued messages remain available
    for the next dispatch. An update that raises is not committed or logged.

    The first render is synchronous, before init commands and subscriptions.
    Subsequent views/renders run at most once per frame. Dispatch during render
    waits until render returns, then uses the animator's spare frame.

    Subscriptions are identified by (home,key), with the first flattened leaf
    winning duplicates. Kept sources use the newest tagger without restarting.
    A given identity must retain its raw payload type for its whole lifetime. */

/** Elm's three-state animator, independent of the message loop. */
module Animator: {
  /** Idle, dirty with a frame scheduled, or clean with a spare frame scheduled. */
  type state =
    | No_request
    | Pending
    | Extra_request;
  /** Mutable scheduling state owned by one animator. */
  type t;
  /** Construct an idle animator. [raf] must schedule, not synchronously call,
      its callback. [draw] is called inside a frame. */
  let make: (~raf: (float => unit) => unit, ~draw: unit => unit) => t;
  /** Mark the view dirty; requests a frame only if none is scheduled. */
  let request: t => unit;
  /** Inspect the current scheduling state. */
  let state: t => state;
};

/** A running program and its lifecycle operations. */
type handle('model, 'msg) = {
  /** Enqueue a message and drain synchronously when outside a transaction. */
  dispatch: 'msg => unit,
  /** Current committed model (for tests/debugging). */
  model: unit => 'model,
  /** Idempotently stop subscriptions; ignore future dispatches and frames. */
  shutdown: unit => unit,
};

/** Instantiate the runtime with an asynchronous frame scheduler. */
module Make:
  (Platform.S) =>
   {
    /** Initialize and synchronously render a program, then run init effects and
      subscriptions. [on_msg] observes every successfully committed update,
      before its effects; recording messages permits replay through update. */
    let start:
      (
        ~program: Program.t('flags, 'model, 'msg, 'view),
        ~flags: 'flags,
        ~render: ('view, 'msg => unit) => unit,
        ~on_msg: ('msg, 'model) => unit=?,
        unit
      ) =>
      handle('model, 'msg);
  };
