type bucket('a) =
  | Empty
  | Cons(string, 'a, bucket('a));
type t('a) = {
  mutable buckets: array(bucket('a)),
  mutable size: int,
};

let create = n => {
  let rec pow2 = c => c >= n ? c : pow2(c * 2);
  {
    buckets: Array.make(pow2(8), Empty),
    size: 0,
  };
};
let length = t => t.size;

/* FNV-1a, truncated to 30 bits so it is identical natively and in JS. */
let hash = s => {
  let h = ref(0x811c9dc5 land 0x3fffffff);
  for (i in 0 to String.length(s) - 1) {
    h :=
      h^ lxor Char.code(String.unsafe_get(s, i)) * 0x01000193 land 0x3fffffff;
  };
  h^;
};
let index = (t, k) => hash(k) land (Array.length(t.buckets) - 1);

let rec find_in = (k, b) =>
  switch (b) {
  | Empty => None
  | Cons(k', v, rest) => k' == k ? Some(v) : find_in(k, rest)
  };
let find_opt = (t, k) =>
  find_in(k, Array.unsafe_get(t.buckets, index(t, k)));
let rec mem_in = (k, b) =>
  switch (b) {
  | Empty => false
  | Cons(k', _, rest) => k' == k || mem_in(k, rest)
  };
let mem = (t, k) => mem_in(k, Array.unsafe_get(t.buckets, index(t, k)));

let rec remove_in = (k, b) =>
  switch (b) {
  | Empty => None
  | Cons(k', v, rest) =>
    if (k' == k) {
      Some(rest);
    } else {
      switch (remove_in(k, rest)) {
      | None => None
      | Some(rest) => Some(Cons(k', v, rest))
      };
    }
  };
let remove = (t, k) => {
  let i = index(t, k);
  switch (remove_in(k, Array.unsafe_get(t.buckets, i))) {
  | None => ()
  | Some(b) =>
    Array.unsafe_set(t.buckets, i, b);
    t.size = t.size - 1;
  };
};

let resize = t => {
  let old = t.buckets;
  t.buckets = Array.make(Array.length(old) * 2, Empty);
  let rec move = b =>
    switch (b) {
    | Empty => ()
    | Cons(k, v, rest) =>
      let i = index(t, k);
      Array.unsafe_set(
        t.buckets,
        i,
        Cons(k, v, Array.unsafe_get(t.buckets, i)),
      );
      move(rest);
    };
  Array.iter(move, old);
};
let replace = (t, k, v) => {
  remove(t, k);
  let i = index(t, k);
  Array.unsafe_set(
    t.buckets,
    i,
    Cons(k, v, Array.unsafe_get(t.buckets, i)),
  );
  t.size = t.size + 1;
  if (t.size > 2 * Array.length(t.buckets)) {
    resize(t);
  };
};

let fold = (f, t, acc) => {
  let rec go = (b, acc) =>
    switch (b) {
    | Empty => acc
    | Cons(k, v, rest) => go(rest, f(k, v, acc))
    };
  Array.fold_left((acc, b) => go(b, acc), acc, t.buckets);
};
let iter = (f, t) => fold((k, v, ()) => f(k, v), t, ());
