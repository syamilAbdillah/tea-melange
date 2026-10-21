module Value = {
  type t =
    | Null
    | Bool(bool)
    | Number(float)
    | String(string)
    | Array(array(t))
    | Object(list((string, t)));
};

type t(_) =
  | Succeed('a): t('a)
  | Fail(string): t('a)
  | String: t(string)
  | Float: t(float)
  | Int: t(int)
  | Bool: t(bool)
  | Null('a): t('a)
  | Value: t(Value.t)
  | Field(string, t('a)): t('a)
  | Index(int, t('a)): t('a)
  | List(t('a)): t(list('a))
  | Array(t('a)): t(array('a))
  | Optional(t('a)): t(option('a))
  | Map('a => 'b, t('a)): t('b)
  | Map2(('a, 'b) => 'c, t('a), t('b)): t('c)
  | And_then('a => t('b), t('a)): t('b)
  | One_of(list(t('a))): t('a);

let succeed = x => Succeed(x);
let fail = e => Fail(e);
let string = String;
let float = Float;
let int = Int;
let bool = Bool;
let null = x => Null(x);
let value = Value;
let field = (name, d) => Field(name, d);
let at = (path, d) =>
  List.fold_right((name, acc) => Field(name, acc), path, d);
let index = (i, d) => Index(i, d);
let list = d => List(d);
let array = d => Array(d);
let optional = d => Optional(d);
let map = (f, d) => Map(f, d);
let map2 = (f, a, b) => Map2(f, a, b);
let map3 = (f, a, b, c) =>
  Map2((g, c) => g(c), Map2((a, b) => f(a, b), a, b), c);
let and_then = (f, d) => And_then(f, d);
let one_of = ds => One_of(ds);

let describe = (v: Value.t) =>
  switch (v) {
  | Null => "null"
  | Bool(_) => "a boolean"
  | Number(_) => "a number"
  | String(_) => "a string"
  | Array(_) => "an array"
  | Object(_) => "an object"
  };

let expected = (what, v) =>
  Error("expected " ++ what ++ ", got " ++ describe(v));

let rec run: type a. (t(a), Value.t) => result(a, string) = (d, v) =>
  switch (d, v) {
  | (Succeed(x), _) => Ok(x)
  | (Fail(e), _) => Error(e)
  | (String, String(s)) => Ok(s)
  | (String, _) => expected("a string", v)
  | (Float, Number(n)) => Ok(n)
  | (Float, _) => expected("a number", v)
  | (Int, Number(n)) when Float.is_integer(n) => Ok(int_of_float(n))
  | (Int, _) => expected("an integer", v)
  | (Bool, Bool(b)) => Ok(b)
  | (Bool, _) => expected("a boolean", v)
  | (Null(x), Null) => Ok(x)
  | (Null(_), _) => expected("null", v)
  | (Value, _) => Ok(v)
  | (Field(name, d), Object(fields)) =>
    switch (List.assoc_opt(name, fields)) {
    | Some(x) =>
      switch (run(d, x)) {
      | Ok(_) as ok => ok
      | Error(e) => Error("at field '" ++ name ++ "': " ++ e)
      }
    | None => Error("missing field '" ++ name ++ "'")
    }
  | (Field(name, _), _) =>
    expected("an object with field '" ++ name ++ "'", v)
  | (Index(i, d), Array(items)) =>
    if (i >= 0 && i < Array.length(items)) {
      switch (run(d, items[i])) {
      | Ok(_) as ok => ok
      | Error(e) => Error("at index " ++ string_of_int(i) ++ ": " ++ e)
      };
    } else {
      Error("index " ++ string_of_int(i) ++ " out of bounds");
    }
  | (Index(_), _) => expected("an array", v)
  | (List(d), Array(items)) =>
    switch (run(Array(d), Array(items))) {
    | Ok(a) => Ok(Array.to_list(a))
    | Error(_) as e => e
    }
  | (List(_), _) => expected("an array", v)
  | (Array(d), Array(items)) =>
    let n = Array.length(items);
    let rec go = (i, acc) =>
      if (i < 0) {
        Ok(Array.of_list(acc));
      } else {
        switch (run(d, items[i])) {
        | Ok(x) => go(i - 1, [x, ...acc])
        | Error(e) => Error("at index " ++ string_of_int(i) ++ ": " ++ e)
        };
      };
    go(n - 1, []);
  | (Array(_), _) => expected("an array", v)
  | (Optional(d), _) =>
    switch (run(d, v)) {
    | Ok(x) => Ok(Some(x))
    | Error(_) => Ok(None)
    }
  | (Map(f, d), _) =>
    switch (run(d, v)) {
    | Ok(x) => Ok(f(x))
    | Error(_) as e => e
    }
  | (Map2(f, a, b), _) =>
    switch (run(a, v)) {
    | Error(_) as e => e
    | Ok(x) =>
      switch (run(b, v)) {
      | Ok(y) => Ok(f(x, y))
      | Error(_) as e => e
      }
    }
  | (And_then(f, d), _) =>
    switch (run(d, v)) {
    | Ok(x) => run(f(x), v)
    | Error(_) as e => e
    }
  | (One_of(ds), _) =>
    let rec go = (ds, errors) =>
      switch (ds) {
      | [] =>
        Error("one_of failed: " ++ String.concat("; ", List.rev(errors)))
      | [d, ...rest] =>
        switch (run(d, v)) {
        | Ok(_) as ok => ok
        | Error(e) => go(rest, [e, ...errors])
        }
      };
    go(ds, []);
  };
