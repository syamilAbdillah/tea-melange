open Tea;
module H = Ui.Hover_intent;
type card = {
  key: string,
  label: string,
  detail: string,
  disabled: bool,
  grouped: bool,
  hover: H.model,
};
type model = {
  cards: list(card),
  group: H.Group.t,
  opens: int,
  closes: int,
};
type msg =
  | Hover(string, H.msg)
  | Show(string)
  | Hide(string);
let make =
    (~key, ~label, ~detail, ~disabled=false, ~grouped=false, ~delay=450, ()) => {
  key,
  label,
  detail,
  disabled,
  grouped,
  hover: H.init(~open_delay_ms=delay, ~close_delay_ms=300, ()),
};
let init = () => (
  {
    cards: [
      make(
        ~key="card",
        ~label="Mountain tea",
        ~detail="A cool, mineral cup from the high slopes.",
        (),
      ),
      make(
        ~key="instant",
        ~label="Evening blend",
        ~detail="No opening delay. The same generous leaving grace.",
        ~delay=0,
        (),
      ),
      make(
        ~key="disabled",
        ~label="Seasonal reserve",
        ~detail="Not available this season.",
        ~disabled=true,
        (),
      ),
      make(
        ~key="group-a",
        ~label="Steep",
        ~detail="Use water just below boiling.",
        ~grouped=true,
        ~delay=1200,
        (),
      ),
      make(
        ~key="group-b",
        ~label="Pour",
        ~detail="Pour slowly to release the aroma.",
        ~grouped=true,
        ~delay=1200,
        (),
      ),
    ],
    group: H.Group.init(~skip_delay_ms=1800, ()),
    opens: 0,
    closes: 0,
  },
  Cmd.none,
);
let update = (msg, model) => {
  let key =
    switch (msg) {
    | Hover(key, _)
    | Show(key)
    | Hide(key) => key
    };
  let cmd = ref(Cmd.none);
  let group = ref(model.group);
  let opens = ref(model.opens);
  let closes = ref(model.closes);
  let cards =
    List.map(
      card =>
        if (!String.equal(key, card.key)) {
          card;
        } else {
          let (hover, effects, out) =
            switch (msg) {
            | Hover(_, intent) =>
              let group = card.grouped ? Some(model.group) : None;
              H.update(~disabled=card.disabled, ~group?, intent, card.hover);
            | Show(_) =>
              let (hover, cmd) =
                card.disabled
                  ? H.close_now(card.hover) : H.open_now(card.hover);
              (hover, cmd, []);
            | Hide(_) =>
              let (hover, cmd) = H.close_now(card.hover);
              (hover, cmd, []);
            };
          cmd := Cmd.map(intent => Hover(key, intent), effects);
          List.iter(
            out =>
              switch (out) {
              | H.Intent_open => incr(opens)
              | H.Intent_close =>
                incr(closes);
                if (card.grouped) {
                  Option.iter(
                    time_stamp =>
                      group := H.Group.record_close(~time_stamp, group^),
                    H.last_close_time(hover),
                  );
                };
              },
            out,
          );
          {
            ...card,
            hover,
          };
        },
      model.cards,
    );
  (
    {
      cards,
      group: group^,
      opens: opens^,
      closes: closes^,
    },
    cmd^,
  );
};
let card_view = card =>
  H.view(
    ~model=card.hover,
    ~disabled=card.disabled,
    ~to_parent=intent => Hover(card.key, intent),
    ~to_view=
      parts => {
        let button =
          Ui.Button.parts(
            ~id=card.key,
            ~disabled=card.disabled,
            ~on_press=Show(card.key),
            ~on_key_down=
              key =>
                String.equal(key, "Escape") ? Some(Hide(card.key)) : None,
            (),
          );
        Html.div(
          [Html.class_("sample")],
          [
            Html.button(
              button.root
              @ parts.trigger_attrs
              @ [
                Html.aria("expanded", parts.is_open ? "true" : "false"),
                Html.aria("controls", card.key ++ "-content"),
                Html.on_focus(Show(card.key)),
                Html.on_blur(Hide(card.key)),
              ],
              [Html.text(card.label)],
            ),
            Html.section(
              parts.content_attrs
              @ [
                Html.id(card.key ++ "-content"),
                Html.aria("labelledby", card.key),
                Html.hidden(!parts.is_open),
                Html.class_("panel"),
              ],
              [Html.p([], [Html.text(card.detail)])],
            ),
          ],
        );
      },
    (),
  );
let view = model =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Give the pointer a moment.")]),
      Html.p(
        [],
        [
          Html.text(
            "A hover-intent study. Stay over a tea to open its card; cross the small gap to keep reading. Focus opens immediately. Escape or blur closes. The consumer supplies these keyboard and disclosure semantics.",
          ),
        ],
      ),
      Html.h2([], [Html.text("A little patience")]),
      Html.p(
        [],
        [
          Html.text(
            "Mountain tea waits 450 ms; Evening blend opens immediately. Both allow 300 ms to cross the gap. Seasonal reserve is disabled.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("samples")],
        List.map(card_view, List.filter(card => !card.grouped, model.cards)),
      ),
      Html.h2([], [Html.text("A warm group")]),
      Html.p(
        [],
        [
          Html.text(
            "The first action waits 1200 ms. After it closes, hover the other within 1800 ms to skip the wait. This is a group of native buttons, not an ARIA toolbar.",
          ),
        ],
      ),
      Html.div(
        [
          Html.role("group"),
          Html.aria("label", "Tea preparation"),
          Html.class_("samples"),
        ],
        List.map(card_view, List.filter(card => card.grouped, model.cards)),
      ),
      Html.p(
        [Html.id("activity")],
        [
          Html.text(
            "Pointer opens: "
            ++ string_of_int(model.opens)
            ++ ". Pointer closes: "
            ++ string_of_int(model.closes)
            ++ ".",
          ),
        ],
      ),
    ],
  );
let () =
  ignore(
    App.element(
      ~init,
      ~update,
      ~view,
      ~subscriptions=
        model =>
          Sub.batch(
            List.map(
              card =>
                Sub.map(
                  intent => Hover(card.key, intent),
                  H.subscriptions(~key=card.key, card.hover),
                ),
              model.cards,
            ),
          ),
      ~node="#app",
      ~flags=(),
      (),
    ),
  );
