let () = Url_cases.Url_test.run();
module N = Tea_nav.Navigation;
module C = Tea_core;
let setup: unit => Dom.element = [%mel.raw
  {|function() {
  var w = new EventTarget(); w.Event = Event;
  w.location = new URL('https://example.com/start');
  w.history = { pushState: function(_,__,url) { w.location = new URL(url, w.location); }, replaceState: function(_,__,url) { w.location = new URL(url, w.location); }, go: function(n) { w.lastGo = n; } };
  globalThis.window = w;
  var root = new EventTarget(); root.contains = function(node) { return node === root || node.parentElement === root; };
  return root;
}|}
];
let click: (Dom.element, string, string) => bool = [%mel.raw
  {|function(root, href, kind) {
  var attrs = {href:href};
  if (kind === 'blank') attrs.target = '_blank';
  if (kind === 'download') attrs.download = '';
  var a = {nodeType:1, tagName:'A', href:href, parentElement:root, hasAttribute:function(k) { return k in attrs; }, getAttribute:function(k) { return attrs[k] || null; }};
  var e = new Event('click', {cancelable:true});
  Object.defineProperty(e, 'target', {value:a});
  e.button = kind === 'right' ? 2 : 0;
  e.ctrlKey = kind === 'ctrl'; e.metaKey = kind === 'meta'; e.shiftKey = kind === 'shift'; e.altKey = kind === 'alt';
  root.dispatchEvent(e); return e.defaultPrevented;
}|}
];
let pop: unit => unit = [%mel.raw
  {|function() { window.dispatchEvent(new Event('popstate')); }|}
];
let run = cmd =>
  switch (cmd) {
  | C.Cmd.Effect(f) => f(_ => ())
  | _ => assert(false)
  };
let () = {
  let root = setup();
  let count = ref(0);
  let sub = N.on_url_change(url => url);
  let stop =
    switch (sub) {
    | C.Sub.Leaf(leaf) =>
      leaf.start(raw => {
        let url = leaf.tagger(raw);
        assert(url.host == "example.com");
        count := count^ + 1;
      })
    | _ => assert(false)
    };
  run(N.push_url("/next"));
  assert(count^ == 1);
  assert(N.current_url().path == "/next");
  run(N.replace_url("/last"));
  assert(count^ == 2);
  pop();
  assert(count^ == 3);
  stop();
  stop();
  pop();
  assert(count^ == 3);
  let requests = ref([]);
  let dispose =
    N.intercept_links(
      ~on_url_request=req => requests := [req, ...requests^],
      root,
    );
  assert(click(root, "/internal", "normal"));
  assert(
    switch (List.hd(requests^)) {
    | N.Internal(url) => url.path == "/internal"
    | _ => false
    },
  );
  assert(click(root, "https://other.com/", "normal"));
  assert(
    switch (List.hd(requests^)) {
    | N.External(_) => true
    | _ => false
    },
  );
  List.iter(
    kind => assert(!click(root, "/ignore", kind)),
    ["blank", "download", "right", "ctrl", "meta", "shift", "alt"],
  );
  dispose();
  dispose();
  assert(!click(root, "/ignored", "normal"));
  assert(List.length(requests^) == 2);
  print_endline(
    "navigation: history notifications, click classification and cleanup passed",
  );
};
