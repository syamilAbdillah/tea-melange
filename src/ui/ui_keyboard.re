module D = Tea_core.Decode;
type event = {
  key: string,
  shift: bool,
  ctrl: bool,
  alt: bool,
  meta: bool,
  composing: bool,
  time_stamp: float,
  repeat: bool,
};
let bool = name => D.one_of([D.field(name, D.bool), D.succeed(false)]);
let decoder =
  D.map3(
    ((key, time_stamp), (shift, ctrl, alt), (meta, composing, repeat)) =>
      {
        key,
        time_stamp,
        shift,
        ctrl,
        alt,
        meta,
        composing,
        repeat,
      },
    D.map2(
      (key, time_stamp) => (key, time_stamp),
      D.field("key", D.string),
      D.field("timeStamp", D.float),
    ),
    D.map3(
      (shift, ctrl, alt) => (shift, ctrl, alt),
      bool("shiftKey"),
      bool("ctrlKey"),
      bool("altKey"),
    ),
    D.map3(
      (meta, composing, repeat) => (meta, composing, repeat),
      bool("metaKey"),
      D.map2(
        (composing, code) => composing || code == Some(229),
        bool("isComposing"),
        D.optional(D.field("keyCode", D.int)),
      ),
      bool("repeat"),
    ),
  );
let available = event =>
  !event.composing
  && !event.ctrl
  && !event.alt
  && !event.meta
  && !String.equal(event.key, "Tab");
let custom = handle =>
  D.and_then(
    event =>
      if (!available(event)) {
        D.fail("UI keyboard: protected key");
      } else {
        switch (handle(event)) {
        | None => D.fail("UI keyboard: unhandled key")
        | Some((message, prevent_default)) =>
          D.succeed({
            Tea_vdom.Vdom.message,
            prevent_default,
            stop_propagation: false,
          })
        };
      },
    decoder,
  );
let on_key_down = handle =>
  Tea_html.Events.on(
    "keydown",
    Tea_vdom.Vdom.Custom_optional(
      custom(event =>
        Option.map(message => (Some(message), true), handle(event))
      ),
    ),
  );
