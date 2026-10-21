external document: Dom.document = "document";
external example: string = "teaExample";
external calls: array(string) = "teaFetchCalls";
external flush: unit => unit = "teaFlushFrame";
external back: unit => Js.Promise.t(unit) = "teaBack";
[@mel.send]
external query: (Dom.document, string) => Dom.node = "querySelector";
[@mel.send]
external query_all: (Dom.document, string) => array(Dom.node) =
  "querySelectorAll";
[@mel.get] external text: Dom.node => string = "textContent";
[@mel.get_index] external get: (Dom.node, string) => 'a;
[@mel.set_index] external put: (Dom.node, string, 'a) => unit;
[@mel.new]
external event:
  (
    string,
    {
      .
      "bubbles": bool,
      "cancelable": bool,
    }
  ) =>
  Dom.event =
  "Event";
[@mel.new]
external mouse_event:
  (
    string,
    {
      .
      "bubbles": bool,
      "cancelable": bool,
    }
  ) =>
  Dom.event =
  "window.MouseEvent";
[@mel.send] external dispatch: (Dom.node, Dom.event) => bool = "dispatchEvent";
let check = (label, pass) =>
  if (!pass) {
    failwith(example ++ ": " ++ label);
  };
let node = selector => query(document, selector);
let rows = () => query_all(document, "#todos > li");
let frame = () => {
  flush();
  flush();
};
let emit = (selector, kind) => {
  let options = {
    "bubbles": true,
    "cancelable": true,
  };
  let e =
    kind == "click" ? mouse_event(kind, options) : event(kind, options);
  let accepted = dispatch(node(selector), e);
  frame();
  accepted;
};
let click = selector => ignore(emit(selector, "click"));
let input = (selector, value) => {
  put(node(selector), "value", value);
  ignore(emit(selector, "input"));
};
let counter = () => {
  check("initial mount", text(node("#app span")) == "0");
  click("#app button:last-child");
  check("increment", text(node("#app span")) == "1");
  click("#app button:first-child");
  click("#app button:first-child");
  check("decrement", text(node("#app span")) == "-1");
};
let todo = () => {
  check("empty initial model", Array.length(rows()) == 0);
  click("#seed");
  check("seed 1000", Array.length(rows()) == 1000);
  let original = rows();
  input("[data-id='500'] input[type=text]", "Edited task");
  let edited = rows();
  Array.iteri(
    (i, row) => check("edit recreates zero rows", edited[i] === row),
    original,
  );
  check(
    "edit value",
    get(node("[data-id='500'] input[type=text]"), "value") == "Edited task",
  );
  put(node("[data-id='500'] input[type=checkbox]"), "checked", true);
  ignore(emit("[data-id='500'] input[type=checkbox]", "change"));
  check(
    "toggle property",
    get(node("[data-id='500'] input[type=checkbox]"), "checked") == true,
  );
  let toggled = rows();
  Array.iteri(
    (i, row) => check("toggle recreates zero rows", toggled[i] === row),
    original,
  );
  input("#draft", "  A new task  ");
  check("submit prevents default", !emit("#add-form", "submit"));
  check("added", Array.length(rows()) == 1001);
  check("draft cleared", get(node("#draft"), "value") == "");
  check(
    "new task trimmed",
    get(node("[data-id='1000'] input[type=text]"), "value") == "A new task",
  );
  click("#filter-Completed");
  check("completed filter", Array.length(rows()) == 1);
  check("filter retains visible keyed row", rows()[0] === original[500]);
  click("#filter-Active");
  check("active filter", Array.length(rows()) == 1000);
  click("#filter-All");
  check("all filter", Array.length(rows()) == 1001);
  click("[data-id='1000'] button");
  check("delete", Array.length(rows()) == 1000);
  input("#draft", "   ");
  ignore(emit("#add-form", "submit"));
  check("blank rejected", Array.length(rows()) == 1000);
};
let routing = () => {
  frame();
  check("home", text(node("#home")) != "");
  check("fetch URL and count", calls == [|"https://tea.test/users.json"|]);
  click("a[href='/users']");
  check(
    "users loaded",
    Array.length(query_all(document, "#users li")) == 3,
  );
  click("a[href='/users/1']");
  check("detail", text(node("#user")) == "Ada Lovelace");
  Js.Promise.then_(
    _ => {
      frame();
      check(
        "history back",
        Array.length(query_all(document, "#users li")) == 3,
      );
      click("a[href='/missing']");
      check("NotFound", text(node("#not-found")) == "Page not found");
      click("a[href='/']");
      check("home link", text(node("#home")) != "");
      check("routing uses cached JSON", Array.length(calls) == 1);
      Js.Promise.resolve();
    },
    back(),
  );
};
/* Real shipped HTML must resolve imports/assets at the emission root even
   when a static SPA fallback serves it for a nested path. */
let () =
  check(
    "root asset base",
    get(node("base"), "href") == "https://tea.test/",
  );
let completed =
  switch (example) {
  | "counter" =>
    counter();
    Js.Promise.resolve();
  | "todo" =>
    todo();
    Js.Promise.resolve();
  | "fetch_routing" => routing()
  | _ => failwith("Unknown example")
  };
let completed =
  Js.Promise.then_(
    () => {
      print_endline("example smoke: " ++ example ++ " passed");
      Js.Promise.resolve();
    },
    completed,
  );
