external global: Js.Dict.t(Js.Json.t) = "globalThis";

external raf: (float => unit) => int = "requestAnimationFrame";

external timeout: (unit => unit, int) => int = "setTimeout";
[@mel.send] external performance_now: Js.Json.t => float = "now";
let now = () =>
  switch (Js.Dict.get(global, "performance")) {
  | Some(p) =>
    if (!Js.testAny(p) && Js.typeof(Bindings.get(p, "now")) == "function") {
      performance_now(p);
    } else {
      Js.Date.now();
    }
  | None => Js.Date.now()
  };
let request_animation_frame = f => {
  switch (Js.Dict.get(global, "requestAnimationFrame")) {
  | Some(r) when Js.typeof(r) == "function" => ignore(raf(f))
  | _ => ignore(timeout(() => f(now()), 16))
  };
};
