open Tea_vdom;
let rows = changed =>
  Vdom.keyed(
    "ul",
    [],
    List.init(1000, i =>
      (
        string_of_int(i),
        Vdom.node(
          "li",
          [],
          [Vdom.text(i == changed ? "changed" : string_of_int(i))],
        ),
      )
    ),
  );
let () = {
  let old = rows(-1);
  let fresh = rows(500);
  let tree = Naive.render(old);
  let samples =
    Array.init(
      1000,
      _ => {
        let start = Unix.gettimeofday();
        let result = Patch_sim.apply_to_tree(tree, Diff.diff(old, fresh));
        let elapsed = (Unix.gettimeofday() -. start) *. 1000.0;
        if (Naive.created(tree, result) != 0) {
          failwith("unexpected recreation");
        };
        elapsed;
      },
    );
  Array.sort(Float.compare, samples);
  Printf.printf(
    "1000 keyed rows, one changed, 1000 diff+apply samples: median %.4f ms; recreated 0 nodes\n",
    (samples[499] +. samples[500]) /. 2.0,
  );
};
