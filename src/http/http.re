module Json = {
  module D = Tea_core.Decode;
  let empty_dict: unit => Js.Dict.t(Js.Json.t) = [%mel.raw
    {|function() { return Object.create(null); }|}
  ];
  let has_own: (Js.Dict.t(Js.Json.t), string) => bool = [%mel.raw
    {|function(obj, key) { return Object.prototype.hasOwnProperty.call(obj, key); }|}
  ];
  let rec of_value = (v: D.Value.t) =>
    switch (v) {
    | Null => Js.Json.null
    | Bool(b) => Js.Json.boolean(b)
    | Number(n) => Js.Json.number(n)
    | String(s) => Js.Json.string(s)
    | Array(xs) => Js.Json.array(Array.map(of_value, xs))
    | Object(xs) =>
      let dict = empty_dict();
      List.iter(((k, v)) => Js.Dict.set(dict, k, of_value(v)), xs);
      Js.Json.object_(dict);
    };
  let rec to_value = v =>
    switch (Js.Json.classify(v)) {
    | JSONNull => D.Value.Null
    | JSONTrue => D.Value.Bool(true)
    | JSONFalse => D.Value.Bool(false)
    | JSONNumber(n) => D.Value.Number(n)
    | JSONString(s) => D.Value.String(s)
    | JSONArray(xs) => D.Value.Array(Array.map(to_value, xs))
    | JSONObject(xs) =>
      D.Value.Object(
        Array.to_list(
          Array.map(((k, v)) => (k, to_value(v)), Js.Dict.entries(xs)),
        ),
      )
    };
  let describe = v =>
    switch (Js.Json.classify(v)) {
    | JSONNull => "null"
    | JSONTrue
    | JSONFalse => "a boolean"
    | JSONNumber(_) => "a number"
    | JSONString(_) => "a string"
    | JSONArray(_) => "an array"
    | JSONObject(_) => "an object"
    };

  let expected = (what, v) =>
    Error("expected " ++ what ++ ", got " ++ describe(v));

  open D;
  let rec decode_value: type a. (D.t(a), Js.Json.t) => result(a, string) =
          (d, v) =>
    switch (d, Js.Json.classify(v)) {
    | (Succeed(x), _) => Ok(x)
    | (Fail(e), _) => Error(e)
    | (String, JSONString(s)) => Ok(s)
    | (String, _) => expected("a string", v)
    | (Float, JSONNumber(n)) => Ok(n)
    | (Float, _) => expected("a number", v)
    | (Int, JSONNumber(n))
        when Float.is_integer(n) && n >= (-2147483648.) && n <= 2147483647. =>
      Ok(int_of_float(n))
    | (Int, _) => expected("an integer", v)
    | (Bool, JSONTrue) => Ok(true)
    | (Bool, JSONFalse) => Ok(false)
    | (Bool, _) => expected("a boolean", v)
    | (Null(x), JSONNull) => Ok(x)
    | (Null(_), _) => expected("null", v)
    | (Value, _) => Ok(to_value(v))
    | (Field(name, d), JSONObject(fields)) =>
      switch (has_own(fields, name) ? Js.Dict.get(fields, name) : None) {
      | Some(x) =>
        switch (decode_value(d, x)) {
        | Ok(_) as ok => ok
        | Error(e) => Error("at field '" ++ name ++ "': " ++ e)
        }
      | None => Error("missing field '" ++ name ++ "'")
      }
    | (Field(name, _), _) =>
      expected("an object with field '" ++ name ++ "'", v)
    | (Index(i, d), JSONArray(items)) =>
      if (i >= 0 && i < Array.length(items)) {
        switch (decode_value(d, items[i])) {
        | Ok(_) as ok => ok
        | Error(e) => Error("at index " ++ string_of_int(i) ++ ": " ++ e)
        };
      } else {
        Error("index " ++ string_of_int(i) ++ " out of bounds");
      }
    | (Index(_), _) => expected("an array", v)
    | (List(d), JSONArray(items)) =>
      switch (decode_value(Array(d), Js.Json.array(items))) {
      | Ok(a) => Ok(Array.to_list(a))
      | Error(_) as e => e
      }
    | (List(_), _) => expected("an array", v)
    | (Array(d), JSONArray(items)) =>
      let n = Array.length(items);
      let rec go = (i, acc) =>
        if (i < 0) {
          Ok(Array.of_list(acc));
        } else {
          switch (decode_value(d, items[i])) {
          | Ok(x) => go(i - 1, [x, ...acc])
          | Error(e) => Error("at index " ++ string_of_int(i) ++ ": " ++ e)
          };
        };
      go(n - 1, []);
    | (Array(_), _) => expected("an array", v)
    | (Optional(d), _) =>
      switch (decode_value(d, v)) {
      | Ok(x) => Ok(Some(x))
      | Error(_) => Ok(None)
      }
    | (Map(f, d), _) =>
      switch (decode_value(d, v)) {
      | Ok(x) => Ok(f(x))
      | Error(_) as e => e
      }
    | (Map2(f, a, b), _) =>
      switch (decode_value(a, v)) {
      | Error(_) as e => e
      | Ok(x) =>
        switch (decode_value(b, v)) {
        | Ok(y) => Ok(f(x, y))
        | Error(_) as e => e
        }
      }
    | (And_then(f, d), _) =>
      switch (decode_value(d, v)) {
      | Ok(x) => decode_value(f(x), v)
      | Error(_) as e => e
      }
    | (One_of(ds), _) =>
      let rec go = (ds, errors) =>
        switch (ds) {
        | [] =>
          Error("one_of failed: " ++ String.concat("; ", List.rev(errors)))
        | [d, ...rest] =>
          switch (decode_value(d, v)) {
          | Ok(_) as ok => ok
          | Error(e) => go(rest, [e, ...errors])
          }
        };
      go(ds, []);
    };
};
type error =
  | Bad_url(string)
  | Timeout
  | Network_error
  | Bad_status(int)
  | Bad_body(string);
type expect('msg) = result(string, error) => 'msg;
type body =
  | Empty
  | String_body(string, string);
let empty_body = Empty;
let string_body = (mime, s) => String_body(mime, s);
let json_body = json =>
  string_body("application/json", Js.Json.stringify(json));
let expect_string = f => f;
let expect_whatever = (f, result) => f(Result.map(_ => (), result));
let decode_json = (decoder, text) => {
  let parsed =
    try(Ok(Js.Json.parseExn(text))) {
    | _ => Error(Bad_body("invalid JSON"))
    };
  Result.bind(parsed, value =>
    Result.map_error(e => Bad_body(e), Json.decode_value(decoder, value))
  );
};
let expect_json = (f, decoder, result) =>
  f(Result.bind(result, text => decode_json(decoder, text)));
let transport:
  (
    string,
    array(array(string)),
    string,
    Js.Nullable.t(string),
    float,
    (int, int, string) => unit
  ) =>
  unit = [%mel.raw
  {|function(method, headers, url, body, timeout, cb) {
  var resolved;
  try { resolved = new URL(url, typeof location !== 'undefined' ? location.href : undefined).href; }
  catch (_) { cb(1, 0, url); return; }
  var controller = new AbortController(), timer, finished = false;
  function finish(kind, status, text) { if (finished) return; finished = true; if (timer !== undefined) clearTimeout(timer); queueMicrotask(function() { cb(kind, status, text); }); }
  if (timeout > 0) timer = setTimeout(function() { controller.abort(); finish(2, 0, ''); }, timeout);
  try {
    fetch(resolved, {method:method, headers:headers, body:body, signal:controller.signal}).then(function(r) {
      if (!r.ok) { finish(4, r.status, ''); return; }
      return r.text().then(function(text) { finish(0, r.status, text); });
    }).catch(function() { finish(3, 0, ''); });
  } catch (_) { finish(3, 0, ''); }
}|}
];
let request_task = (~method_, ~headers, ~url, ~body, ~timeout=0., (), done_) => {
  let (headers, body) =
    switch (body) {
    | Empty => (headers, Js.Nullable.null)
    | String_body(mime, s) =>
      let has_type =
        List.exists(
          ((name, _)) => String.lowercase_ascii(name) == "content-type",
          headers,
        );
      (
        has_type ? headers : [("Content-Type", mime), ...headers],
        Js.Nullable.return(s),
      );
    };
  transport(
    method_,
    Array.of_list(List.map(((k, v)) => [|k, v|], headers)),
    url,
    body,
    timeout,
    (kind, status, text) =>
    done_(
      switch (kind) {
      | 0 => Ok(text)
      | 1 => Error(Bad_url(text))
      | 2 => Error(Timeout)
      | 4 => Error(Bad_status(status))
      | _ => Error(Network_error)
      },
    )
  );
};
let get_task = (~url, ~timeout=?, ()) =>
  request_task(
    ~method_="GET",
    ~headers=[],
    ~url,
    ~body=empty_body,
    ~timeout?,
    (),
  );
let post_task = (~url, ~body, ~timeout=?, ()) =>
  request_task(~method_="POST", ~headers=[], ~url, ~body, ~timeout?, ());
let request = (~method_, ~headers, ~url, ~body, ~expect, ~timeout=?, ()) =>
  Tea_core.Task.attempt(
    expect,
    request_task(~method_, ~headers, ~url, ~body, ~timeout?, ()),
  );
let get = (~url, ~expect) =>
  request(~method_="GET", ~headers=[], ~url, ~body=empty_body, ~expect, ());
let post = (~url, ~body, ~expect) =>
  request(~method_="POST", ~headers=[], ~url, ~body, ~expect, ());
