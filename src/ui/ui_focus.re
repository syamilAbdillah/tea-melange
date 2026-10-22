/* A pending post-render focus request. Each request bumps the generation,
   which keys the subscription, so a newer request cancels an older one and
   stale completions are ignored. */
module Events = Tea_browser.Browser_events;
type t = {
  generation: int,
  target: option(Events.focus_target),
};
let none = {
  generation: 0,
  target: None,
};
let request = (t, target) => {
  generation: t.generation + 1,
  target: Some(target),
};
let element = (t, id) => request(t, Events.Element(id));
let cancel = t => {
  ...t,
  target: None,
};
let pending = t => Option.is_some(t.target);
/** Clear the request if [generation] is current; returns whether it was. */
let finish = (t, generation) =>
  generation == t.generation ? (cancel(t), true) : (t, false);
/** [home] and [id] scope the subscription identity. */
let sub = (~home, ~id, ~from=?, ~ready=?, ~prevent_scroll=?, t, tagger) =>
  switch (t.target) {
  | None => Tea_core.Sub.none
  | Some(target) =>
    let generation = t.generation;
    Events.focus(
      ~key=
        home
        ++ ":"
        ++ string_of_int(String.length(id))
        ++ ":"
        ++ id
        ++ ":"
        ++ string_of_int(generation),
      ~target,
      ~from?,
      ~ready?,
      ~prevent_scroll?,
      focused =>
      tagger(generation, focused)
    );
  };
