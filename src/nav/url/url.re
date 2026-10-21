type protocol =
  | Http
  | Https;
type t = {
  protocol,
  host: string,
  port: option(int),
  path: string,
  query: option(string),
  fragment: option(string),
};
let split_once = (c, s) =>
  switch (String.index_opt(s, c)) {
  | None => (s, None)
  | Some(i) => (
      String.sub(s, 0, i),
      Some(String.sub(s, i + 1, String.length(s) - i - 1)),
    )
  };
let percent_decode = s => {
  let b = Buffer.create(String.length(s));
  let hex = c =>
    if (c >= '0' && c <= '9') {
      Char.code(c) - 48;
    } else if (c >= 'a' && c <= 'f') {
      Char.code(c) - 87;
    } else if (c >= 'A' && c <= 'F') {
      Char.code(c) - 55;
    } else {
      (-1);
    };
  let rec loop = i =>
    if (i == String.length(s)) {
      Some(Buffer.contents(b));
    } else if (s.[i] == '%') {
      if (i + 2 >= String.length(s)) {
        None;
      } else {
        let a = hex(s.[i + 1]);
        let c = hex(s.[i + 2]);
        if (a < 0 || c < 0) {
          None;
        } else {
          Buffer.add_char(b, Char.chr(a * 16 + c));
          loop(i + 3);
        };
      };
    } else {
      Buffer.add_char(b, s.[i]);
      loop(i + 1);
    };
  loop(0);
};
let from_string = input => {
  let prefix = (p, s) => String.starts_with(~prefix=p, s);
  let scheme =
    prefix("https://", input)
      ? Some((Https, 8)) : prefix("http://", input) ? Some((Http, 7)) : None;
  switch (scheme) {
  | None => None
  | Some((protocol, offset)) =>
    let rest = String.sub(input, offset, String.length(input) - offset);
    let (rest, fragment) = split_once('#', rest);
    let (rest, query) = split_once('?', rest);
    let (authority, path) =
      switch (String.index_opt(rest, '/')) {
      | None => (rest, "/")
      | Some(i) => (
          String.sub(rest, 0, i),
          String.sub(rest, i, String.length(rest) - i),
        )
      };
    let host_port =
      if (String.length(authority) > 0 && authority.[0] == '[') {
        switch (String.index_opt(authority, ']')) {
        | None => None
        | Some(i) =>
          let host = String.sub(authority, 0, i + 1);
          let suffix =
            String.sub(authority, i + 1, String.length(authority) - i - 1);
          if (suffix == "") {
            Some((host, None));
          } else if (suffix.[0] == ':') {
            Some((
              host,
              Some(String.sub(suffix, 1, String.length(suffix) - 1)),
            ));
          } else {
            None;
          };
        };
      } else {
        Some(split_once(':', authority));
      };
    switch (host_port) {
    | None => None
    | Some((host, raw_port)) =>
      let port =
        switch (raw_port) {
        | None => Some(None)
        | Some(s) =>
          switch (int_of_string_opt(s)) {
          | Some(p)
              when
                p >= 0
                && p <= 65535
                && s != ""
                && String.for_all(c => c >= '0' && c <= '9', s) =>
            Some(Some(p))
          | _ => None
          }
        };
      if (host == ""
          || String.exists(
               c =>
                 c <= ' '
                 || c == '@'
                 || c == '\\'
                 || c == ':'
                 && host.[0] != '[',
               host,
             )
          || String.exists(c => c <= ' ' || c == '\\', input)) {
        None;
      } else {
        switch (port) {
        | None => None
        | Some(port) =>
          Some({
            protocol,
            host: String.lowercase_ascii(host),
            port,
            path,
            query,
            fragment,
          })
        };
      };
    };
  };
};
let to_string = u =>
  (u.protocol == Https ? "https://" : "http://")
  ++ u.host
  ++ (
    switch (u.port) {
    | None => ""
    | Some(p) => ":" ++ string_of_int(p)
    }
  )
  ++ u.path
  ++ (
    switch (u.query) {
    | None => ""
    | Some(s) => "?" ++ s
    }
  )
  ++ (
    switch (u.fragment) {
    | None => ""
    | Some(s) => "#" ++ s
    }
  );
let decimal_int = s => {
  let n = String.length(s);
  let start = n > 0 && (s.[0] == '-' || s.[0] == '+') ? 1 : 0;
  let rec digits = i =>
    i == n || s.[i] >= '0' && s.[i] <= '9' && digits(i + 1);
  if (n == start || !digits(start)) {
    None;
  } else {
    switch (float_of_string_opt(s)) {
    | Some(f) when f >= (-2147483648.) && f <= 2147483647. =>
      int_of_string_opt(s)
    | _ => None
    };
  };
};
module Query = {
  type t('a) = option(string) => option('a);
  let find = (name, raw) => {
    let decode = s => percent_decode(String.map(c => c == '+' ? ' ' : c, s));
    let rec go = xs =>
      switch (xs) {
      | [] => None
      | [s, ...rest] =>
        let (key, value) = split_once('=', s);
        if (decode(key) == Some(name)) {
          decode(Option.value(~default="", value));
        } else {
          go(rest);
        };
      };
    go(String.split_on_char('&', Option.value(~default="", raw)));
  };
  let string = (name, raw) => find(name, raw);
  let int = (name, raw) => Option.bind(find(name, raw), decimal_int);
};
module Parser = {
  type url = t;
  type state = {
    segments: list(string),
    url: t,
  };
  type t('a) = state => list(('a, state));
  let top = state => [((), state)];
  let string = state =>
    switch (state.segments) {
    | [] => []
    | [s, ...rest] =>
      switch (percent_decode(s)) {
      | None => []
      | Some(s) => [
          (
            s,
            {
              ...state,
              segments: rest,
            },
          ),
        ]
      }
    };
  let map = (f, p, state) =>
    List.map(((v, state)) => (f(v), state), p(state));
  let s = (expected, state) =>
    List.filter_map(
      ((s, next)) => s == expected ? Some(((), next)) : None,
      string(state),
    );
  let int = state =>
    List.filter_map(
      ((s, next)) => Option.map(n => (n, next), decimal_int(s)),
      string(state),
    );
  let slash = (a, b, state) =>
    List.concat_map(
      ((a, state)) =>
        List.map(((b, state)) => ((a, b), state), b(state)),
      a(state),
    );
  let one_of = (ps, state) => List.concat_map(p => p(state), ps);
  let query = (q, state) => [(q(state.url.query), state)];
  let fragment = state =>
    switch (state.url.fragment) {
    | None => [(None, state)]
    | Some(s) =>
      switch (percent_decode(s)) {
      | None => []
      | Some(s) => [(Some(s), state)]
      }
    };
  let parse = (p, url) => {
    let segments = String.split_on_char('/', url.path);
    let segments =
      switch (segments) {
      | ["", ...rest] => rest
      | xs => xs
      };
    let segments =
      List.rev(
        switch (List.rev(segments)) {
        | ["", ...rest] => rest
        | xs => xs
        },
      );
    List.find_map(
      ((v, state)) => state.segments == [] ? Some(v) : None,
      p({
        segments,
        url,
      }),
    );
  };
};
