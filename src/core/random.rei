/** Portable xorshift32 state. Zero seeds are normalized to one. Not cryptographic. */
type seed;
/** Pure state transition producing a value and the next seed. */
type generator('a);
/** Initialize from the low 32 bits of an integer. */
let initial_seed: int => seed;
/** Run one generator without hidden state. */
let step: (generator('a), seed) => ('a, seed);
/** Inclusive integer bounds, restricted to signed 32-bit integers. */
let int: (int, int) => generator(int);
/** Finite ascending bounds; upper bound is exclusive. */
let float: (float, float) => generator(float);
/** Equally likely boolean. */
let bool: generator(bool);
/** Choose uniformly from a nonempty collection. */
let uniform: ('a, list('a)) => generator('a);
/** Nonnegative finite weights with positive finite total. */
let weighted: ((float, 'a), list((float, 'a))) => generator('a);
/** Generate a nonnegative number of values, in order. */
let list: (int, generator('a)) => generator(list('a));
/** Generate both values left to right. */
let pair: (generator('a), generator('b)) => generator(('a, 'b));
/** Transform a generated value. */
let map: ('a => 'b, generator('a)) => generator('b);
/** Combine two generated values. */
let map2: (('a, 'b) => 'c, generator('a), generator('b)) => generator('c);
/** Choose a generator from the previous result. */
let and_then: ('a => generator('b), generator('a)) => generator('b);
/** Return a value without advancing the seed. */
let constant: 'a => generator('a);
