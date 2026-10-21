/** Fetch failures, including decoding and non-2xx responses. */
type error =
  | Bad_url(string)
  | Timeout
  | Network_error
  | Bad_status(int)
  | Bad_body(string);
/** A response-to-message description. */
type expect('msg);
/** An optional request payload. */
type body;
/** No payload. */
let empty_body: body;
/** UTF-8 text payload and its content type. */
let string_body: (string, string) => body;
/** JSON payload with application/json content type. */
let json_body: Js.Json.t => body;
/** Expect response text. */
let expect_string: (result(string, error) => 'msg) => expect('msg);
/** Parse JSON then interpret a portable decoder over it. */
let expect_json:
  (result('a, error) => 'msg, Tea_core.Decode.t('a)) => expect('msg);
/** Ignore the successful response body. */
let expect_whatever: (result(unit, error) => 'msg) => expect('msg);
/** GET command. */
let get: (~url: string, ~expect: expect('msg)) => Tea_core.Cmd.t('msg);
/** POST command. */
let post:
  (~url: string, ~body: body, ~expect: expect('msg)) => Tea_core.Cmd.t('msg);
/** General request. Timeout is milliseconds; zero disables it. Headers preserve order.
    An explicit Content-Type overrides the body's default. */
let request:
  (
    ~method_: string,
    ~headers: list((string, string)),
    ~url: string,
    ~body: body,
    ~expect: expect('msg),
    ~timeout: float=?,
    unit
  ) =>
  Tea_core.Cmd.t('msg);
/** Lazy general request task yielding text; use Task.and_then for typed decoding. */
let request_task:
  (
    ~method_: string,
    ~headers: list((string, string)),
    ~url: string,
    ~body: body,
    ~timeout: float=?,
    unit
  ) =>
  Tea_core.Task.t(string, error);
/** Lazy GET task. */
let get_task:
  (~url: string, ~timeout: float=?, unit) => Tea_core.Task.t(string, error);
/** Lazy POST task. */
let post_task:
  (~url: string, ~body: body, ~timeout: float=?, unit) =>
  Tea_core.Task.t(string, error);
/** Decode JSON text, returning Bad_body on malformed JSON or decoding failure. */
let decode_json: (Tea_core.Decode.t('a), string) => result('a, error);
/** Raw JSON interpreter and portable value adapters. */
module Json: {
  /** Interpret without first copying the input into Decode.Value. */
  let decode_value: (Tea_core.Decode.t('a), Js.Json.t) => result('a, string);
  /** Convert a portable JSON value. */
  let of_value: Tea_core.Decode.Value.t => Js.Json.t;
  /** Copy raw JSON into a portable value. Input must be JSON (not cyclic JS objects). */
  let to_value: Js.Json.t => Tea_core.Decode.Value.t;
};
