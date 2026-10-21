/** A document view, rendered as the document body with an updated title. */
type document_view('msg) = {
  title: string,
  body: list(Tea_vdom.Vdom.t('msg)),
};
/** Pure-model convenience program. The selector must identify a mount element.
    Initialization renders synchronously; subsequent renders are frame-batched. */
let beginner:
  (
    ~init: 'model,
    ~update: ('model, 'msg) => 'model,
    ~view: 'model => Tea_vdom.Vdom.t('msg),
    ~node: string,
    unit
  ) =>
  Tea_core.Runtime.handle('model, 'msg);
/** Start an effectful program inside a CSS-selected mount element. Shutdown
    stops the runtime and detaches its managed DOM listeners. */
let element:
  (
    ~init: 'flags => ('model, Tea_core.Cmd.t('msg)),
    ~update: ('model, 'msg) => ('model, Tea_core.Cmd.t('msg)),
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
    ~update: ('model, 'msg) => ('model, Tea_core.Cmd.t('msg)),
    ~view: 'model => document_view('msg),
    ~subscriptions: 'model => Tea_core.Sub.t('msg),
    ~flags: 'flags,
    unit
  ) =>
  Tea_core.Runtime.handle('model, 'msg);
/** Navigation integration hook: application can reuse this lifecycle without
    introducing a tea.nav dependency into tea.dom. */
module Internal: {
  /** Adapt any program view to a virtual root. By default replace mount contents;
      mount_root instead reuses mount as the root element (used by document).
      In that mode render_root must return an element matching the mount tag.
      The adapter also provides a place for title and navigation bookkeeping. */
  let start:
    (
      ~program: Tea_core.Program.t('flags, 'model, 'msg, 'view),
      ~flags: 'flags,
      ~mount: Dom.node,
      ~render_root: 'view => Tea_vdom.Vdom.t('msg),
      ~mount_root: bool=?,
      unit
    ) =>
    Tea_core.Runtime.handle('model, 'msg);
};
