/** The message loop (contract — see docs/runtime-contract.md).

    - One queue. [dispatch] enqueues; if no update is in progress the queue is
      drained synchronously: for each msg, [update] → commit model → run cmds
      → diff subscriptions. Messages dispatched during that (by cmds, subs or
      handlers) are appended and processed in order. Never re-entrant.
    - Rendering is decoupled: after the queue drains, the runtime asks the
      animator for a frame. [view] + [render] run only inside the frame,
      at most once per frame regardless of how many msgs were processed
      (ADR-04, Elm's _Browser_makeAnimator: NO_REQUEST → PENDING → EXTRA).
    - The very first render happens synchronously in [start] (so the DOM
      exists immediately), then init cmds run and init subs start. */

module Animator: {
  type state =
    | No_request
    | Pending
    | Extra_request;
  type t;
  /** [make(~raf, ~draw)] — [draw] is called inside a frame. */
  let make: (~raf: (float => unit) => unit, ~draw: unit => unit) => t;
  /** Mark the view dirty; requests a frame if needed. */
  let request: t => unit;
  let state: t => state;
};

type handle('model, 'msg) = {
  dispatch: 'msg => unit,
  /** Current committed model (for tests/debugging). */
  model: unit => 'model,
  /** Stop all subscriptions and ignore further messages. */
  shutdown: unit => unit,
};

module Make: (_: Platform.S) => {
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
