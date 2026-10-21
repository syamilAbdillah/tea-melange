type seed = Int32.t;
type generator('a) = seed => ('a, seed);
let initial_seed = n => {
  let n = Int32.of_int(n);
  n == 0l ? 1l : n;
};
let step = (g, s) => g(s);
let next = s => {
  let s = Int32.logxor(s, Int32.shift_left(s, 13));
  let s = Int32.logxor(s, Int32.shift_right_logical(s, 17));
  Int32.logxor(s, Int32.shift_left(s, 5));
};
let unsigned = s => {
  let n = Int32.to_float(s);
  n < 0. ? n +. 4294967296. : n;
};
let unit_float = s => {
  let s = next(s);
  (unsigned(s) /. 4294967296., s);
};
let constant = (x, s) => (x, s);
let map = (f, g, s) => {
  let (x, s) = g(s);
  (f(x), s);
};
let and_then = (f, g, s) => {
  let (x, s) = g(s);
  f(x, s);
};
let map2 = (f, a, b) => and_then(x => map(y => f(x, y), b), a);
let pair = (a, b) => map2((a, b) => (a, b), a, b);
let float = (lo, hi) => {
  if (!Float.is_finite(lo) || !Float.is_finite(hi) || lo > hi) {
    invalid_arg("Random.float");
  };
  map(x => lo *. (1. -. x) +. hi *. x, unit_float);
};
let int = (lo, hi) => {
  if (lo > hi
      || float_of_int(lo) < (-2147483648.)
      || float_of_int(hi) > 2147483647.) {
    invalid_arg("Random.int");
  };
  let width = float_of_int(hi) -. float_of_int(lo) +. 1.;
  let limit = floor(4294967296. /. width) *. width;
  let rec draw = s => {
    let s = next(s);
    let n = unsigned(s);
    n >= limit
      ? draw(s)
      : (int_of_float(float_of_int(lo) +. mod_float(n, width)), s);
  };
  draw;
};
let bool = map(x => x < 0.5, unit_float);
let uniform = (first, rest) => {
  let xs = Array.of_list([first, ...rest]);
  map(i => xs[i], int(0, Array.length(xs) - 1));
};
let weighted = (first, rest) => {
  let xs = [first, ...rest];
  let total =
    List.fold_left(
      (a, (w, _)) => {
        if (w < 0. || !Float.is_finite(w)) {
          invalid_arg("Random.weighted");
        };
        a +. w;
      },
      0.,
      xs,
    );
  if (total <= 0. || !Float.is_finite(total)) {
    invalid_arg("Random.weighted");
  };
  let fallback =
    List.fold_left((last, (w, v)) => w > 0. ? v : last, snd(first), xs);
  map(
    x => {
      let rec pick = (n, xs) =>
        switch (xs) {
        | [] => fallback
        | [(w, v), ...rest] => n < w ? v : pick(n -. w, rest)
        };
      pick(x *. total, xs);
    },
    unit_float,
  );
};
let list = (n, g) => {
  if (n < 0) {
    invalid_arg("Random.list");
  };
  s => {
    let rec loop = (n, s, acc) =>
      n == 0
        ? (List.rev(acc), s)
        : {
          let (x, s) = g(s);
          loop(n - 1, s, [x, ...acc]);
        };
    loop(n, s, []);
  };
};
