module App = Tea_dom.App;
module Cmd = Tea_core.Cmd;

let body: unit => Dom.element = [%mel.raw
  {|function() { return document.body; }|}
];
external as_node: Dom.element => Dom.node = "%identity";
let set_title: string => unit = [%mel.raw
  {|function(title) { document.title = title; }|}
];

let application =
    (
      ~init,
      ~update,
      ~view,
      ~subscriptions,
      ~on_url_request,
      ~on_url_change,
      ~flags,
      (),
    ) => {
  let mount = body();
  let stop_navigation = ref(() => ());
  let stop_links = ref(() => ());
  let initial_url = Navigation.current_url();
  /* Install before user init commands so even an initial push/replace enters
     the runtime's initialization FIFO. The handle owns both disposers. */
  let listen =
    Cmd.effect(dispatch => {
      switch (Navigation.on_url_change(on_url_change)) {
      | Tea_core.Sub.Leaf(leaf) =>
        stop_navigation := leaf.start(raw => dispatch(leaf.tagger(raw)))
      | _ => assert(false)
      };
      stop_links :=
        Navigation.intercept_links(
          ~on_url_request=request => dispatch(on_url_request(request)),
          mount,
        );
    });
  let cleanup = () => {
    stop_links^();
    stop_navigation^();
  };
  let program: Tea_core.Program.t(_, _, _, _) = {
    init: flags => {
      let (model, cmd) = init(flags, initial_url);
      (model, Cmd.batch([listen, cmd]));
    },
    update,
    view,
    subscriptions,
  };
  let handle =
    try(
      App.Internal.start(
        ~program,
        ~flags,
        ~mount=as_node(mount),
        ~mount_root=true,
        ~render_root=
          (view: App.document_view(_)) => {
            set_title(view.title);
            Tea_vdom.Vdom.node("body", [], view.body);
          },
        (),
      )
    ) {
    | exn =>
      cleanup();
      raise(exn);
    };
  let alive = ref(true);
  {
    ...handle,
    shutdown: () =>
      if (alive^) {
        alive := false;
        cleanup();
        handle.shutdown();
      },
  };
};
