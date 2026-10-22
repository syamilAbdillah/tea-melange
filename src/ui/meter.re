module Html = Tea_html.Html;
type parts('msg) = {
  root: list(Html.attr('msg)),
  label: list(Html.attr('msg)),
  value: float,
  percent: float,
  state: string,
};
let finite = value =>
  switch (classify_float(value)) {
  | FP_nan
  | FP_infinite => false
  | _ => true
  };
let clamp = (~min, ~max, value) =>
  classify_float(value) == FP_nan
    ? min : value < min ? min : value > max ? max : value;
let threshold = (~min, ~max, ~default, value) =>
  switch (value) {
  | Some(value) when finite(value) => clamp(~min, ~max, value)
  | _ => default
  };
let parts =
    (
      ~id,
      ~value,
      ~has_label=true,
      ~aria_label=?,
      ~min=0.,
      ~max=1.,
      ~low=?,
      ~high=?,
      ~optimum=?,
      ~format=?,
      (),
    ) => {
  if (!finite(min) || !finite(max) || min >= max) {
    invalid_arg("Meter: expected finite min < max");
  };
  let aria_label = Option.map(Js.String.trim, aria_label);
  if (Option.fold(~none=!has_label, ~some=String.equal(""), aria_label)) {
    invalid_arg(
      "Meter: render a nonempty label or provide a nonblank aria_label",
    );
  };
  let value = clamp(~min, ~max, value);
  let low = threshold(~min, ~max, ~default=min, low);
  let high = threshold(~min=low, ~max, ~default=max, high);
  let midpoint =
    finite(min +. max) ? (min +. max) /. 2. : min /. 2. +. max /. 2.;
  let optimum = threshold(~min, ~max, ~default=midpoint, optimum);
  let state =
    if (optimum < low) {
      value <= low ? "optimum" : value <= high ? "suboptimal" : "critical";
    } else if (optimum > high) {
      value >= high ? "optimum" : value >= low ? "suboptimal" : "critical";
    } else {
      value >= low && value <= high ? "optimum" : "suboptimal";
    };
  /* Half-scale only when subtracting finite endpoints overflows. */
  let percent =
    100.
    *. (
      finite(max -. min)
        ? (value -. min) /. (max -. min)
        : (value /. 2. -. min /. 2.) /. (max /. 2. -. min /. 2.)
    );
  {
    root: [
      Html.id(id),
      Html.role("meter"),
      Html.aria("valuemin", Js.Float.toString(min)),
      Html.aria("valuemax", Js.Float.toString(max)),
      Html.aria("valuenow", Js.Float.toString(value)),
      switch (format) {
      | None => Html.none
      | Some(format) => Html.aria("valuetext", format(value))
      },
      Ui_attr.aria_ids(
        "labelledby",
        has_label ? [Ui_attr.part_id(id, "label")] : [],
      ),
      switch (aria_label) {
      | Some(label) => Html.aria("label", label)
      | None => Html.none
      },
      Html.data("state", state),
      Html.style("--tea-meter-percent", Js.Float.toString(percent) ++ "%"),
    ],
    label: [Html.id(Ui_attr.part_id(id, "label"))],
    value,
    percent,
    state,
  };
};
let view =
    (
      ~id,
      ~value,
      ~has_label=true,
      ~aria_label=?,
      ~min=0.,
      ~max=1.,
      ~low=?,
      ~high=?,
      ~optimum=?,
      ~format=?,
      ~to_view,
      (),
    ) =>
  to_view(
    parts(
      ~id,
      ~value,
      ~has_label,
      ~aria_label?,
      ~min,
      ~max,
      ~low?,
      ~high?,
      ~optimum?,
      ~format?,
      (),
    ),
  );
