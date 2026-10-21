module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Task = Tea_core.Task;
module Program = Tea_core.Program;
module Runtime = Tea_core.Runtime;
module Decode = Tea_core.Decode;
module Vdom = Tea_vdom.Vdom;
module Html = Tea_html.Html;
module Svg = Tea_html.Svg;
module Attr = Tea_html.Attr;
module Attributes = Tea_html.Attributes;
module Events = Tea_html.Events;
module Keyed = Tea_html.Keyed;
module Http = Tea_http.Http;
module Url = Tea_url.Url;
module Navigation = Tea_nav.Navigation;
module Time = Tea_effects.Time;
module Animation_frame = Tea_effects.Animation_frame;
module AnimationFrame = Tea_effects.Animation_frame;
module Random = {
  include Tea_core.Random;
  include Tea_effects.Random;
};
module App = {
  type document_view('msg) =
    Tea_dom.App.document_view('msg) = {
      title: string,
      body: list(Tea_vdom.Vdom.t('msg)),
    };
  /* Unit-returning entry point: the plan's let () = App.beginner(...) can
     be pasted verbatim. Use Tea_dom.App.beginner when a handle is needed. */
  let beginner = (~init, ~update, ~view, ~node, ()) =>
    ignore(Tea_dom.App.beginner(~init, ~update, ~view, ~node, ()));
  let element = Tea_dom.App.element;
  let document = Tea_dom.App.document;
  let application = Tea_nav.Application.application;
};
