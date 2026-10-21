open Tea;
module Drag = Ui.Drag_and_drop;

type demo = {
  id: string,
  title: string,
  note: string,
  orientation: Drag.orientation,
  dir: Drag.direction,
  drag: Drag.model,
  containers: array(Drag.container),
  moves: int,
};
type model = list(demo);
type msg =
  | Drag(string, Drag.msg)
  | Cancel(string);

let item = (~disabled=false, key, label): Drag.item => {
  key,
  label,
  disabled,
};
let container = (~disabled=false, key, label, items): Drag.container => {
  key,
  label,
  items,
  disabled,
};

/* Indices in Moved refer to the destination AFTER removing the source key.
   Never mutate the collection during preview, and never identify an item by index. */
let apply_move = (containers: array(Drag.container), move: Drag.movement) => {
  let source =
    containers
    |> Array.to_list
    |> List.map((c: Drag.container) => Array.to_list(c.items))
    |> List.flatten
    |> List.find_opt((i: Drag.item) => String.equal(i.key, move.key));
  switch (source) {
  | None => containers
  | Some(item) =>
    Array.map(
      (c: Drag.container) => {
        let remaining =
          Array.to_list(c.items)
          |> List.filter((i: Drag.item) => !String.equal(i.key, move.key));
        let rec insert = (index, items) =>
          switch (items) {
          | [] => [item]
          | [head, ...tail] =>
            index <= 0
              ? [item, ...items] : [head, ...insert(index - 1, tail)]
          };
        {
          ...c,
          items:
            Array.of_list(
              String.equal(c.key, move.to_container)
                ? insert(move.to_index, remaining) : remaining,
            ),
        };
      },
      containers,
    )
  };
};

let update = (msg, demos) => {
  let id =
    switch (msg) {
    | Drag(id, _)
    | Cancel(id) => id
    };
  let rec change = demos =>
    switch (demos) {
    | [] => ([], Cmd.none)
    | [demo, ...tail] when String.equal(demo.id, id) =>
      let (drag, command, out) =
        switch (msg) {
        | Drag(_, intent) =>
          Drag.update(~containers=demo.containers, intent, demo.drag)
        | Cancel(_) =>
          let (drag, command) = Drag.cancel(demo.drag);
          (drag, command, []);
        };
      let demo =
        List.fold_left(
          (demo, event) =>
            switch (event) {
            | Drag.Moved(move) => {
                ...demo,
                containers: apply_move(demo.containers, move),
                moves: demo.moves + 1,
              }
            | Drag.Drag_started(_)
            | Drag.Cancelled => demo
            },
          {
            ...demo,
            drag,
          },
          out,
        );
      ([demo, ...tail], Cmd.map(intent => Drag(id, intent), command));
    | [head, ...tail] =>
      let (tail, command) = change(tail);
      ([head, ...tail], command);
    };
  change(demos);
};

let demo =
    (
      ~id,
      ~title,
      ~note,
      ~containers,
      ~orientation=Drag.Vertical,
      ~dir=Drag.Ltr,
      ~drag_from_item=true,
      ~threshold=6.0,
      ~touch_delay=250,
      ~assertive=false,
      ~labels=Drag.default_labels,
      (),
    )
    : demo => {
  id,
  title,
  note,
  orientation,
  dir,
  containers,
  moves: 0,
  drag:
    Drag.init(
      ~id,
      ~orientation,
      ~dir,
      ~drag_from_item,
      ~threshold,
      ~touch_delay,
      ~assertive,
      ~labels,
      (),
    ),
};

let init = () => (
  [
    demo(
      ~id="board",
      ~title="Print studio board",
      ~note=
        "Drag a card or its handle. Move work between Queue and Making; the archive and the proof awaiting approval are locked.",
      ~containers=[|
        container(
          "queue",
          "Queue",
          [|
            item("poster", "Edition poster"),
            item("cover", "Book cover"),
            item(~disabled=true, "proof", "Proof awaiting approval"),
          |],
        ),
        container("making", "Making", [|item("labels", "Parcel labels")|]),
        container(
          ~disabled=true,
          "archive",
          "Archive (locked)",
          [|item("sample", "Reference sample")|],
        ),
      |],
      (),
    ),
    demo(
      ~id="packing",
      ~title="Packing order",
      ~drag_from_item=false,
      ~threshold=10.0,
      ~touch_delay=350,
      ~assertive=true,
      ~labels={
        ...Drag.default_labels,
        dropped: "Packing order saved.",
      },
      ~note=
        "Only the Move buttons start a drag. This list has a 10px pointer threshold, 350ms touch delay, assertive announcements and a custom drop message. Drag near its edge to scroll.",
      ~containers=[|
        container(
          "packing",
          "Packing order",
          [|
            item("tissue", "Tissue paper"),
            item("sleeve", "Print sleeve"),
            item("backing", "Backing board"),
            item("corners", "Corner protectors"),
            item("wrap", "Paper wrap"),
            item("seal", "Studio seal"),
            item("note", "Thank-you note"),
            item("invoice", "Invoice"),
            item("mailer", "Rigid mailer"),
            item("tape", "Paper tape"),
            item("address", "Address label"),
            item("stamp", "Postage stamp"),
          |],
        ),
      |],
      (),
    ),
    demo(
      ~id="horizontal",
      ~title="Shelf sequence",
      ~orientation=Drag.Horizontal,
      ~note="Horizontal: Left/Right reorder. Shift+Down/Up changes shelves.",
      ~containers=[|
        container(
          "upper",
          "Upper shelf",
          [|
            item("cyan", "Cyan ink"),
            item("magenta", "Magenta ink"),
            item("yellow", "Yellow ink"),
          |],
        ),
        container("lower", "Lower shelf", [|item("black", "Black ink")|]),
      |],
      (),
    ),
    demo(
      ~id="rtl",
      ~title="Right-to-left shelf",
      ~orientation=Drag.Horizontal,
      ~dir=Drag.Rtl,
      ~note=
        "RTL horizontal: Left moves forward, Right moves back. Shift+Down/Up changes shelves.",
      ~containers=[|
        container(
          "upper",
          "RTL upper shelf",
          [|
            item("red", "Red ink"),
            item("blue", "Blue ink"),
            item("green", "Green ink"),
          |],
        ),
        container(
          "lower",
          "RTL lower shelf",
          [|item("white", "White ink")|],
        ),
      |],
      (),
    ),
    demo(
      ~id="rtl-board",
      ~title="Right-to-left columns",
      ~dir=Drag.Rtl,
      ~note=
        "RTL vertical: Up/Down reorder. Shift+Left moves to the next column; Shift+Right moves back.",
      ~containers=[|
        container(
          "first",
          "First RTL column",
          [|item("etching", "Etching"), item("linocut", "Linocut")|],
        ),
        container(
          "second",
          "Second RTL column",
          [|item("screen", "Screen print")|],
        ),
      |],
      (),
    ),
  ],
  Cmd.none,
);

let view_demo = demo =>
  Drag.view(
    ~model=demo.drag,
    ~containers=demo.containers,
    ~to_parent=intent => Drag(demo.id, intent),
    ~to_view=
      parts =>
        Html.section(
          [
            Html.id(demo.id ++ "-example"),
            Html.aria("labelledby", demo.id ++ "-title"),
          ],
          [
            Html.h2(
              [Html.id(demo.id ++ "-title")],
              [Html.text(demo.title)],
            ),
            Html.p([Html.class_("note")], [Html.text(demo.note)]),
            Html.p(
              parts.instructions @ [Html.class_("instructions")],
              [Html.text(parts.instructions_text)],
            ),
            Html.Keyed.div(
              [
                Html.class_(
                  demo.orientation == Drag.Horizontal
                    ? "lists shelves" : "lists columns",
                ),
                Html.attribute("dir", demo.dir == Drag.Rtl ? "rtl" : "ltr"),
              ],
              Array.to_list(
                Array.map(
                  (c: Drag.container) =>
                    (
                      c.key,
                      Html.div(
                        [Html.class_("lane")],
                        [
                          Html.h3([], [Html.text(c.label)]),
                          Html.Keyed.ul(
                            parts.container(c.key)
                            @ [
                              Html.class_(
                                demo.id == "packing"
                                  ? "items scroll-area" : "items",
                              ),
                              Html.data("container", c.key),
                            ],
                            Array.to_list(
                              Array.map(
                                (item: Drag.item) =>
                                  (
                                    item.key,
                                    Html.li(
                                      parts.item(item.key)
                                      @ [Html.data("key", item.key)],
                                      [
                                        Html.span(
                                          [Html.class_("item-label")],
                                          [Html.text(item.label)],
                                        ),
                                        Html.button(
                                          parts.handle(item.key)
                                          @ [Html.class_("handle")],
                                          [Html.text("Move " ++ item.label)],
                                        ),
                                      ],
                                    ),
                                  ),
                                c.items,
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                  demo.containers,
                ),
              ),
            ),
            /* Preview is text outside the lists: no placeholder can change hit geometry. */
            Html.p(
              [
                Html.class_("preview"),
                Html.aria("hidden", "true"),
                Html.data("preview", demo.id),
              ],
              [
                Html.text(
                  switch (parts.preview) {
                  | None => "No move in progress."
                  | Some(target) =>
                    "Drop in "
                    ++ target.container
                    ++ " at position "
                    ++ string_of_int(target.index + 1)
                    ++ "."
                  },
                ),
              ],
            ),
            Html.div(
              parts.live @ [Html.class_("live")],
              [Html.text(parts.live_text)],
            ),
            Html.div(
              [Html.class_("actions")],
              [
                Html.button(
                  [
                    Html.id(demo.id ++ "-cancel"),
                    Html.type_("button"),
                    Html.on_click(Cancel(demo.id)),
                  ],
                  [Html.text("Cancel move")],
                ),
                Html.node(
                  "output",
                  [Html.id(demo.id ++ "-moves")],
                  [
                    Html.text(
                      "Committed moves: " ++ string_of_int(demo.moves),
                    ),
                  ],
                ),
              ],
            ),
          ],
        ),
    (),
  );

let view = demos =>
  Html.main(
    [],
    [
      Html.header(
        [],
        [
          Html.h1([], [Html.text("A place for every print.")]),
          Html.p(
            [],
            [
              Html.text(
                "Reorder the studio's work without changing it until you drop. Use a mouse, touch, or the keyboard; each list keeps its native list semantics.",
              ),
            ],
          ),
        ],
      ),
      Html.div([], List.map(view_demo, demos)),
    ],
  );
let subscriptions = demos =>
  Sub.batch(
    List.map(
      demo =>
        Sub.map(
          intent => Drag(demo.id, intent),
          Drag.subscriptions(demo.drag),
        ),
      demos,
    ),
  );
let () = {
  let app =
    App.element(
      ~init,
      ~update,
      ~view,
      ~subscriptions,
      ~node="#app",
      ~flags=(),
      (),
    );
  ignore(app);
};
