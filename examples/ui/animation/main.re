open Tea;
module A = Ui.Animation;
type lane = {
  id: string,
  title: string,
  animation: A.model,
  desired: bool,
  entered: int,
  left: int,
  unmount: bool,
};
type msg =
  | Toggle(string)
  | Show(string)
  | Hide(string)
  | Animation(string, A.msg);
let make = (~open_=false, ~appear=false, ~unmount=false, id, title) => {
  id,
  title,
  desired: open_,
  entered: 0,
  left: 0,
  unmount,
  animation: A.init(~id=id ++ "-panel", ~open_, ~appear, ()),
};
let initial = [|
  make("transition", "CSS transition"),
  make("keyframe", "CSS keyframes"),
  make("none", "No animation CSS"),
  make(~open_=true, ~appear=true, "appear", "Animate initial mount"),
  make(~open_=true, "initial", "Initially open, without appear"),
  make(~unmount=true, "unmount", "Unmount after Left"),
|];
let phase_name = animation =>
  switch (A.phase(animation)) {
  | A.Closed => "Closed"
  | A.Entering => "Entering"
  | A.Open => "Open"
  | A.Leaving => "Leaving"
  };
let update = (message, lanes) => {
  let commands = ref([]);
  let lanes =
    Array.map(
      lane => {
        let id =
          switch (message) {
          | Toggle(id)
          | Show(id)
          | Hide(id)
          | Animation(id, _) => id
          };
        if (!String.equal(id, lane.id)) {
          lane;
        } else {
          let (animation, command, outs, desired) =
            switch (message) {
            | Animation(_, message) =>
              let (animation, command, outs) =
                A.update(message, lane.animation);
              (animation, command, outs, lane.desired);
            | Toggle(_)
            | Show(_)
            | Hide(_) =>
              let desired =
                switch (message) {
                | Show(_) => true
                | Hide(_) => false
                | _ => !lane.desired
                };
              let (animation, command) =
                desired ? A.show(lane.animation) : A.hide(lane.animation);
              (animation, command, [], desired);
            };
          commands :=
            [
              Cmd.map(message => Animation(id, message), command),
              ...commands^,
            ];
          List.fold_left(
            (lane, out) =>
              switch (out) {
              | A.Entered => {
                  ...lane,
                  entered: lane.entered + 1,
                }
              | A.Left => {
                  ...lane,
                  left: lane.left + 1,
                }
              },
            {
              ...lane,
              animation,
              desired,
            },
            outs,
          );
        };
      },
      lanes,
    );
  (lanes, Cmd.batch(commands^));
};
let view = lanes =>
  Html.main(
    [],
    [
      Html.h1([], [Html.text("Between open and closed")]),
      Html.p(
        [],
        [
          Html.text(
            "Each panel uses the same lifecycle. Open, close, or reverse it before it finishes. Your reduced-motion preference is respected.",
          ),
        ],
      ),
      Html.div(
        [Html.class_("lanes")],
        Array.to_list(
          Array.map(
            lane => {
              let id = lane.id;
              Html.section(
                [
                  Html.id(id),
                  Html.aria("labelledby", id ++ "-title"),
                  Html.data("phase", phase_name(lane.animation)),
                  Html.data("entered", string_of_int(lane.entered)),
                  Html.data("left", string_of_int(lane.left)),
                ],
                [
                  Html.h2(
                    [Html.id(id ++ "-title")],
                    [Html.text(lane.title)],
                  ),
                  Ui.Button.button(
                    ~id=id ++ "-toggle",
                    ~on_press=Toggle(id),
                    ~attrs=[
                      Html.aria("expanded", lane.desired ? "true" : "false"),
                      /* Unmounted closed content has no dangling controls idref. */
                      !lane.unmount || A.should_render(lane.animation)
                        ? Html.aria("controls", id ++ "-panel") : Html.none,
                    ],
                    [Html.text("Toggle " ++ lane.title)],
                  ),
                  Ui.Button.button(
                    ~id=id ++ "-show",
                    ~on_press=Show(id),
                    [Html.text("Show")],
                  ),
                  Ui.Button.button(
                    ~id=id ++ "-hide",
                    ~on_press=Hide(id),
                    [Html.text("Hide")],
                  ),
                  Html.p(
                    [Html.class_("phase"), Html.id(id ++ "-state")],
                    [
                      Html.text(
                        phase_name(lane.animation)
                        ++ " · Entered: "
                        ++ string_of_int(lane.entered)
                        ++ " · Left: "
                        ++ string_of_int(lane.left),
                      ),
                    ],
                  ),
                  !lane.unmount || A.should_render(lane.animation)
                    ? Html.div(
                        A.attrs(lane.animation)
                        @ [
                          Html.id(id ++ "-panel"),
                          Html.class_("panel " ++ id),
                          Html.role("region"),
                          Html.aria("label", lane.title ++ " content"),
                        ],
                        [
                          Html.p(
                            [],
                            [
                              Html.text(
                                "The panel stays mounted while leaving.",
                              ),
                            ],
                          ),
                          Ui.Button.button(
                            ~id=id ++ "-close",
                            ~on_press=Hide(id),
                            [Html.text("Close panel")],
                          ),
                        ],
                      )
                    : Html.text(""),
                ],
              );
            },
            lanes,
          ),
        ),
      ),
    ],
  );
let subscriptions = lanes =>
  Sub.batch(
    Array.to_list(
      Array.map(
        lane =>
          Sub.batch([
            Sub.map(
              message => Animation(lane.id, message),
              A.subscriptions(lane.animation),
            ),
            A.should_render(lane.animation)
              ? Browser_events.restore_focus(
                  ~key=lane.id,
                  ~within=lane.id ++ "-panel",
                  (),
                )
              : Sub.none,
          ]),
        lanes,
      ),
    ),
  );
let app =
  App.element(
    ~init=() => (initial, Cmd.none),
    ~update,
    ~view,
    ~subscriptions,
    ~node="#app",
    ~flags=(),
    (),
  );
