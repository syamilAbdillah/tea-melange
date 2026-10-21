external document: Dom.document = "document";
external window: Dom.window = "window";
external global: Js.Json.t = "globalThis";
[@mel.get_index] external get: ('a, string) => 'b;
[@mel.set_index] external put: ('a, string, 'b) => unit;
[@mel.send]
external query: (Dom.document, string) => Dom.node = "querySelector";
[@mel.send]
external query_all: (Dom.document, string) => array(Dom.node) =
  "querySelectorAll";
[@mel.send]
external attribute: (Dom.node, string) => Js.Nullable.t(string) =
  "getAttribute";
[@mel.send]
external set_attribute: (Dom.node, string, string) => unit = "setAttribute";
[@mel.send]
external remove_attribute: (Dom.node, string) => unit = "removeAttribute";
[@mel.send] external focus: Dom.node => unit = "focus";
[@mel.send] external remove: Dom.node => unit = "remove";
[@mel.get] external active: Dom.document => Dom.node = "activeElement";
[@mel.send] external dispatch: ('a, Dom.event) => bool = "dispatchEvent";
[@mel.new] external event: (string, Js.t({..})) => Dom.event = "Event";
[@mel.new]
external keyboard_event: (string, Js.t({..})) => Dom.event = "KeyboardEvent";
[@mel.new]
external pointer_event: (string, Js.t({..})) => Dom.event = "PointerEvent";
[@mel.new]
external focus_event: (string, Js.t({..})) => Dom.event = "FocusEvent";
external flush: unit => unit = "teaFlushFrame";
external listener_count: unit => int = "teaListenerCount";
external listener_count_for: (string, string, bool) => int =
  "teaListenerCountFor";
external timeout: (unit => unit, int) => int = "setTimeout";
let assertions = ref(0);
let check = (label, condition) => {
  incr(assertions);
  if (!condition) {
    failwith(label);
  };
};
let node = selector => query(document, selector);
let attr = (selector, name) =>
  Js.Nullable.toOption(attribute(node(selector), name));
let has_focus = selector => active(document) === node(selector);
let frame = () => {
  flush();
  flush();
};
let emit = (selector, name) => {
  let e =
    event(
      name,
      {
        "bubbles": true,
        "cancelable": true,
      },
    );
  let accepted = dispatch(node(selector), e);
  frame();
  accepted;
};
let click = selector => ignore(emit(selector, "click"));
let pointer = selector => {
  let e =
    pointer_event(
      "pointerdown",
      {
        "bubbles": true,
        "cancelable": true,
        "pointerType": "mouse",
      },
    );
  ignore(dispatch(node(selector), e));
};
let key =
    (
      ~shift=false,
      ~ctrl=false,
      ~alt=false,
      ~meta=false,
      ~composing=false,
      ~repeat=false,
      selector,
      name,
      key,
    ) => {
  let e =
    keyboard_event(
      name,
      {
        "key": key,
        "bubbles": true,
        "cancelable": true,
        "shiftKey": shift,
        "ctrlKey": ctrl,
        "altKey": alt,
        "metaKey": meta,
        "isComposing": composing,
        "repeat": repeat,
      },
    );
  let accepted = dispatch(node(selector), e);
  accepted;
};
let tick = () =>
  Js.Promise.make((~resolve, ~reject as _) =>
    ignore(timeout(() => resolve(. true), 0))
  );
let after_tick = f =>
  Js.Promise.then_(
    _ => {
      f();
      Js.Promise.resolve();
    },
    tick(),
  );
let run_task = task => {
  let result = ref(None);
  task(value => result := Some(value));
  check("tasks defer until render", result^ == None);
  flush();
  result^;
};
let report = label =>
  print_endline(
    label ++ ": " ++ string_of_int(assertions^) ++ " assertions passed",
  );
