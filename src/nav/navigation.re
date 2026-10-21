module Url = Tea_url.Url;
type url_request =
  | Internal(Url.t)
  | External(string);
let href: unit => string = [%mel.raw
  {|function() { return window.location.href; }|}
];
let current_url = () =>
  switch (Url.from_string(href())) {
  | Some(url) => url
  | None => invalid_arg("Navigation.current_url: requires HTTP(S)")
  };
let navigate: (string, string) => unit = [%mel.raw
  {|function(kind, url) { window.history[kind](null, '', url); window.dispatchEvent(new window.Event('tea:navigation')); }|}
];
let go: int => unit = [%mel.raw {|function(n) { window.history.go(n); }|}];
let assign: string => unit = [%mel.raw
  {|function(url) { window.location.assign(url); }|}
];
let reload_location: unit => unit = [%mel.raw
  {|function() { window.location.reload(); }|}
];
let push_url = url => Tea_core.Cmd.effect(_ => navigate("pushState", url));
let replace_url = url =>
  Tea_core.Cmd.effect(_ => navigate("replaceState", url));
let back = n => Tea_core.Cmd.effect(_ => go(- n));
let forward = n => Tea_core.Cmd.effect(_ => go(n));
let load = url => Tea_core.Cmd.effect(_ => assign(url));
let reload = () => Tea_core.Cmd.effect(_ => reload_location());
let listen: (unit => unit, unit) => unit = [%mel.raw
  {|function(cb) {
  window.addEventListener('popstate', cb); window.addEventListener('tea:navigation', cb);
  return function() { window.removeEventListener('popstate', cb); window.removeEventListener('tea:navigation', cb); };
}|}
];
let on_url_change = tagger =>
  Tea_core.Sub.make(~home="Navigation", ~key="url", ~tagger, ~start=dispatch =>
    listen(() => dispatch(current_url()))
  );
let intercept: (Dom.element, (bool, string) => unit, unit) => unit = [%mel.raw
  {|function(root, cb) {
  function click(e) {
    if (e.defaultPrevented || e.button !== 0 || e.ctrlKey || e.metaKey || e.shiftKey || e.altKey) return;
    var node = e.target;
    if (node && node.nodeType !== 1) node = node.parentElement;
    while (node && node !== root && !(node.tagName && node.tagName.toLowerCase() === 'a' && node.hasAttribute('href'))) node = node.parentElement;
    if (!node || !root.contains(node) || !node.tagName || node.tagName.toLowerCase() !== 'a' || !node.hasAttribute('href')) return;
    var target = node.getAttribute('target');
    if ((target && target.toLowerCase() !== '_self') || node.hasAttribute('download')) return;
    var url;
    try { url = new URL(node.href, window.location.href); } catch (_) { return; }
    e.preventDefault();
    cb(url.origin === window.location.origin && (url.protocol === 'http:' || url.protocol === 'https:'), url.href);
  }
  root.addEventListener('click', click);
  return function() { root.removeEventListener('click', click); };
}|}
];
let intercept_links = (~on_url_request, root) =>
  intercept(root, (internal, href) =>
    on_url_request(
      switch (Url.from_string(href)) {
      | Some(url) when internal => Internal(url)
      | _ => External(href)
      },
    )
  );
