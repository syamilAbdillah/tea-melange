open Tea_dom;
open Tea_vdom;
open Tea_html;
/* Independent cold mount, followed by 1000 alternating updates. */
let () = {
  let row = i => (
    string_of_int(i),
    Html.li([Events.on_click(i)], [Html.text(string_of_int(i))]),
  );
  let rows = Array.init(1000, row);
  let old = Vdom.keyed("ul", [], Array.to_list(rows));
  let changed = Array.copy(rows);
  changed[500] = (
    "500",
    Html.li([Events.on_click(500)], [Html.text("changed")]),
  );
  let next = Vdom.keyed("ul", [], Array.to_list(changed));
  let start = Browser_platform.now();
  let root = Render.create(_ => (), old);
  let mount_ms = Browser_platform.now() -. start;
  let samples =
    Array.init(
      1000,
      i => {
        let (a, b) =
          if (i mod 2 == 0) {
            (old, next);
          } else {
            (next, old);
          };
        let start = Browser_platform.now();
        ignore(Patch.apply(root, Diff.diff(a, b)));
        Browser_platform.now() -. start;
      },
    );
  Array.sort(Float.compare, samples);
  Printf.printf(
    "jsdom cold mount: %.4f ms; diff+patch median (1000 samples): %.4f ms\n%!",
    mount_ms,
    samples[500],
  );
  Render.dispose(root);
};
