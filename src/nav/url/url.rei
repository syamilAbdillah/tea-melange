/** Supported absolute URL protocols. */
type protocol =
  | Http
  | Https;
/** Encoded path/query/fragment; host is normalized to lowercase. */
type t = {
  protocol,
  host: string,
  port: option(int),
  path: string,
  query: option(string),
  fragment: option(string),
};
/** Parse an absolute HTTP(S) URL; reject credentials and malformed ports. */
let from_string: string => option(t);
/** Serialize with an explicit slash for an empty path. */
let to_string: t => string;
/** Decode percent-encoded bytes; reject malformed escapes. */
let percent_decode: string => option(string);
/** Query decoders select the first matching parameter; plus means space. */
module Query: {
  /** Query parameter description. */
  type t('a);
  /** Read a string parameter. */
  let string: string => t(string);
  /** Read an integer parameter. */
  let int: string => t(int);
};
/** Value-oriented parser combinators. Slash pairs results; map removes units. */
module Parser: {
  /** The URL consumed by a parser. */
  type url = t;
  /** A parser consuming path segments. */
  type t('a);
  /** Consume nothing. */
  let top: t(unit);
  /** Consume a literal segment. */
  let s: string => t(unit);
  /** Consume a percent-decoded segment. */
  let string: t(string);
  /** Consume an integer segment. */
  let int: t(int);
  /** Sequence parsers, pairing their results. */
  let slash: (t('a), t('b)) => t(('a, 'b));
  /** Transform the parsed value. */
  let map: ('a => 'b, t('a)) => t('b);
  /** Try alternatives in order, with backtracking. */
  let one_of: list(t('a)) => t('a);
  /** Read a query parameter without consuming path segments. */
  let query: Query.t('a) => t(option('a));
  /** Read the percent-decoded fragment. */
  let fragment: t(option(string));
  /** Require the whole path; one trailing slash is accepted. */
  let parse: (t('a), url) => option('a);
};
