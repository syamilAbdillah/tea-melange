type document_view('msg) =
  Tea_dom.App.document_view('msg) = {
    title: string,
    body: list(Tea_vdom.Vdom.t('msg)),
  };
/* Unit-returning entry point: the plan's let () = App.beginner(...) works
   as written. Use Tea_dom.App.beginner when a handle is needed. */
let beginner = (~init, ~update, ~view, ~node, ()) =>
  ignore(Tea_dom.App.beginner(~init, ~update, ~view, ~node, ()));
let element = Tea_dom.App.element;
let document = Tea_dom.App.document;
let application = Tea_nav.Application.application;
