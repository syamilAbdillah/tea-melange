/** Pure, data-only decoders (Elm's [Json.Decode]).

    A decoder is a description, not a function: it is interpreted by a
    backend. [run] interprets it over the portable {!Value.t} (used by native
    tests and [tea.test]); [tea.dom] and [tea.http] interpret the same
    description directly over raw JS values (DOM events, parsed JSON). */

module Value: {
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

/** Always succeed with the supplied value. */
let succeed: 'a => t('a);
/** Always fail with an explanatory message. */
let fail: string => t('a);
/** Decode a string. */
let string: t(string);
/** Decode a numeric value. */
let float: t(float);
/** Decode an integral numeric value using the host integer conversion. */
let int: t(int);
/** Decode a boolean. */
let bool: t(bool);
/** Decode null as the supplied value. */
let null: 'a => t('a);
/** Retain the portable input value. */
let value: t(Value.t);
/** Decode a named object field. */
let field: (string, t('a)) => t('a);
/** [at(["target", "value"], string)] */
let at: (list(string), t('a)) => t('a);
/** Decode an array element at a zero-based index. */
let index: (int, t('a)) => t('a);
/** Decode every element as a list. */
let list: t('a) => t(list('a));
/** Decode every element as an array. */
let array: t('a) => t(array('a));
/** Succeeds with [None] when the value is null/missing or does not match. */
let optional: t('a) => t(option('a));
/** Transform a decoded value. */
let map: ('a => 'b, t('a)) => t('b);
/** Combine two decoders over the same input. */
let map2: (('a, 'b) => 'c, t('a), t('b)) => t('c);
/** Combine three decoders over the same input. */
let map3: (('a, 'b, 'c) => 'd, t('a), t('b), t('c)) => t('d);
/** Choose a decoder from a result, then run it over the original input. */
let and_then: ('a => t('b), t('a)) => t('b);
/** Try decoders in order until one succeeds. */
let one_of: list(t('a)) => t('a);

/** Interpret a decoder over a portable value. */
let run: (t('a), Value.t) => result('a, string);
