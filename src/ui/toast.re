open Tea_core;
module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Decode;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
module Platform = Tea_dom.Browser_platform;
type urgency =
  | Normal
  | Urgent;
type order =
  | Oldest_first
  | Newest_first;
type reason =
  | Timeout
  | Close_button
  | Action
  | Escape
  | Programmatic;
type out_msg =
  | Dismissed(string, reason)
  | Action(string);
type hotkey = {
  key: string,
  ctrl: bool,
  alt: bool,
  meta: bool,
  shift: bool,
};
type entry = {
  key: string,
  content_key: string,
  urgency,
  instance: int,
  epoch: int,
  animation: Animation.model,
  waiting: bool,
  remaining: option(float),
  since: option(float),
  reason: option(reason),
};
type model = {
  id: string,
  label: string,
  max_visible: int,
  order,
  hotkey,
  entries: list(entry),
  serial: int,
  ready: bool,
  hovered: bool,
  focused: bool,
  focused_key: option(string),
  hidden: bool,
  browsing: bool,
  tabbing: bool,
  focus_epoch: int,
  focus_pending: bool,
  repair_target: option(string),
};
type msg =
  | Ready(bool)
  | Hover(bool)
  | Focus(bool)
  | Focus_toast(string)
  | Tab
  | Hotkey
  | Hotkey_focused(int, bool)
  | Focus_repaired(int)
  | Visible(bool)
  | Close(string, reason)
  | Activate(string)
  | Elapsed(string, int, int)
  | Animate(string, int, Animation.msg);
let init =
    (
      ~id,
      ~label="Notifications",
      ~max_visible=3,
      ~order=Oldest_first,
      ~hotkey={
                key: "F8",
                ctrl: false,
                alt: false,
                meta: false,
                shift: false,
              },
      (),
    ) => {
  id,
  label,
  max_visible: max(1, max_visible),
  order,
  hotkey,
  entries: [],
  serial: 0,
  ready: false,
  hovered: false,
  focused: false,
  focused_key: None,
  hidden: false,
  browsing: false,
  tabbing: false,
  focus_epoch: 0,
  focus_pending: false,
  repair_target: None,
};
let paused = model => model.hovered || model.focused || model.hidden;
let visible_entries = model => {
  let entries = List.filter(e => !e.waiting, model.entries);
  switch (model.order) {
  | Oldest_first => entries
  | Newest_first => List.rev(entries)
  };
};
let keys = model => List.map(e => e.key, visible_entries(model));
let queued_keys = model =>
  List.filter_map(
    e => e.waiting && Option.is_none(e.reason) ? Some(e.key) : None,
    model.entries,
  );
let find = (model, key) =>
  List.find_opt(e => String.equal(e.key, key), model.entries);
let promote = model => {
  let count = List.length(List.filter(e => !e.waiting, model.entries));
  let (_, entries) =
    List.fold_left(
      ((count, entries), e) =>
        if (model.ready
            && e.waiting
            && Option.is_none(e.reason)
            && count < model.max_visible) {
          let (animation, _) = Animation.show(e.animation);
          (
            count + 1,
            [
              {
                ...e,
                waiting: false,
                animation,
              },
              ...entries,
            ],
          );
        } else {
          (count, [e, ...entries]);
        },
      (count, []),
      model.entries,
    );
  {
    ...model,
    entries: List.rev(entries),
  };
};
let settle_timers = (now, model) => {
  let pause = paused(model);
  {
    ...model,
    entries:
      List.map(
        e => {
          let runnable =
            !pause
            && !e.waiting
            && Option.is_none(e.reason)
            && Animation.phase(e.animation) == Animation.Open;
          switch (e.remaining, e.since, runnable) {
          | (Some(remaining), Some(since), false) => {
              ...e,
              remaining: Some(max(0., remaining -. max(0., now -. since))),
              since: None,
              epoch: e.epoch + 1,
            }
          | (Some(_), None, true) => {
              ...e,
              since: Some(now),
              epoch: e.epoch + 1,
            }
          | _ => e
          };
        },
        model.entries,
      ),
  };
};
let push =
    (
      model,
      ~key=?,
      ~duration=Some(5000),
      ~urgency,
      ~has_actions=false,
      content_key,
    ) => {
  let rec fresh = serial => {
    let candidate = "toast-" ++ string_of_int(serial);
    Option.is_some(find(model, candidate))
      ? fresh(serial + 1) : (candidate, serial);
  };
  let (key, serial) =
    switch (key) {
    | Some(key) => (key, model.serial + 1)
    | None => fresh(model.serial + 1)
    };
  let remaining =
    has_actions
      ? None : Option.map(ms => float_of_int(max(5000, ms)), duration);
  let entries =
    switch (find(model, key)) {
    | Some(old) =>
      List.map(
        e =>
          String.equal(e.key, key)
            ? {
              ...e,
              content_key,
              urgency: e.waiting ? urgency : e.urgency,
              remaining,
              since: None,
              reason: None,
              epoch: e.epoch + 1,
              animation:
                Option.is_some(old.reason) && !e.waiting
                  ? fst(Animation.show(e.animation)) : e.animation,
            }
            : e,
        model.entries,
      )
    | None =>
      model.entries
      @ [
        {
          key,
          content_key,
          urgency,
          instance: serial,
          epoch: 0,
          animation:
            Animation.init(
              ~id=Ui_attr.item_id(model.id, key),
              ~open_=false,
              (),
            ),
          waiting: true,
          remaining,
          since: None,
          reason: None,
        },
      ]
    };
  (
    settle_timers(
      Ui_dom.now(),
      promote({
        ...model,
        entries,
        serial,
      }),
    ),
    Cmd.none,
  );
};
let close = (model, key, reason) => {
  let entries =
    List.map(
      e =>
        if (String.equal(e.key, key) && Option.is_none(e.reason)) {
          /* Queued cancellations also complete through Left, without mounting or
             consuming a visible slot. A missing element completes Animation. */
          let animation =
            e.waiting
              ? Animation.init(
                  ~id=Ui_attr.item_id(model.id, key),
                  ~open_=true,
                  (),
                )
              : e.animation;
          let (animation, _) = Animation.hide(animation);
          {
            ...e,
            animation,
            reason: Some(reason),
            since: None,
            epoch: e.epoch + 1,
          };
        } else {
          e;
        },
      model.entries,
    );
  (
    {
      ...model,
      entries,
    },
    Cmd.none,
  );
};
let dismiss = (model, key) => close(model, key, Programmatic);
let dismiss_all = model => (
  List.fold_left(
    (model, e) => fst(dismiss(model, e.key)),
    model,
    model.entries,
  ),
  Cmd.none,
);
let focus_target = model => {
  let candidates =
    List.filter(e => !e.waiting && Option.is_none(e.reason), model.entries);
  switch (List.rev(candidates)) {
  | [e, ..._] => Ui_attr.item_id(model.id, e.key)
  | [] => model.id
  };
};
let update = (message, model) => {
  let now = Ui_dom.now();
  let (model, command, outs) =
    switch (message) {
    | Ready(hidden) => (
        promote({
          ...model,
          ready: true,
          hidden,
        }),
        Cmd.none,
        [],
      )
    | Hover(hovered) => (
        {
          ...model,
          hovered,
        },
        Cmd.none,
        [],
      )
    | Visible(visible) => (
        {
          ...model,
          hidden: !visible,
        },
        Cmd.none,
        [],
      )
    | Focus(true) => (
        {
          ...model,
          focused: true,
        },
        Cmd.none,
        [],
      )
    | Focus(false) => (
        {
          ...model,
          focused: false,
          focused_key: None,
          browsing: false,
          tabbing: false,
        },
        /* Commands are scheduled before restore_focus's stop callback. On a
           native Tab-out, temporarily focus the region AFTER the browser moves,
           then let restore_focus return to the saved element. Pointer departures
           never take this path and never have their focus stolen. */
        model.browsing && model.tabbing
          ? Browser.focus_cmd(model.id) : Cmd.none,
        [],
      )
    | Focus_toast(key) => (
        {
          ...model,
          focused: true,
          focused_key: Some(key),
          tabbing: false,
        },
        Cmd.none,
        [],
      )
    | Tab => (
        {
          ...model,
          tabbing: true,
        },
        Cmd.none,
        [],
      )
    | Hotkey => (
        {
          ...model,
          browsing: true,
          tabbing: false,
          focus_epoch: model.focus_epoch + 1,
          focus_pending: true,
          repair_target: None,
        },
        Cmd.none,
        [],
      )
    | Hotkey_focused(epoch, focused) when epoch == model.focus_epoch => (
        {
          ...model,
          focus_pending: false,
          browsing: focused && model.browsing,
        },
        Cmd.none,
        [],
      )
    | Hotkey_focused(_, _) => (model, Cmd.none, [])
    | Focus_repaired(epoch) when epoch == model.focus_epoch => (
        {
          ...model,
          repair_target: None,
        },
        Cmd.none,
        [],
      )
    | Focus_repaired(_) => (model, Cmd.none, [])
    | Close(key, reason) =>
      let (next, command) = close(model, key, reason);
      let returning = reason == Escape && model.browsing;
      (
        {
          ...next,
          browsing: returning ? false : next.browsing,
        },
        Cmd.batch([
          command,
          returning ? Browser.focus_cmd(model.id) : Cmd.none,
        ]),
        [],
      );
    | Activate(key) =>
      switch (find(model, key)) {
      | Some(e)
          when
            !e.waiting
            && Option.is_none(e.reason)
            && Option.is_none(e.remaining) =>
        let (model, command) = close(model, key, Action);
        (model, command, [Action(key)]);
      | _ => (model, Cmd.none, [])
      }
    | Elapsed(key, instance, epoch) =>
      switch (find(model, key)) {
      | Some(e)
          when
            e.instance == instance
            && e.epoch == epoch
            && Option.is_some(e.since)
            && !paused(model) =>
        let (model, command) = close(model, key, Timeout);
        (model, command, []);
      | _ => (model, Cmd.none, [])
      }
    | Animate(key, instance, message) =>
      switch (find(model, key)) {
      | Some(e) when e.instance == instance =>
        let (animation, command, outs) =
          Animation.update(message, e.animation);
        let left = List.mem(Animation.Left, outs);
        let entries =
          left
            ? List.filter(e => !String.equal(e.key, key), model.entries)
            : List.map(
                e =>
                  String.equal(e.key, key)
                    ? {
                      ...e,
                      animation,
                    }
                    : e,
                model.entries,
              );
        let next =
          promote({
            ...model,
            entries,
          });
        let lost_focus = left && model.focused_key == Some(key);
        let next =
          lost_focus
            ? {
              ...next,
              browsing: false,
              focused_key: None,
              focus_epoch: model.focus_epoch + 1,
              repair_target:
                model.browsing ? None : Some(focus_target(next)),
            }
            : next;
        let outs =
          left
            ? switch (e.reason) {
              | Some(reason) => [Dismissed(key, reason)]
              | None => []
              }
            : [];
        (
          next,
          Cmd.batch([
            Cmd.map(message => Animate(key, instance, message), command),
            /* A browsing dismissal must place focus inside the region before
               the opener restoration stopper runs. Ordinary repair can use the
               cancellable focus subscription instead. */
            lost_focus && model.browsing
              ? Browser.focus_cmd(model.id) : Cmd.none,
          ]),
          outs,
        );
      | _ => (model, Cmd.none, [])
      }
    };
  (settle_timers(now, model), command, outs);
};
let subscriptions = model => {
  let contains = Ui_dom.within(model.id);
  Sub.batch([
    model.ready
      ? Sub.none
      : Sub.make(
          ~home="Tea_ui.Toast.mount",
          ~key=model.id,
          ~tagger=hidden => Ready(hidden),
          ~start=
            dispatch => {
              let live = ref(true);
              Platform.request_animation_frame(_ =>
                if (live^) {
                  Platform.request_animation_frame(_ =>
                    if (live^) {
                      dispatch(Ui_dom.hidden(Ui_dom.document));
                    }
                  );
                }
              );
              () => live := false;
            },
        ),
    Ui_sub.scoped(
      ~key=model.id,
      Events.on_visibility_change(visible => Visible(visible)),
    ),
    /* Browser_events keydown deliberately cannot cancel a browser default.
       This capture source uses the same decoder and current filter semantics,
       but consumes only the explicitly configured hotkey (including modifiers). */
    Sub.filter_map(
      raw =>
        switch (Tea_dom.Dom_decode.run(Ui_keyboard.decoder, Ui_dom.raw(raw))) {
        | Ok(event) =>
          let h = model.hotkey;
          if (Ui_keyboard.available(event)
              && !event.repeat
              && String.equal(h.key, event.key)
              && h.ctrl == event.ctrl
              && h.alt == event.alt
              && h.meta == event.meta
              && h.shift == event.shift) {
            Ui_dom.prevent(raw);
            Ui_dom.stop(raw);
            Some(Hotkey);
          } else {
            None;
          };
        | Error(_) => None
        },
      Sub.make(
        ~home="Tea_ui.Toast.hotkey",
        ~key=model.id,
        ~tagger=event => event,
        ~start=
          dispatch => {
            Ui_dom.on(~capture=true, Ui_dom.document, "keydown", dispatch)
          },
      ),
    ),
    Sub.make(
      ~home="Tea_ui.Toast.focus",
      ~key=model.id,
      ~tagger=inside => Focus(inside),
      ~start=
        dispatch => {
          let enter = event =>
            if (contains(Ui_dom.target(event))
                && !contains(Ui_dom.related(event))) {
              dispatch(true);
            };
          let leave = event =>
            if (contains(Ui_dom.target(event))
                && !contains(Ui_dom.related(event))) {
              dispatch(false);
            };
          let off_in =
            Ui_dom.on(~capture=true, Ui_dom.document, "focusin", enter);
          let off_out =
            Ui_dom.on(~capture=true, Ui_dom.document, "focusout", leave);
          () => {
            off_in();
            off_out();
          };
        },
    ),
    model.focus_pending && model.browsing
      ? Events.focus(
          ~key=
            "Toast:" ++ model.id ++ ":" ++ string_of_int(model.focus_epoch),
          ~target=Events.Element(focus_target(model)),
          ~prevent_scroll=false,
          focused =>
          Hotkey_focused(model.focus_epoch, focused)
        )
      : Sub.none,
    switch (model.repair_target) {
    | Some(id) =>
      Events.focus(
        ~key=
          "Toast.repair:"
          ++ model.id
          ++ ":"
          ++ string_of_int(model.focus_epoch),
        ~target=Events.Element(id),
        ~from=[model.id],
        ~prevent_scroll=false,
        _ =>
        Focus_repaired(model.focus_epoch)
      )
    | None => Sub.none
    },
    model.browsing
      ? Events.restore_focus(
          ~key="Tea_ui.Toast:" ++ model.id,
          ~within=model.id,
          (),
        )
      : Sub.none,
    Sub.batch(
      List.map(
        e =>
          Sub.batch([
            Sub.map(
              message => Animate(e.key, e.instance, message),
              Animation.subscriptions(e.animation),
            ),
            switch (e.remaining, e.since) {
            | (Some(remaining), Some(since)) =>
              Sub.make(
                ~home="Tea_ui.Toast.timer",
                ~key=
                  Ui_attr.item_id(model.id, e.key)
                  ++ ":"
                  ++ string_of_int(e.instance)
                  ++ ":"
                  ++ string_of_int(e.epoch),
                ~tagger=() => Elapsed(e.key, e.instance, e.epoch),
                ~start=
                  dispatch => {
                    let live = ref(true);
                    let timer =
                      Ui_dom.timeout(
                        () =>
                          if (live^) {
                            live := false;
                            dispatch();
                          },
                        int_of_float(
                          ceil(
                            max(
                              0.,
                              remaining -. max(0., Ui_dom.now() -. since),
                            ),
                          ),
                        ),
                      );
                    () => {
                      live := false;
                      Ui_dom.clear_timeout(timer);
                    };
                  },
              )
            | _ => Sub.none
            },
          ]),
        model.entries,
      ),
    ),
  ]);
};
type toast_parts('parent) = {
  key: string,
  content_key: string,
  urgency,
  phase: Animation.phase,
  index: int,
  persistent: bool,
  root: list(Html.attr('parent)),
  title: list(Html.attr('parent)),
  description: list(Html.attr('parent)),
  close_button: list(Html.attr('parent)),
  action_attrs:
    (~key: string, ~alt_text: string, unit) => list(Html.attr('parent)),
};
type parts('parent) = {
  region: list(Html.attr('parent)),
  polite: list(Html.attr('parent)),
  assertive: list(Html.attr('parent)),
  toasts: list(toast_parts('parent)),
  paused: bool,
  queued: int,
};
let view = (~model, ~to_parent, ~on_key_down=_ => None, ~to_view, ()) => {
  let map = attrs => List.map(V.map_fact(to_parent), attrs);
  let keydown = (key, enabled) =>
    Html.on(
      "keydown",
      V.Custom_optional(
        Ui_keyboard.claiming(event =>
          if (enabled
              && String.equal(event.key, "Escape")
              && Ui_keyboard.plain(event)) {
            Some((Some(Close(key, Escape)), true));
          } else {
            None;
          }
        ),
      ),
    );
  let toasts =
    List.mapi(
      (index, e: entry) => {
        let id = Ui_attr.item_id(model.id, e.key);
        let leaving = Option.is_some(e.reason);
        {
          key: e.key,
          content_key: e.content_key,
          urgency: e.urgency,
          phase: Animation.phase(e.animation),
          index,
          persistent: Option.is_none(e.remaining),
          root:
            Animation.attrs(e.animation)
            @ map([
                Html.id(id),
                Html.role("group"),
                Html.tabindex(-1),
                Html.aria("labelledby", Ui_attr.part_id(id, "title")),
                Html.aria("describedby", Ui_attr.part_id(id, "description")),
                Html.data("index", string_of_int(index)),
                Html.style("--tea-toast-index", string_of_int(index)),
                Html.data(
                  "urgency",
                  e.urgency == Urgent ? "urgent" : "normal",
                ),
                Ui_attr.data_flag("paused", paused(model)),
                Html.on(
                  "focusin",
                  V.Normal(D.succeed(Focus_toast(e.key))),
                ),
                keydown(e.key, !leaving),
              ]),
          title: [Html.id(Ui_attr.part_id(id, "title"))],
          description: [Html.id(Ui_attr.part_id(id, "description"))],
          close_button:
            Button.parts(
              ~id=Ui_attr.part_id(id, "close"),
              ~disabled=leaving,
              ~on_press=to_parent(Close(e.key, Close_button)),
              (),
            ).
              root
            @ [Html.aria("label", "Dismiss notification")],
          action_attrs: (~key, ~alt_text, ()) =>
            Button.parts(
              ~id=Ui_attr.part_id(id, "action-" ++ Ui_attr.safe_key(key)),
              ~disabled=
                leaving
                || Option.is_some(e.remaining)
                || String.equal(String.trim(alt_text), ""),
              ~on_press=to_parent(Activate(e.key)),
              (),
            ).
              root
            @ [Html.aria("description", alt_text)],
        };
      },
      visible_entries(model),
    );
  to_view({
    region:
      map([
        Html.id(model.id),
        Html.role("region"),
        Html.aria("label", model.label),
        Html.tabindex(-1),
        Ui_attr.data_flag("paused", paused(model)),
        Html.on(
          "pointerenter",
          V.Normal(
            D.and_then(
              kind =>
                String.equal(kind, "touch")
                  ? D.fail("Touch does not hover") : D.succeed(Hover(true)),
              D.field("pointerType", D.string),
            ),
          ),
        ),
        Html.on("pointerleave", V.Normal(D.succeed(Hover(false)))),
        Html.on("pointercancel", V.Normal(D.succeed(Hover(false)))),
      ])
      @ [
        /* Tab bubbles from toast roots once. Only the region itself may claim
           Escape here; a leaving toast must leave it to an enclosing overlay. */
        Html.on(
          "keydown",
          V.Custom_optional(
            D.and_then(
              target_id => {
                let keyboard =
                  Ui_keyboard.claiming(event =>
                    if (String.equal(event.key, "Tab")
                        && Ui_keyboard.plain(event)) {
                      Some((Some(to_parent(Tab)), false));
                    } else if (String.equal(target_id, model.id)
                               && model.browsing
                               && String.equal(event.key, "Escape")
                               && Ui_keyboard.plain(event)) {
                      Some((Some(to_parent(Focus(false))), true));
                    } else if (Ui_keyboard.plain(event)) {
                      Option.map(
                        message => (Some(message), false),
                        on_key_down(event.key),
                      );
                    } else {
                      None;
                    }
                  );
                keyboard;
              },
              D.one_of([D.at(["target", "id"], D.string), D.succeed("")]),
            ),
          ),
        ),
      ],
    polite: [
      Html.id(Ui_attr.part_id(model.id, "polite")),
      Html.role("status"),
      Html.aria("live", "polite"),
      Html.aria("atomic", "false"),
      Html.aria("relevant", "additions text"),
    ],
    assertive: [
      Html.id(Ui_attr.part_id(model.id, "assertive")),
      Html.role("alert"),
      Html.aria("live", "assertive"),
      Html.aria("atomic", "false"),
      Html.aria("relevant", "additions text"),
    ],
    toasts,
    paused: paused(model),
    queued: List.length(queued_keys(model)),
  });
};
