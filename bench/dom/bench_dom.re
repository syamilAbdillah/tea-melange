open Tea_dom;
open Tea_vdom;
open Tea_html;
[@mel.get] external children: Dom.node => array(Dom.node) = "childNodes";
[@mel.send] external attribute: (Dom.node, string) => string = "getAttribute";
external listener_count: Dom.node => int = "teaListenerCount";
external flush: unit => unit = "teaFlushFrame";
external requests: unit => int = "teaFrameRequests";
let check = (message, condition) =>
  if (!condition) {
    failwith(message);
  };
let median = samples => {
  Array.sort(Float.compare, samples);
  (samples[499] +. samples[500]) /. 2.;
};
let () = {
  let row = (i, label) => (
    string_of_int(i),
    Html.li(
      [Attr.data("uid", string_of_int(i)), Events.on_click(i)],
      [Html.text(label)],
    ),
  );
  let rows = Array.init(1000, i => row(i, string_of_int(i)));
  let old = Vdom.keyed("ul", [], Array.to_list(rows));
  let changed = Array.copy(rows);
  changed[500] = row(500, "changed");
  let next = Vdom.keyed("ul", [], Array.to_list(changed));
  /* First framework render in this fresh process; no warmup. */
  let start = Browser_platform.now();
  let root = Render.create(_ => (), old);
  let mount_ms = Browser_platform.now() -. start;
  let identities = Array.init(1000, i => children(root)[i]);
  let texts = Array.map(n => children(n)[0], identities);
  let counts = Array.map(listener_count, identities);
  check(
    "one actual click registration per row",
    Array.for_all(n => n == 1, counts),
  );
  let recreated = ref(0);
  let max_listener_growth = ref(0);
  let samples =
    Array.init(
      1000,
      i => {
        let (a, b) = i mod 2 == 0 ? (old, next) : (next, old);
        let start = Browser_platform.now();
        let result = Patch.apply(root, Diff.diff(a, b));
        let elapsed = Browser_platform.now() -. start;
        /* Every identity/count check is outside the timed interval. */
        check("root retained", result === root);
        let current = children(root);
        Array.iteri(
          (j, n) => {
            if (current[j] !== n) {
              incr(recreated);
            };
            if (children(current[j])[0] !== texts[j]) {
              incr(recreated);
            };
            check(
              "data-uid retained",
              attribute(current[j], "data-uid") == string_of_int(j),
            );
            max_listener_growth :=
              max(
                max_listener_growth^,
                listener_count(current[j]) - counts[j],
              );
          },
          identities,
        );
        elapsed;
      },
    );
  let after =
    Array.fold_left((sum, n) => sum + listener_count(n), 0, identities);
  check("zero recreated row/text observations", recreated^ == 0);
  check(
    "zero per-node listener growth across all patches",
    max_listener_growth^ == 0 && after == 1000,
  );
  Printf.printf(
    "DOM: cold mount 1000 rows %.4f ms; diff+patch median %.4f ms (1000 alternating samples)\n%!",
    mount_ms,
    median(samples),
  );
  Printf.printf(
    "DOM: recreated row/text nodes %d (checked after every patch); actual listeners 1000 -> %d; maximum per-node growth %d\n%!",
    recreated^,
    after,
    max_listener_growth^,
  );
  Render.dispose(root);
  check(
    "dispose removes actual listeners",
    Array.for_all(n => listener_count(n) == 0, identities),
  );
  let app =
    App.beginner(
      ~init=0,
      ~update=(model, msg) => model + msg,
      ~view=
        model =>
          Html.button(
            [Events.on_click(1)],
            [Html.text(string_of_int(model))],
          ),
      ~node="#app",
      (),
    );
  let initial_requests = requests();
  let initial_patches = Debug.patch_count();
  for (_i in 1 to 100) {
    app.dispatch(1);
  };
  let tick_requests = requests() - initial_requests;
  check("no synchronous patch", Debug.patch_count() == initial_patches);
  flush();
  let draw_patches = Debug.patch_count() - initial_patches;
  let total_requests = requests() - initial_requests;
  flush();
  let total_patches = Debug.patch_count() - initial_patches;
  check("batch final model", app.model() == 100);
  check(
    "batch one requested draw plus one spare",
    tick_requests == 1 && total_requests == 2,
  );
  check(
    "batch exactly one patch, spare does not patch",
    draw_patches == 1 && total_patches == 1,
  );
  Printf.printf(
    "DOM: 100 messages/tick: %d initial rAF, %d total rAF including spare, %d patches after both frames\n%!",
    tick_requests,
    total_requests,
    total_patches,
  );
  app.shutdown();
  check("all managed listeners disposed", Debug.active_listeners() == 0);
  print_endline(
    "DOM: all identity/listener/batching gates passed; active managed listeners after shutdown 0",
  );
};
