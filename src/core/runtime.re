/* PLACEHOLDER — to be implemented in task B (P3 runtime). */
module Animator = {
  type state =
    | No_request
    | Pending
    | Extra_request;
  type t = {state};
  let make = (~raf as _, ~draw as _) => failwith("TODO: Runtime.Animator");
  let request = _ => failwith("TODO: Runtime.Animator.request");
  let state = t => t.state;
};

type handle('model, 'msg) = {
  dispatch: 'msg => unit,
  model: unit => 'model,
  shutdown: unit => unit,
};

module Make = (P: Platform.S) => {
  let _ = P.now;
  let start = (~program as _, ~flags as _, ~render as _, ~on_msg as _=?, ()) =>
    failwith("TODO: Runtime.start");
};
