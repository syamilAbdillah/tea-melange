/* Range arithmetic shared by Meter and Progress. */
let finite = Js.Float.isFinite;
/** Clamp into [min, max]; NaN becomes [min]. */
let clamp = (~min, ~max, value) =>
  Js.Float.isNaN(value) ? min : value < min ? min : value > max ? max : value;
/** Position of [value] in [min, max] as 0–100, halving the operands when
    [max - min] itself overflows. */
let percent = (~min, ~max, value) =>
  100.
  *. (
    finite(max -. min)
      ? (value -. min) /. (max -. min)
      : (value /. 2. -. min /. 2.) /. (max /. 2. -. min /. 2.)
  );
/** Raise unless [min] < [max] are both finite. */
let check_range = (~component, ~min, ~max) =>
  if (!finite(min) || !finite(max) || min >= max) {
    invalid_arg(component ++ ": expected finite min < max");
  };
