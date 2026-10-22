/* Bridge JSON text to the portable Decode.Value the contract is written in. */
module V = Tea_core.Decode.Value;

let rec of_yojson: Yojson.Basic.t => V.t =
  fun
  | `Null => V.Null
  | `Bool(b) => V.Bool(b)
  | `Int(i) => V.Number(float_of_int(i))
  | `Float(f) => V.Number(f)
  | `String(s) => V.String(s)
  | `List(items) => V.Array(Array.of_list(List.map(of_yojson, items)))
  | `Assoc(fields) =>
    V.Object(
      List.map(((key, value)) => (key, of_yojson(value)), fields),
    );

let rec to_yojson: V.t => Yojson.Basic.t =
  fun
  | V.Null => `Null
  | V.Bool(b) => `Bool(b)
  | V.Number(n) when Float.is_integer(n) && Float.abs(n) < 9007199254740992.0 =>
    `Int(int_of_float(n))
  | V.Number(n) => `Float(n)
  | V.String(s) => `String(s)
  | V.Array(items) => `List(Array.to_list(Array.map(to_yojson, items)))
  | V.Object(fields) =>
    `Assoc(List.map(((key, value)) => (key, to_yojson(value)), fields));

let of_string = text =>
  switch (Yojson.Basic.from_string(text)) {
  | json => Ok(of_yojson(json))
  | exception (Yojson.Json_error(message)) => Error(message)
  };

let to_string = value => Yojson.Basic.to_string(to_yojson(value));
