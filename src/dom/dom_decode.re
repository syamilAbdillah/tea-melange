open Tea_core;
open Bindings;
type raw = Js.Json.t;
let error = () => Error("DOM decoder: unexpected value or missing property");
let object_like = v =>
  !Js.testAny(v) && (Js.typeof(v) == "object" || Js.typeof(v) == "function");
let length = v =>
  if (!object_like(v)) {
    None;
  } else {
    let n = get(v, "length");
    if (Js.typeof(n) == "number") {
      let f: float = cast(n);
      if (Float.is_integer(f) && f >= 0. && f <= 2147483647.) {
        Some(int_of_float(f));
      } else {
        None;
      };
    } else {
      None;
    };
  };
let rec decode_value = v => {
  Decode.Value.(
    switch (Js.typeof(v)) {
    | "string" => String(cast(v))
    | "boolean" => Bool(cast(v))
    | "number" => Number(cast(v))
    | "object" when !Js.testAny(v) =>
      if (Js.Array.isArray(v)) {
        Array(Array.map(decode_value, cast(v): array(raw)));
      } else {
        Object(
          Array.to_list(
            Array.map(
              k => (k, decode_value(get(v, k))),
              Js.Dict.keys(cast(v)),
            ),
          ),
        );
      }
    | _ => Null
    }
  );
};
open Decode;
let rec run: type a. (Decode.t(a), raw) => result(a, string) = (d, v) => {
  switch (d) {
  | Succeed(x) => Ok(x)
  | Fail(e) => Error(e)
  | String =>
    if (Js.typeof(v) == "string") {
      Ok(cast(v));
    } else {
      error();
    }
  | Bool =>
    if (Js.typeof(v) == "boolean") {
      Ok(cast(v));
    } else {
      error();
    }
  | Float =>
    if (Js.typeof(v) == "number") {
      Ok(cast(v));
    } else {
      error();
    }
  | Int =>
    if (Js.typeof(v) == "number" && Float.is_integer(cast(v))) {
      Ok(int_of_float(cast(v)));
    } else {
      error();
    }
  | Null(x) =>
    if (v === Js.Json.null) {
      Ok(x);
    } else {
      error();
    }
  | Value => Ok(decode_value(v))
  | Field(k, d) =>
    if (object_like(v) && has(v, k)) {
      run(d, get(v, k));
    } else {
      error();
    }
  | Index(i, d) =>
    switch (length(v)) {
    | Some(n) when i >= 0 && i < n => run(d, get(v, string_of_int(i)))
    | _ => error()
    }
  | Array(d) =>
    switch (length(v)) {
    | None => error()
    | Some(n) =>
      let rec loop = (i, acc) =>
        if (i < 0) {
          Ok(Array.of_list(acc));
        } else {
          switch (run(d, get(v, string_of_int(i)))) {
          | Ok(x) => loop(i - 1, [x, ...acc])
          | Error(_) as e => e
          };
        };
      loop(n - 1, []);
    }
  | List(d) => Result.map(Array.to_list, run(Array(d), v))
  | Optional(d) =>
    Ok(
      switch (run(d, v)) {
      | Ok(x) => Some(x)
      | Error(_) => None
      },
    )
  | Map(f, d) => Result.map(f, run(d, v))
  | Map2(f, a, b) =>
    switch (run(a, v)) {
    | Error(_) as e => e
    | Ok(a) => Result.map(b => f(a, b), run(b, v))
    }
  | And_then(f, d) =>
    switch (run(d, v)) {
    | Error(_) as e => e
    | Ok(x) => run(f(x), v)
    }
  | One_of(ds) =>
    let rec loop = ds =>
      switch (ds) {
      | [] => error()
      | [d, ...rest] =>
        switch (run(d, v)) {
        | Ok(_) as x => x
        | Error(_) => loop(rest)
        }
      };
    loop(ds);
  };
};
