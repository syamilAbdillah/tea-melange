/** Own document.body and title, intercept ordinary links, and deliver history
    changes through the same FIFO as all other messages. Init receives flags
    and the initial HTTP(S) URL. Requests do not navigate automatically: update
    chooses Navigation.push_url/load (or no command). Modified, download and
    non-_self links retain browser behavior. Shutdown removes navigation and
    click listeners as well as the DOM/runtime resources. */
let application:
  (
    ~init: ('flags, Tea_url.Url.t) => ('model, Tea_core.Cmd.t('msg)),
    ~update: ('model, 'msg) => ('model, Tea_core.Cmd.t('msg)),
    ~view: 'model => Tea_dom.App.document_view('msg),
    ~subscriptions: 'model => Tea_core.Sub.t('msg),
    ~on_url_request: Navigation.url_request => 'msg,
    ~on_url_change: Tea_url.Url.t => 'msg,
    ~flags: 'flags,
    unit
  ) =>
  Tea_core.Runtime.handle('model, 'msg);
