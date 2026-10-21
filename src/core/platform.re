/** What the runtime needs from its host. [tea.dom] provides the browser
    implementation; native tests provide a fake with a manual clock. */
module type S = {
  /** Schedule [f] for the next frame; [f] receives a timestamp in ms. */
  let request_animation_frame: (float => unit) => unit;
  /** Current time in ms (only used by the runtime for frame bookkeeping). */
  let now: unit => float;
};
