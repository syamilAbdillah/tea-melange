open Tea;

/* counter.re */
type msg =
  | Increment
  | Decrement
  | Set(int);

let update = (msg, model) =>
  switch (msg) {
  | Increment => model + 1
  | Decrement => model - 1
  | Set(n) => n
  };

let view = model =>
  Html.div(
    [],
    [
      Html.button([Html.on_click(Decrement)], [Html.text("−")]),
      Html.span([], [Html.text(string_of_int(model))]),
      Html.button([Html.on_click(Increment)], [Html.text("+")]),
    ],
  );

let () = App.beginner(~init=0, ~update, ~view, ~node="#app", ());
