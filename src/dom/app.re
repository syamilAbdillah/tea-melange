open Tea_core;
module Runtime = Runtime.Make(Browser_platform);
type document_view('msg) = {
  title: string,
  body: list(Tea_vdom.Vdom.t('msg)),
};
module Internal = {
  let start = (~program, ~flags, ~mount, ~render_root, ~mount_root=false, ()) => {
    let previous = ref(None);
    let root = ref(None);
    let handle =
      Runtime.start(
        ~program,
        ~flags,
        ~render=
          (view, dispatch) => {
            let next = render_root(view);
            switch (previous^, root^) {
            | (Some(old), Some(dom)) =>
              root := Some(Patch.apply(dom, Tea_vdom.Diff.diff(old, next)))
            | _ =>
              let dom =
                if (mount_root) {
                  Backend.create_in(dispatch, next, mount);
                } else {
                  let dom = Render.create(dispatch, next);
                  Bindings.set_text(mount, "");
                  Bindings.append(mount, dom);
                  dom;
                };
              root := Some(dom);
            };
            previous := Some(next);
          },
        (),
      );
    {
      ...handle,
      shutdown: () => {
        handle.shutdown();
        switch (root^) {
        | Some(dom) => Render.dispose(dom)
        | None => ()
        };
      },
    };
  };
};
let find_mount = selector =>
  switch (Js.Nullable.toOption(Bindings.query(Bindings.document, selector))) {
  | Some(node) => node
  | None => invalid_arg("App: no mount matches " ++ selector)
  };
let element = (~init, ~update, ~view, ~subscriptions, ~node, ~flags, ()) => {
  let program: Program.t(_, _, _, _) = {
    init,
    update,
    view,
    subscriptions,
  };
  Internal.start(
    ~program,
    ~flags,
    ~mount=find_mount(node),
    ~render_root=x => x,
    (),
  );
};
let beginner = (~init, ~update, ~view, ~node, ()) =>
  element(
    ~init=() => (init, Cmd.none),
    ~update=(msg, model) => (update(msg, model), Cmd.none),
    ~view,
    ~subscriptions=_ => Sub.none,
    ~node,
    ~flags=(),
    (),
  );
let document = (~init, ~update, ~view, ~subscriptions, ~flags, ()) => {
  let program: Program.t(_, _, _, _) = {
    init,
    update,
    view,
    subscriptions,
  };
  Internal.start(
    ~program,
    ~flags,
    ~mount=Bindings.body(Bindings.document),
    ~mount_root=true,
    ~render_root=
      view => {
        Bindings.title(Bindings.document, view.title);
        Tea_vdom.Vdom.node("body", [], view.body);
      },
    (),
  );
};
