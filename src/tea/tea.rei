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
/** Pure seeded generators and browser generation commands. */
module Random: {
  include (module type of Tea_core.Random);
  include (module type of Tea_effects.Random);
};
/** Browser program entry points. Beginner returns unit for top-level let ().
    Element, document and application return lifecycle handles. */
module App: {
  /** A document view, rendered as the document body with an updated title. */
  type document_view('msg) =
    Tea_dom.App.document_view('msg) = {
      title: string,
      body: list(Tea_vdom.Vdom.t('msg)),
    };
  /** Pure-model convenience program. The selector must identify a mount element.
    Initialization renders synchronously; subsequent renders are frame-batched. */
  let beginner:
    (
      ~init: 'model,
      ~update: ('msg, 'model) => 'model,
      ~view: 'model => Tea_vdom.Vdom.t('msg),
      ~node: string,
      unit
    ) =>
    unit;
  /** Start an effectful program inside a CSS-selected mount element. Shutdown
    stops the runtime and detaches its managed DOM listeners. */
  let element:
    (
      ~init: 'flags => ('model, Tea_core.Cmd.t('msg)),
      ~update: ('msg, 'model) => ('model, Tea_core.Cmd.t('msg)),
      ~view: 'model => Tea_vdom.Vdom.t('msg),
      ~subscriptions: 'model => Tea_core.Sub.t('msg),
      ~node: string,
      ~flags: 'flags,
      unit
    ) =>
    Tea_core.Runtime.handle('model, 'msg);
  /** Own the document body contents and title, retaining document.body itself.
    Body children have no additional wrapper element. */
  let document:
    (
      ~init: 'flags => ('model, Tea_core.Cmd.t('msg)),
      ~update: ('msg, 'model) => ('model, Tea_core.Cmd.t('msg)),
      ~view: 'model => document_view('msg),
      ~subscriptions: 'model => Tea_core.Sub.t('msg),
      ~flags: 'flags,
      unit
    ) =>
    Tea_core.Runtime.handle('model, 'msg);
  /** Own document.body and title, intercept ordinary links, and deliver history
    changes through the same FIFO as all other messages. Init receives flags
    and the initial HTTP(S) URL. Requests do not navigate automatically: update
    chooses Navigation.push_url/load (or no command). Modified, download and
    non-_self links retain browser behavior. Shutdown removes navigation and
    click listeners as well as the DOM/runtime resources. */
  let application:
    (
      ~init: ('flags, Tea_url.Url.t) => ('model, Tea_core.Cmd.t('msg)),
      ~update: ('msg, 'model) => ('model, Tea_core.Cmd.t('msg)),
      ~view: 'model => Tea_dom.App.document_view('msg),
      ~subscriptions: 'model => Tea_core.Sub.t('msg),
      ~on_url_request: Navigation.url_request => 'msg,
      ~on_url_change: Tea_url.Url.t => 'msg,
      ~flags: 'flags,
      unit
    ) =>
    Tea_core.Runtime.handle('model, 'msg);
};
