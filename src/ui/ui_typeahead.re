type t = {
  buffer: string,
  last: option(float),
};
let empty = {
  buffer: "",
  last: None,
};
let buffer = state => state.buffer;
let last_time_stamp = state => state.last;
/* Labels and keys are JS (UTF-16) strings. Fold case, then decompose (NFD)
   and drop combining marks, so precomposed and decomposed accents match.
   Letters with no canonical decomposition get explicit folds. */
[@mel.send]
external to_nfd: (string, [@mel.as "NFD"] _) => string = "normalize";
[@mel.send] external to_lower: string => string = "toLowerCase";
let folds = [
  ([%re "/[\\u0300-\\u036f]/g"], ""),
  ([%re "/\\u00e6/g"], "ae"),
  ([%re "/\\u0153/g"], "oe"),
  ([%re "/\\u00f8/g"], "o"),
  ([%re "/\\u00df/g"], "ss"),
  ([%re "/\\u00fe/g"], "th"),
  ([%re "/[\\u00f0\\u0111]/g"], "d"),
  ([%re "/\\u0142/g"], "l"),
  ([%re "/\\u0131/g"], "i"),
];
let normalize = value =>
  List.fold_left(
    (value, (pattern, replacement)) =>
      Js.String.replaceByRe(~regexp=pattern, ~replacement, value),
    to_nfd(to_lower(value)),
    folds,
  );
/* One Unicode scalar: a single UTF-16 unit or a surrogate pair. */
let character = key =>
  switch (String.length(key)) {
  | 1 => true
  | 2 =>
    let high = Char.code(key.[0]);
    high >= 0xd800 && high <= 0xdbff;
  | _ => false
  };
let feed =
    (~timeout=500., ~time_stamp, ~key, ~from, ~labels, ~disabled, state) =>
  if (!character(key) || String.equal(normalize(key), "")) {
    (
      /* A lone combining mark folds to nothing and cannot extend a search. */
      state,
      None,
    );
  } else {
    let key = normalize(key);
    let fresh =
      switch (state.last) {
      | None => true
      | Some(last) => time_stamp < last || time_stamp -. last >= timeout
      };
    let cycle = fresh || String.equal(state.buffer, key);
    let buffer = cycle ? key : state.buffer ++ key;
    let n = Array.length(labels);
    let find = (prefix, start) => {
      let start = start < 0 || start >= n ? 0 : start;
      let rec loop = (offset, left) =>
        if (left == 0) {
          None;
        } else {
          let i = offset >= n ? 0 : offset;
          if (i < Array.length(disabled)
              && !disabled[i]
              && String.starts_with(~prefix, normalize(labels[i]))) {
            Some(i);
          } else {
            loop(i + 1, left - 1);
          };
        };
      loop(start, n);
    };
    let start = cycle ? from + 1 : from;
    switch (find(buffer, start)) {
    | Some(index) => (
        {
          buffer,
          last: Some(time_stamp),
        },
        Some(index),
      )
    | None =>
      let matched = find(key, from + 1);
      (
        {
          buffer: key,
          last: Some(time_stamp),
        },
        matched,
      );
    };
  };
