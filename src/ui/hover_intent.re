module Html = Tea_html.Html;
module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module D = Tea_core.Decode;
module V = Tea_vdom.Vdom;
module Group = {
  type t = {
    skip_delay_ms: int,
    closed_at: option(float),
    open_keys: list(string),
  };
  let init = (~skip_delay_ms, ()) => {
    skip_delay_ms: max(0, skip_delay_ms),
    closed_at: None,
    open_keys: [],
  };
  let record_open = (~key, group) =>
    List.mem(key, group.open_keys)
      ? group
      : {
        ...group,
        open_keys: [key, ...group.open_keys],
      };
  let record_close = (~key=?, ~time_stamp, group) => {
    ...group,
    closed_at: Some(time_stamp),
    open_keys:
      switch (key) {
      | None => group.open_keys
      | Some(key) =>
        List.filter(open_ => !String.equal(open_, key), group.open_keys)
      },
  };
  let forget = (~key, group) => {
    ...group,
    open_keys:
      List.filter(open_ => !String.equal(open_, key), group.open_keys),
  };
  let open_keys = group => group.open_keys;
  let warm = (time_stamp, group) =>
    group.skip_delay_ms > 0
    && (
      group.open_keys != []
      || (
        switch (group.closed_at) {
        | Some(closed) =>
          /* Compare deadlines, not a subtraction that can round an exact
             fractional-millisecond boundary back inside the warm window. */
          time_stamp >= closed
          && time_stamp < closed
          +. float_of_int(group.skip_delay_ms)
        | None => false
        }
      )
    );
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
    Ui_dom.timeout(
      () =>
        if (live^) {
          live := false;
          done_(Ok(Ui_dom.now()));
        },
      ms,
    );
  cancel :=
    (
      () => {
        live := false;
        Ui_dom.clear_timeout(timer);
      }
    );
};
let subscriptions = (~key, model) => {
  let timer =
    switch (model.pending) {
    | None => Sub.none
    | Some(phase) =>
      Sub.make(
        ~home="Hover_intent.timer",
        ~key=Ui_sub.prefix(key) ++ ":" ++ string_of_int(model.generation),
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
              let blur = _ => dispatch(Ui_dom.now());
              let out = event =>
                if (!String.equal(Ui_dom.pointer_type(event), "touch")
                    && Option.is_none(Ui_dom.related(event))) {
                  dispatch(Ui_dom.now());
                };
              let off_blur = Ui_dom.on(Ui_dom.window, "blur", blur);
              let off_out =
                Ui_dom.on(~capture=true, Ui_dom.document, "pointerout", out);
              () => {
                off_blur();
                off_out();
              };
            },
        );
  Sub.batch([
    timer,
    window,
    watching
      ? Sub.filter_map(
          visible => visible ? None : Some(Hidden(Ui_dom.now())),
          Ui_sub.scoped(
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
