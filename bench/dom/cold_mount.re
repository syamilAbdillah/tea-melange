/* Measure first-render latency without the listener-registration profiler.
   The separate bench_dom process verifies actual listeners and identity reuse. */
open Tea_html;
let () = {
  let rows =
    List.init(1000, i =>
      (
        string_of_int(i),
        Html.li(
          [Attr.data("uid", string_of_int(i)), Events.on_click(i)],
          [Html.text(string_of_int(i))],
        ),
      )
    );
  let view = Tea_vdom.Vdom.keyed("ul", [], rows);
  let start = Tea_dom.Browser_platform.now();
  let root = Tea_dom.Render.create(_ => (), view);
  let elapsed = Tea_dom.Browser_platform.now() -. start;
  Printf.printf("DOM: unprofiled cold mount 1000 rows %.4f ms\n%!", elapsed);
  Tea_dom.Render.dispose(root);
};
