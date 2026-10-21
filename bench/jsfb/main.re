/* js-framework-benchmark style keyed table. [?lazy] selects the optimized
   view (Html.lazy2 rows); otherwise every row is rebuilt each render. */
open Tea;
open Html;

type row = {
  id: int,
  label: string,
};
type model = {
  rows: array(row),
  selected: int,
  next_id: int,
  seed: int,
};
type msg =
  | Run(int)
  | Add
  | Update
  | Clear
  | Swap
  | Select(int)
  | Remove(int);

let adjectives = [|
  "pretty",
  "large",
  "big",
  "small",
  "tall",
  "short",
  "long",
  "handsome",
  "plain",
  "quaint",
  "clean",
  "elegant",
  "easy",
  "angry",
  "crazy",
  "helpful",
  "mushy",
  "odd",
  "unsightly",
  "adorable",
  "important",
  "inexpensive",
  "cheap",
  "expensive",
  "fancy",
|];
let colours = [|
  "red",
  "yellow",
  "blue",
  "green",
  "pink",
  "brown",
  "purple",
  "brown",
  "white",
  "black",
  "orange",
|];
let nouns = [|
  "table",
  "chair",
  "house",
  "bbq",
  "desk",
  "car",
  "pony",
  "cookie",
  "sandwich",
  "burger",
  "pizza",
  "mouse",
  "keyboard",
|];

let next = seed => (seed * 1103515245 + 12345) land 0x3fffffff;
let build = (model, n) => {
  let seed = ref(model.seed);
  let pick = a => {
    seed := next(seed^);
    a[seed^ mod Array.length(a)];
  };
  let rows =
    Array.init(n, i =>
      {
        id: model.next_id + i,
        label:
          pick(adjectives) ++ " " ++ pick(colours) ++ " " ++ pick(nouns),
      }
    );
  (
    rows,
    {
      ...model,
      next_id: model.next_id + n,
      seed: seed^,
    },
  );
};

let update = (msg, model) =>
  switch (msg) {
  | Run(n) =>
    let (rows, model) = build(model, n);
    {
      ...model,
      rows,
      selected: (-1),
    };
  | Add =>
    let (rows, model) = build(model, 1000);
    {
      ...model,
      rows: Array.append(model.rows, rows),
    };
  | Update => {
      ...model,
      rows:
        Array.mapi(
          (i, r) =>
            i mod 10 == 0
              ? {
                ...r,
                label: r.label ++ " !!!",
              }
              : r,
          model.rows,
        ),
    }
  | Clear => {
      ...model,
      rows: [||],
      selected: (-1),
    }
  | Swap =>
    if (Array.length(model.rows) > 998) {
      let rows = Array.copy(model.rows);
      let a = rows[1];
      rows[1] = rows[998];
      rows[998] = a;
      {
        ...model,
        rows,
      };
    } else {
      model;
    }
  | Select(id) => {
      ...model,
      selected: id,
    }
  | Remove(id) => {
      ...model,
      rows:
        Array.of_list(
          List.filter(r => r.id != id, Array.to_list(model.rows)),
        ),
    }
  };

let row_view = (selected, r) => {
  let id = string_of_int(r.id);
  tr(
    [class_(selected ? "danger" : "")],
    [
      td([class_("col-md-1")], [text(id)]),
      td(
        [class_("col-md-4")],
        [a([on_click(Select(r.id))], [text(r.label)])],
      ),
      td(
        [class_("col-md-1")],
        [
          a(
            [on_click(Remove(r.id))],
            [
              span(
                [
                  class_("glyphicon glyphicon-remove"),
                  attribute("aria-hidden", "true"),
                ],
                [],
              ),
            ],
          ),
        ],
      ),
      td([class_("col-md-6")], []),
    ],
  );
};

let button_ = (id, label, msg) =>
  div(
    [class_("col-sm-6 smallpad")],
    [
      button(
        [
          type_("button"),
          class_("btn btn-primary btn-block"),
          Attr.id(id),
          on_click(msg),
        ],
        [text(label)],
      ),
    ],
  );

let view = (lazy_, model) =>
  div(
    [class_("container")],
    [
      div(
        [class_("jumbotron")],
        [
          div(
            [class_("row")],
            [
              div([class_("col-md-6")], [h1([], [text("tea-melange")])]),
              div(
                [class_("col-md-6")],
                [
                  div(
                    [class_("row")],
                    [
                      button_("run", "Create 1,000 rows", Run(1000)),
                      button_("runlots", "Create 10,000 rows", Run(10000)),
                      button_("add", "Append 1,000 rows", Add),
                      button_("update", "Update every 10th row", Update),
                      button_("clear", "Clear", Clear),
                      button_("swaprows", "Swap Rows", Swap),
                    ],
                  ),
                ],
              ),
            ],
          ),
        ],
      ),
      table(
        [class_("table table-hover table-striped test-data")],
        [
          keyed(
            "tbody",
            [],
            Array.to_list(
              Array.map(
                r =>
                  (
                    string_of_int(r.id),
                    lazy_
                      ? lazy2(row_view, r.id == model.selected, r)
                      : row_view(r.id == model.selected, r),
                  ),
                model.rows,
              ),
            ),
          ),
        ],
      ),
    ],
  );

let lazy_: bool = [%mel.raw {|location.search.includes("lazy")|}];

let () =
  App.beginner(
    ~init={
      rows: [||],
      selected: (-1),
      next_id: 1,
      seed: 42,
    },
    ~update,
    ~view=view(lazy_),
    ~node="#main",
    (),
  );
