/** Minimal mutable string-keyed hash table. Unlike Stdlib.Hashtbl it has no
    randomisation support, so browser bundles do not pull in Random, Int64
    and Digest. Iteration order is unspecified. */
type t('a);
let create: int => t('a);
let length: t('a) => int;
let mem: (t('a), string) => bool;
let find_opt: (t('a), string) => option('a);
/** Add or overwrite the binding for a key. */
let replace: (t('a), string, 'a) => unit;
let remove: (t('a), string) => unit;
let iter: ((string, 'a) => unit, t('a)) => unit;
let fold: ((string, 'a, 'acc) => 'acc, t('a), 'acc) => 'acc;
