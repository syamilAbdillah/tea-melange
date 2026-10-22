open Tea_core;
module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Decode;
module Browser = Tea_browser.Browser_dom;
module Events = Tea_browser.Browser_events;
module Platform = Tea_dom.Browser_platform;
module B = {
  type timer;
  external timeout: (unit => unit, int) => timer = "setTimeout";
  external clear: timer => unit = "clearTimeout";
  [@mel.scope "performance"] external now: unit => float = "now";
  external document: Dom.document = "document";
  [@mel.get] external hidden: Dom.document => bool = "hidden";
  [@mel.send]
  external find: (Dom.document, string) => Js.Nullable.t(Dom.node) =
    "getElementById";
  [@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
  [@mel.get] external target: Dom.event => Js.Nullable.t(Dom.node) = "target";
  [@mel.get]
  external related: Dom.event => Js.Nullable.t(Dom.node) = "relatedTarget";
  [@mel.send]
  external listen: (Dom.document, string, Dom.event => unit, bool) => unit =
    "addEventListener";
  [@mel.send]
  external unlisten: (Dom.document, string, Dom.event => unit, bool) => unit =
    "removeEventListener";
  external raw: Dom.event => Tea_dom.Dom_decode.raw = "%identity";
  [@mel.send] external prevent: Dom.event => unit = "preventDefault";
  [@mel.send] external stop: Dom.event => unit = "stopPropagation";
};
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
  instance: string,
  epoch: string,
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
  serial: string,
  ready: bool,
  hovered: bool,
  focused: bool,
  focused_key: option(string),
  hidden: bool,
  browsing: bool,
  tabbing: bool,
};
type msg =
  | Ready(bool)
  | Hover(bool)
  | Focus(bool)
  | Focus_toast(string)
  | Tab
  | Hotkey
  | Visible(bool)
  | Close(string, reason)
  | Activate(string)
  | Elapsed(string, string, string)
  | Animate(string, string, Animation.msg);
let successor = value => {
  let digits = Bytes.of_string(value);
  let rec carry = i =>
    if (i < 0) {
      "1" ++ Bytes.to_string(digits);
    } else if (Bytes.get(digits, i) == '9') {
      Bytes.set(digits, i, '0');
      carry(i - 1);
    } else {
      Bytes.set(digits, i, Char.chr(Char.code(Bytes.get(digits, i)) + 1));
      Bytes.to_string(digits);
    };
  carry(Bytes.length(digits) - 1);
};
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
  serial: "0",
  ready: false,
  hovered: false,
  focused: false,
  focused_key: None,
  hidden: false,
  browsing: false,
  tabbing: false,
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
              epoch: successor(e.epoch),
            }
          | (Some(_), None, true) => {
              ...e,
              since: Some(now),
              epoch: successor(e.epoch),
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
    let candidate = "toast-" ++ serial;
    Option.is_some(find(model, candidate))
      ? fresh(successor(serial)) : (candidate, serial);
  };
  let (key, serial) =
    switch (key) {
    | Some(key) => (key, successor(model.serial))
    | None => fresh(successor(model.serial))
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
              epoch: successor(e.epoch),
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
          epoch: "0",
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
      B.now(),
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
            epoch: successor(e.epoch),
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
  let now = B.now();
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
        },
        Browser.focus_cmd(focus_target(model)),
        [],
      )
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
            String.equal(e.instance, instance)
            && String.equal(e.epoch, epoch)
            && Option.is_some(e.since)
            && !paused(model) =>
        let (model, command) = close(model, key, Timeout);
        (model, command, []);
      | _ => (model, Cmd.none, [])
      }
    | Animate(key, instance, message) =>
      switch (find(model, key)) {
      | Some(e) when String.equal(e.instance, instance) =>
        let (animation, command, outs) =
          Animation.update(message, e.animation);
        let left = List.exists(out => out == Animation.Left, outs);
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
            lost_focus
              ? Browser.focus_cmd(
                  model.browsing ? model.id : focus_target(next),
                )
              : Cmd.none,
          ]),
          outs,
        );
      | _ => (model, Cmd.none, [])
      }
    };
  (settle_timers(now, model), command, outs);
};
let rec scoped = (~key, sub) => {
  let scope = source =>
    string_of_int(String.length(key)) ++ ":" ++ key ++ source;
  switch (sub) {
  | Sub.None => Sub.none
  | Sub.Batch(subs) => Sub.batch(List.map(scoped(~key), subs))
  | Sub.Leaf(leaf) =>
    Sub.Leaf({
      ...leaf,
      key: scope(leaf.key),
    })
  | Sub.Filtered(leaf) =>
    Sub.Filtered({
      ...leaf,
      key: scope(leaf.key),
    })
  };
};
let subscriptions = model => {
  let contains = node =>
    switch (
      Js.Nullable.toOption(B.find(B.document, model.id)),
      Js.Nullable.toOption(node),
    ) {
    | (Some(root), Some(node)) => B.contains(root, node)
    | _ => false
    };
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
                      dispatch(B.hidden(B.document));
                    }
                  );
                }
              );
              () => live := false;
            },
        ),
    scoped(
      ~key=model.id,
      Events.on_visibility_change(visible => Visible(visible)),
    ),
    /* Browser_events keydown deliberately cannot cancel a browser default.
       This capture source uses the same decoder and current filter semantics,
       but consumes only the explicitly configured hotkey (including modifiers). */
    Sub.filter_map(
      raw =>
        switch (Tea_dom.Dom_decode.run(Ui_keyboard.decoder, B.raw(raw))) {
        | Ok(event) =>
          let h = model.hotkey;
          if (Ui_keyboard.available(event)
              && !event.repeat
              && String.equal(h.key, event.key)
              && h.ctrl == event.ctrl
              && h.alt == event.alt
              && h.meta == event.meta
              && h.shift == event.shift) {
            B.prevent(raw);
            B.stop(raw);
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
            B.listen(B.document, "keydown", dispatch, true);
            () => B.unlisten(B.document, "keydown", dispatch, true);
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
            if (contains(B.target(event)) && !contains(B.related(event))) {
              dispatch(true);
            };
          let leave = event =>
            if (contains(B.target(event)) && !contains(B.related(event))) {
              dispatch(false);
            };
          B.listen(B.document, "focusin", enter, true);
          B.listen(B.document, "focusout", leave, true);
          () => {
            B.unlisten(B.document, "focusin", enter, true);
            B.unlisten(B.document, "focusout", leave, true);
          };
        },
    ),
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
                  ++ e.instance
                  ++ ":"
                  ++ e.epoch,
                ~tagger=() => Elapsed(e.key, e.instance, e.epoch),
                ~start=
                  dispatch => {
                    let live = ref(true);
                    let timer =
                      B.timeout(
                        () =>
                          if (live^) {
                            live := false;
                            dispatch();
                          },
                        int_of_float(
                          ceil(
                            max(0., remaining -. max(0., B.now() -. since)),
                          ),
                        ),
                      );
                    () => {
                      live := false;
                      B.clear(timer);
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
        D.map(
          event =>
            {
              ...event,
              V.stop_propagation: event.V.prevent_default,
            },
          Ui_keyboard.custom(event =>
            if (enabled
                && String.equal(event.key, "Escape")
                && Ui_keyboard.plain(event)) {
              Some((Some(Close(key, Escape)), true));
            } else {
              None;
            }
          ),
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
                  Ui_keyboard.custom(event =>
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
                D.map(
                  event =>
                    {
                      ...event,
                      V.stop_propagation: event.V.prevent_default,
                    },
                  keyboard,
                );
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
