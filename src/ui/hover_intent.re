module Html = Tea_html.Html;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module D = Tea_core.Decode;
module V = Tea_vdom.Vdom;
module B = {
  type timer;
  external timeout: (unit => unit, int) => timer = "setTimeout";
  external clear: timer => unit = "clearTimeout";
  external window: Dom.window = "window";
  external document: Dom.document = "document";
  [@mel.scope "performance"] external now: unit => float = "now";
  [@mel.send]
  external listen: ('a, string, Dom.event => unit, bool) => unit =
    "addEventListener";
  [@mel.send]
  external unlisten: ('a, string, Dom.event => unit, bool) => unit =
    "removeEventListener";
  [@mel.get]
  external related: Dom.event => Js.Nullable.t(Dom.node) = "relatedTarget";
  [@mel.get] external pointer_type: Dom.event => string = "pointerType";
};
module Group = {
  type t = {
    skip_delay_ms: int,
    closed_at: option(float),
  };
  let init = (~skip_delay_ms, ()) => {
    skip_delay_ms: max(0, skip_delay_ms),
    closed_at: None,
  };
  let record_close = (~time_stamp, group) => {
    ...group,
    closed_at: Some(time_stamp),
  };
  let warm = (time_stamp, group) =>
    switch (group.closed_at) {
    | Some(closed) =>
      time_stamp >= closed
      && time_stamp
      -. closed < float_of_int(group.skip_delay_ms)
    | None => false
    };
};
type region =
  | Trigger
  | Content;
type phase =
  | Opening
  | Closing;
type msg =
  | Enter(region, float)
  | Leave(region, float)
  | Elapsed(int, float)
  | Window_left(float)
  | Hidden(float);
type out_msg =
  | Intent_open
  | Intent_close;
type model = {
  open_delay_ms: int,
  close_delay_ms: int,
  opened: bool,
  trigger: bool,
  content: bool,
  generation: int,
  pending: option(phase),
  last_close: option(float),
};
let init = (~open_delay_ms, ~close_delay_ms, ()) => {
  open_delay_ms: max(0, open_delay_ms),
  close_delay_ms: max(0, close_delay_ms),
  opened: false,
  trigger: false,
  content: false,
  generation: 0,
  pending: None,
  last_close: None,
};
let is_open = model => model.opened;
let last_close_time = model => model.last_close;
let invalidate = model => {
  ...model,
  generation: model.generation + 1,
  pending: None,
};
let open_now = model => (
  {
    ...invalidate(model),
    opened: true,
  },
  Cmd.none,
);
let close_now = model => (
  {
    ...invalidate(model),
    opened: false,
    trigger: false,
    content: false,
  },
  Cmd.none,
);
let transition = (~time_stamp, ~opened, model) => (
  {
    ...invalidate(model),
    opened,
    last_close: !opened && model.opened ? Some(time_stamp) : model.last_close,
  },
  Cmd.none,
  model.opened == opened ? [] : [opened ? Intent_open : Intent_close],
);
let leave = (~time_stamp, model) =>
  if (model.trigger || model.content) {
    (model, Cmd.none, []);
  } else if (!model.opened) {
    (invalidate(model), Cmd.none, []);
  } else if (model.close_delay_ms == 0) {
    transition(~time_stamp, ~opened=false, model);
  } else {
    switch (model.pending) {
    | Some(Closing) => (model, Cmd.none, [])
    | _ => (
        {
          ...invalidate(model),
          pending: Some(Closing),
        },
        Cmd.none,
        [],
      )
    };
  };
let update = (~disabled=false, ~group=?, msg, model) =>
  if (disabled) {
    let (model, cmd) = close_now(model);
    (model, cmd, []);
  } else {
    switch (msg) {
    | Enter(region, time_stamp) =>
      let already_inside =
        switch (region) {
        | Trigger => model.trigger
        | Content => model.content
        };
      let model =
        switch (region) {
        | Trigger => {
            ...model,
            trigger: true,
          }
        | Content => {
            ...model,
            content: true,
          }
        };
      if (model.opened) {
        (invalidate(model), Cmd.none, []);
      } else if (already_inside) {
        (model, Cmd.none, []);
      } else if (model.open_delay_ms == 0
                 || Option.fold(
                      ~none=false,
                      ~some=Group.warm(time_stamp),
                      group,
                    )) {
        transition(~time_stamp, ~opened=true, model);
      } else {
        switch (model.pending) {
        | Some(Opening) => (model, Cmd.none, [])
        | _ => (
            {
              ...invalidate(model),
              pending: Some(Opening),
            },
            Cmd.none,
            [],
          )
        };
      };
    | Leave(region, time_stamp) =>
      let model =
        switch (region) {
        | Trigger => {
            ...model,
            trigger: false,
          }
        | Content => {
            ...model,
            content: false,
          }
        };
      leave(~time_stamp, model);
    | Window_left(time_stamp) =>
      leave(
        ~time_stamp,
        {
          ...model,
          trigger: false,
          content: false,
        },
      )
    | Hidden(time_stamp) =>
      transition(
        ~time_stamp,
        ~opened=false,
        {
          ...model,
          trigger: false,
          content: false,
        },
      )
    | Elapsed(generation, time_stamp) when generation == model.generation =>
      switch (model.pending) {
      | Some(Opening) when model.trigger || model.content =>
        transition(~time_stamp, ~opened=true, model)
      | Some(Closing) when !model.trigger && !model.content =>
        transition(~time_stamp, ~opened=false, model)
      | _ => (model, Cmd.none, [])
      }
    | Elapsed(_, _) => (model, Cmd.none, [])
    };
  };
/* A cancellable private Task, owned by the subscription rather than a naked
   command: Cmd has no shutdown disposer. Generation checking also rejects
   callbacks already queued before cancellation. */
let delay = (~ms, ~cancel, done_: result(float, unit) => unit) => {
  let live = ref(true);
  let timer =
    B.timeout(
      () =>
        if (live^) {
          live := false;
          done_(Ok(B.now()));
        },
      ms,
    );
  cancel :=
    (
      () => {
        live := false;
        B.clear(timer);
      }
    );
};
/* Browser_events' global identities otherwise deduplicate across independent
   submodels in one parent subscription tree. Scope the public Sub description
   so every instance receives visibility changes and owns its own disposer. */
let rec scoped = (~key, sub) => {
  let scope = source_key =>
    string_of_int(String.length(key)) ++ ":" ++ key ++ source_key;
  switch (sub) {
  | Sub.None => Sub.none
  | Sub.Batch(subs) => Sub.batch(List.map(scoped(~key), subs))
  | Sub.Leaf(leaf) =>
    Sub.Leaf({
      ...leaf,
      key: scope(leaf.key),
    })
  | Sub.Filtered(leaf) =>
    Sub.Filtered({
      ...leaf,
      key: scope(leaf.key),
    })
  };
};
let subscriptions = (~key, model) => {
  let timer =
    switch (model.pending) {
    | None => Sub.none
    | Some(phase) =>
      Sub.make(
        ~home="Hover_intent.timer",
        ~key=
          string_of_int(String.length(key))
          ++ ":"
          ++ key
          ++ ":"
          ++ string_of_int(model.generation),
        ~tagger=time_stamp => Elapsed(model.generation, time_stamp),
        ~start=
          dispatch => {
            let cancel = ref(() => ());
            let task: Tea_core.Task.t(float, unit) =
              delay(
                ~ms=
                  switch (phase) {
                  | Opening => model.open_delay_ms
                  | Closing => model.close_delay_ms
                  },
                ~cancel,
              );
            task(result =>
              switch (result) {
              | Ok(time_stamp) => dispatch(time_stamp)
              | Error () => ()
              }
            );
            () => cancel^();
          },
      )
    };
  let watching =
    model.opened
    || model.trigger
    || model.content
    || Option.is_some(model.pending);
  let window =
    !watching
      ? Sub.none
      : Sub.make(
          ~home="Hover_intent.window",
          ~key,
          ~tagger=time_stamp => Window_left(time_stamp),
          ~start=
            dispatch => {
              let blur = _ => dispatch(B.now());
              let out = event =>
                if (!String.equal(B.pointer_type(event), "touch")
                    && Js.Nullable.isNullable(B.related(event))) {
                  dispatch(B.now());
                };
              B.listen(B.window, "blur", blur, false);
              B.listen(B.document, "pointerout", out, true);
              () => {
                B.unlisten(B.window, "blur", blur, false);
                B.unlisten(B.document, "pointerout", out, true);
              };
            },
        );
  Sub.batch([
    timer,
    window,
    watching
      ? Sub.filter_map(
          visible => visible ? None : Some(Hidden(B.now())),
          scoped(
            ~key,
            Tea_browser.Browser_events.on_visibility_change(x => x),
          ),
        )
      : Sub.none,
  ]);
};
let attrs = (~model, ~disabled, ~to_parent, region) => {
  let event = make =>
    D.and_then(
      pointer_type =>
        String.equal(pointer_type, "touch")
          ? D.fail("Touch does not hover")
          : D.map(make, D.field("timeStamp", D.float)),
      D.field("pointerType", D.string),
    );
  List.map(
    V.map_fact(to_parent),
    [
      Ui_attr.data_flag("open", model.opened),
      Ui_attr.data_flag("closed", !model.opened),
      Ui_attr.data_flag("disabled", disabled),
      Html.on(
        "pointerenter",
        V.Normal(event(time_stamp => Enter(region, time_stamp))),
      ),
      Html.on(
        "pointerleave",
        V.Normal(event(time_stamp => Leave(region, time_stamp))),
      ),
      Html.on(
        "pointercancel",
        V.Normal(event(time_stamp => Leave(region, time_stamp))),
      ),
    ],
  );
};
let trigger_attrs = (~model, ~disabled=false, ~to_parent, ()) =>
  attrs(~model, ~disabled, ~to_parent, Trigger);
let content_attrs = (~model, ~disabled=false, ~to_parent, ()) =>
  attrs(~model, ~disabled, ~to_parent, Content);
type parts('parent) = {
  trigger_attrs: list(Html.attr('parent)),
  content_attrs: list(Html.attr('parent)),
  is_open: bool,
};
let view = (~model, ~disabled=false, ~to_parent, ~to_view, ()) =>
  to_view({
    trigger_attrs: trigger_attrs(~model, ~disabled, ~to_parent, ()),
    content_attrs: content_attrs(~model, ~disabled, ~to_parent, ()),
    is_open: model.opened,
  });
