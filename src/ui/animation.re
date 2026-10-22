open Tea_core;
module Html = Tea_html.Html;
module Dom = Tea_browser.Browser_dom;
module Platform = Tea_dom.Browser_platform;

type phase =
  | Closed
  | Entering
  | Open
  | Leaving;
type model = {
  id: string,
  phase,
  initial_frame: bool,
  generation: int,
};
type msg =
  | Painted(int)
  | Finished(int);
type out_msg =
  | Entered
  | Left;

let init = (~id, ~open_, ~appear=false, ()) => {
  id,
  phase: open_ ? appear ? Entering : Open : Closed,
  initial_frame: open_ && appear,
  generation: 0,
};
let phase = model => model.phase;
let is_visible = model => model.phase != Closed;
let should_render = is_visible;
let show = model =>
  switch (model.phase) {
  | Open
  | Entering => (model, Cmd.none)
  | Closed
  | Leaving => (
      {
        ...model,
        phase: Entering,
        initial_frame: true,
        generation: model.generation + 1,
      },
      Cmd.none,
    )
  };
let hide = model =>
  switch (model.phase) {
  | Closed
  | Leaving => (model, Cmd.none)
  | Open
  | Entering => (
      {
        ...model,
        phase: Leaving,
        initial_frame: false,
        generation: model.generation + 1,
      },
      Cmd.none,
    )
  };
let update = (msg, model) =>
  switch (msg) {
  | Painted(generation)
      when
        generation == model.generation
        && model.phase == Entering
        && model.initial_frame => (
      {
        ...model,
        initial_frame: false,
      },
      Cmd.none,
      [],
    )
  | Finished(generation) when generation == model.generation =>
    switch (model.phase) {
    | Entering when !model.initial_frame => (
        {
          ...model,
          phase: Open,
        },
        Cmd.none,
        [Entered],
      )
    | Leaving => (
        {
          ...model,
          phase: Closed,
        },
        Cmd.none,
        [Left],
      )
    | _ => (model, Cmd.none, [])
    }
  | _ => (model, Cmd.none, [])
  };
let subscriptions = model =>
  switch (model.phase) {
  | Closed
  | Open => Sub.none
  | Entering
  | Leaving =>
    let initial = model.phase == Entering && model.initial_frame;
    let message =
      initial ? Painted(model.generation) : Finished(model.generation);
    Sub.make(
      ~home="Tea_ui.Animation",
      ~key=
        Ui_sub.prefix(model.id)
        ++ ":"
        ++ string_of_int(model.generation)
        ++ (initial ? ":paint" : ":wait"),
      ~tagger=message => message,
      ~start=
        dispatch => {
          let live = ref(true);
          let cancel = ref(() => ());
          let deliver = () =>
            if (live^) {
              dispatch(message);
            };
          if (initial) {
            /* A single rAF shares the draw's pre-paint callback batch. The
               nested rAF crosses a paint before removing data-closed. Tea
               batches the resulting model's draw on its next render frame. */
            Platform.request_animation_frame(_ =>
              if (live^) {
                Platform.request_animation_frame(_ => deliver());
              }
            );
          } else {
            /* Browser_dom takes its snapshot after the updated attrs render.
               Missing/removed nodes finish too, rather than strand a parent. */
            /* Gate task creation too: a reversal/shutdown before this frame
               must not start an obsolete observer or watchdog. */
            Platform.request_animation_frame(_ =>
              if (live^) {
                cancel := Dom.watch_animations(model.id, _ => deliver());
              }
            );
          };
          () => {
            live := false;
            cancel^();
          };
        },
    );
  };
let attrs = model => [
  Ui_attr.data_flag(
    "closed",
    model.phase == Closed
    || model.phase == Leaving
    || model.phase == Entering
    && model.initial_frame,
  ),
  Ui_attr.data_flag("enter", model.phase == Entering),
  Ui_attr.data_flag("leave", model.phase == Leaving),
  Ui_attr.data_flag(
    "transition",
    model.phase == Entering || model.phase == Leaving,
  ),
  Html.hidden(model.phase == Closed),
];
