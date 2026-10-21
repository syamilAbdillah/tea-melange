module Html = Tea_html.Html;
type parts('msg) = {
  root: list(Html.attr('msg)),
  label: list(Html.attr('msg)),
  region: list(Html.attr('msg)),
  value: option(float),
  percent: option(float),
  state: string,
};
let finite = value =>
  switch (classify_float(value)) {
  | FP_nan
  | FP_infinite => false
  | _ => true
  };
let busy = loading => Ui_attr.aria_bool("busy", loading);
let parts = (~id, ~value=?, ~min=0., ~max=100., ~format=?, ()) => {
  if (!finite(min) || !finite(max) || min >= max) {
    invalid_arg("Progress: expected finite min < max");
  };
  let value =
    Option.map(
      value =>
        classify_float(value) == FP_nan
          ? min : value < min ? min : value > max ? max : value,
      value,
    );
  let percent =
    Option.map(
      value =>
        100.
        *. (
          finite(max -. min)
            ? (value -. min) /. (max -. min)
            : (value /. 2. -. min /. 2.) /. (max /. 2. -. min /. 2.)
        ),
      value,
    );
  let state =
    switch (value) {
    | None => "indeterminate"
    | Some(value) => value >= max ? "complete" : "loading"
    };
  {
    root: [
      Html.id(id),
      Html.role("progressbar"),
      Html.aria("valuemin", Js.Float.toString(min)),
      Html.aria("valuemax", Js.Float.toString(max)),
      switch (value) {
      | None => Html.none
      | Some(value) => Html.aria("valuenow", Js.Float.toString(value))
      },
      switch (value, format) {
      | (Some(value), Some(format)) =>
        Html.aria("valuetext", format(value))
      | _ => Html.none
      },
      Ui_attr.aria_ids("labelledby", [Ui_attr.part_id(id, "label")]),
      Html.data("state", state),
      switch (percent) {
      | None => Html.none
      | Some(percent) =>
        Html.style(
          "--tea-progress-percent",
          Js.Float.toString(percent) ++ "%",
        )
      },
    ],
    label: [Html.id(Ui_attr.part_id(id, "label"))],
    region: [
      busy(!String.equal(state, "complete")),
      Ui_attr.aria_ids("describedby", [id]),
    ],
    value,
    percent,
    state,
  };
};
let view = (~id, ~value=?, ~min=0., ~max=100., ~format=?, ~to_view, ()) =>
  to_view(parts(~id, ~value?, ~min, ~max, ~format?, ()));
