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
let folds = [
  ("À", "a"),
  ("Á", "a"),
  ("Â", "a"),
  ("Ã", "a"),
  ("Ä", "a"),
  ("Å", "a"),
  ("à", "a"),
  ("á", "a"),
  ("â", "a"),
  ("ã", "a"),
  ("ä", "a"),
  ("å", "a"),
  ("Æ", "ae"),
  ("æ", "ae"),
  ("Ç", "c"),
  ("ç", "c"),
  ("È", "e"),
  ("É", "e"),
  ("Ê", "e"),
  ("Ë", "e"),
  ("è", "e"),
  ("é", "e"),
  ("ê", "e"),
  ("ë", "e"),
  ("Ì", "i"),
  ("Í", "i"),
  ("Î", "i"),
  ("Ï", "i"),
  ("ì", "i"),
  ("í", "i"),
  ("î", "i"),
  ("ï", "i"),
  ("Ð", "d"),
  ("ð", "d"),
  ("Ñ", "n"),
  ("ñ", "n"),
  ("Ò", "o"),
  ("Ó", "o"),
  ("Ô", "o"),
  ("Õ", "o"),
  ("Ö", "o"),
  ("Ø", "o"),
  ("ò", "o"),
  ("ó", "o"),
  ("ô", "o"),
  ("õ", "o"),
  ("ö", "o"),
  ("ø", "o"),
  ("Ù", "u"),
  ("Ú", "u"),
  ("Û", "u"),
  ("Ü", "u"),
  ("ù", "u"),
  ("ú", "u"),
  ("û", "u"),
  ("ü", "u"),
  ("Ý", "y"),
  ("ý", "y"),
  ("ÿ", "y"),
  ("Þ", "th"),
  ("þ", "th"),
  ("ß", "ss"),
];
let normalize = value => {
  let out = Buffer.create(String.length(value));
  let rec loop = i =>
    if (i < String.length(value)) {
      switch (
        List.find_opt(
          ((letter, _)) =>
            i
            + String.length(letter) <= String.length(value)
            && String.equal(
                 String.sub(value, i, String.length(letter)),
                 letter,
               ),
          folds,
        )
      ) {
      | Some((letter, folded)) =>
        Buffer.add_string(out, folded);
        loop(i + String.length(letter));
      | None =>
        Buffer.add_char(out, Char.lowercase_ascii(value.[i]));
        loop(i + 1);
      };
    };
  loop(0);
  Buffer.contents(out);
};
let character = key => {
  let n = String.length(key);
  if (n == 0) {
    false;
  } else {
    let first = Char.code(key.[0]);
    n == 1
    || n == 2
    && first >= 0xd800
    && first <= 0xdbff
    || n == (first < 128 ? 1 : first < 224 ? 2 : first < 240 ? 3 : 4);
  };
};
let feed =
    (~timeout=500., ~time_stamp, ~key, ~from, ~labels, ~disabled, state) =>
  if (!character(key)) {
    (state, None);
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
