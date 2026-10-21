module R = Tea_core.Random;
let run = () => {
  let seed = R.initial_seed(42);
  let g = R.list(10, R.int(0, 999));
  let (xs, _) = R.step(g, seed);
  assert(xs == [432, 348, 59, 16, 556, 134, 840, 334, 979, 639]);
  assert(R.step(g, seed) == R.step(g, seed));
  assert(fst(R.step(R.int(-5, -5), seed)) == (-5));
  let (full, _) =
    R.step(R.list(1000, R.int(-2147483648, 2147483647)), seed);
  assert(List.exists(n => n < 0, full) && List.exists(n => n > 0, full));
  assert(R.step(g, R.initial_seed(0)) == R.step(g, R.initial_seed(1)));
  assert(fst(R.step(R.constant("x"), seed)) == "x");
  let (xs, _) = R.step(R.list(20000, R.int(0, 9)), seed);
  let counts = Array.make(10, 0);
  List.iter(x => counts[x] = counts[x] + 1, xs);
  Array.iter(n => assert(n > 1700 && n < 2300), counts);
  let (bs, _) = R.step(R.list(10000, R.bool), seed);
  let n = List.length(List.filter(x => x, bs));
  assert(n > 4700 && n < 5300);
  let (fs, _) = R.step(R.list(1000, R.float(-10., 20.)), seed);
  List.iter(x => assert(x >= (-10.) && x < 20.), fs);
  assert(
    fst(R.step(R.weighted((0., "never"), [(1., "yes")]), seed)) == "yes",
  );
  let (us, _) = R.step(R.list(100, R.uniform(1, [2, 3])), seed);
  assert(List.for_all(n => n >= 1 && n <= 3, us));
  assert(
    fst(
      R.step(R.map2((a, b) => a + b, R.constant(2), R.constant(3)), seed),
    )
    == 5,
  );
  assert(
    fst(R.step(R.and_then(n => R.constant(n + 1), R.constant(2)), seed))
    == 3,
  );
  assert(
    fst(R.step(R.pair(R.constant(1), R.constant(2)), seed)) == (1, 2),
  );
  print_endline("random: portable sequence and distribution passed");
};
