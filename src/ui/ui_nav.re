type direction =
  | Ltr
  | Rtl;
let enabled = (disabled, i) =>
  i >= 0 && i < Array.length(disabled) && !disabled[i];
let rec scan = (disabled, i, step) =>
  if (i < 0 || i >= Array.length(disabled)) {
    None;
  } else if (!disabled[i]) {
    Some(i);
  } else {
    scan(disabled, i + step, step);
  };
let first = disabled => scan(disabled, 0, 1);
let last = disabled => scan(disabled, Array.length(disabled) - 1, -1);
let move = (~wrap, ~forward, ~from, disabled) => {
  let n = Array.length(disabled);
  let step = forward ? 1 : (-1);
  let from = from < 0 || from >= n ? forward ? (-1) : n : from;
  let rec loop = (i, left) =>
    if (left == 0 || n == 0) {
      None;
    } else if (!wrap && (i < 0 || i >= n)) {
      enabled(disabled, from) ? Some(from) : None;
    } else {
      let i = i < 0 ? n - 1 : i >= n ? 0 : i;
      !disabled[i] ? Some(i) : loop(i + step, left - 1);
    };
  loop(from + step, n);
};
let next = (~wrap=true, ~from, disabled) =>
  move(~wrap, ~forward=true, ~from, disabled);
let prev = (~wrap=true, ~from, disabled) =>
  move(~wrap, ~forward=false, ~from, disabled);
let page = (~size, ~forward, ~from, disabled) => {
  let n = Array.length(disabled);
  if (n == 0) {
    None;
  } else {
    let from = max(0, min(n - 1, from));
    let size = max(1, min(n, size));
    let target =
      forward ? from + min(size, n - 1 - from) : from - min(size, from);
    switch (scan(disabled, target, forward ? 1 : (-1))) {
    | Some(_) as found => found
    | None => scan(disabled, target, forward ? (-1) : 1)
    };
  };
};
let repair = (~from, disabled) =>
  enabled(disabled, from) ? Some(from) : first(disabled);
let horizontal = (~dir, key) =>
  switch (key, dir) {
  | ("ArrowRight", Ltr)
  | ("ArrowLeft", Rtl) => Some(1)
  | ("ArrowLeft", Ltr)
  | ("ArrowRight", Rtl) => Some(-1)
  | _ => None
  };
