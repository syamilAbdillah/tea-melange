open Tea_core;
module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Decode;
module Events = Tea_browser.Browser_events;

/* Private platform reads: geometry at event time, and focus verification.
   No raw JS or browser-wide listeners are necessary. */
module Browser = {
  external document: Dom.document = "document";
  [@mel.send]
  external find: (Dom.document, string) => Js.Nullable.t(Dom.node) =
    "getElementById";
  [@mel.get]
  external active: Dom.document => Js.Nullable.t(Dom.node) = "activeElement";
  [@mel.send] external contains: (Dom.node, Dom.node) => bool = "contains";
  [@mel.send]
  external rect:
    Dom.node =>
    {
      .
      "left": float,
      "right": float,
      "top": float,
      "bottom": float,
    } =
    "getBoundingClientRect";
  external window: Dom.window = "window";
  [@mel.get] external body: Dom.document => Dom.node = "body";
  [@mel.get]
  external parent: Dom.node => Js.Nullable.t(Dom.node) = "parentElement";
  [@mel.get] external tab_index: Dom.node => int = "tabIndex";
  [@mel.get] external tag: Dom.node => string = "localName";
  [@mel.get] external checked: Dom.node => bool = "checked";
  [@mel.get] external form: Dom.node => Js.Nullable.t(Dom.node) = "form";
  [@mel.get] external name: Dom.node => string = "name";
  [@mel.send] external matches: (Dom.node, string) => bool = "matches";
  [@mel.send]
  external query_all: (Dom.node, string) => array(Dom.node) =
    "querySelectorAll";
  [@mel.send] external focus: Dom.node => unit = "focus";
  [@mel.send]
  external style:
    (Dom.window, Dom.node) =>
    {
      .
      "display": string,
      "visibility": string,
    } =
    "getComputedStyle";
  let rec visible = node => {
    let css = style(window, node);
    !matches(node, "[hidden],[inert]")
    && !String.equal(css##display, "none")
    && !String.equal(css##visibility, "hidden")
    && !String.equal(css##visibility, "collapse")
    && (
      switch (Js.Nullable.toOption(parent(node))) {
      | None => true
      | Some(parent) =>
        (
          !matches(parent, "details:not([open])")
          || String.equal(tag(node), "summary")
        )
        && visible(parent)
      }
    );
  };
  [@mel.send]
  external focus_without_scroll:
    (Dom.node, {. "preventScroll": bool }) => unit =
    "focus";
  /* Sequential tab order: positive tabindex ascending, then tabindex 0 in DOM
     order. [backwards] picks the last stop instead of the first. */
  let tabbable = (~backwards, root) => {
    let radios = query_all(root, "input[type=radio]");
    let chosen = ref(None);
    Array.iter(
      node => {
        let tab = tab_index(node);
        let radio =
          !matches(node, "input[type=radio]")
          || String.equal(name(node), "")
          || !
               Array.exists(
                 other =>
                   other !== node
                   && String.equal(name(other), name(node))
                   && form(other) === form(node)
                   && checked(other)
                   && !matches(other, ":disabled"),
                 radios,
               );
        if (tab >= 0
            && !matches(node, ":disabled,input[type=hidden]")
            && radio
            && visible(node)) {
          let priority = tab > 0 ? tab : max_int;
          switch (chosen^) {
          | Some((_, previous))
              when backwards ? previous > priority : previous <= priority =>
            ()
          | _ => chosen := Some((node, priority))
          };
        };
      },
      query_all(
        root,
        "a[href],area[href],button,input,select,textarea,iframe,summary,[tabindex],[contenteditable]:not([contenteditable=false]),audio[controls],video[controls]",
      ),
    );
    Option.map(((node, _)) => node, chosen^);
  };
  let last_tabbable = root =>
    Option.value(~default=root, tabbable(~backwards=true, root));
  let repair_tab = (id, backwards, live) =>
    Tea_dom.Browser_platform.request_animation_frame(_ =>
      switch (
        Js.Nullable.toOption(find(document, id)),
        Js.Nullable.toOption(active(document)),
      ) {
      | (Some(root), Some(active))
          when
            live^
            && matches(root, "[open]:not([hidden])")
            && active === body(document) =>
        /* Chromium permits viewport focus at native modal Tab boundaries.
           Repair AFTER default navigation; never cancel Tab or steal focus
           from another connected element. */
        if (backwards) {
          focus(last_tabbable(root));
        } else {
          focus(
            Option.value(~default=root, tabbable(~backwards=false, root)),
          );
        }
      | _ => ()
      }
    );
  let outside = (id, target, x, y) =>
    String.equal(id, target)
    && (
      switch (Js.Nullable.toOption(find(document, id))) {
      | None => false
      | Some(node) =>
        let r = rect(node);
        x < r##left || x > r##right || y < r##top || y > r##bottom;
      }
    );
  let inside = (root, id) =>
    switch (
      Js.Nullable.toOption(find(document, root)),
      Js.Nullable.toOption(find(document, id)),
    ) {
    | (Some(root), Some(node)) => contains(root, node)
    | _ => false
    };
  let focused = id =>
    switch (
      Js.Nullable.toOption(find(document, id)),
      Js.Nullable.toOption(active(document)),
    ) {
    | (Some(node), Some(active)) => node === active
    | _ => false
    };
};
module Dom = Tea_browser.Browser_dom;
type kind =
  | Modal
  | Alertdialog(string);
type name =
  | Title
  | Label(string);
type labels = {close: string};
let default_labels = { close: "Close" };
type reason =
  | Escape
  | Backdrop
  | Close_button
  | Native;
type out_msg =
  | Opened
  | Close_requested(reason)
  | Closed;
type msg =
  | Animation(Animation.msg)
  | Ready
  | Request(reason)
  | Native_closed(string, bool)
  | Submit(string)
  | Pointer_down(bool)
  | Pointer_up(bool)
  | Pointer_cancel
  | Click(bool)
  | Tab(bool);
type model = {
  id: string,
  kind,
  initial_focus: option(string),
  dismissible: bool,
  close_on_backdrop: bool,
  animation: Animation.model,
  pointer_down: bool,
  pointer_up: bool,
  ready: bool,
  tab: option((bool, bool)),
  native_repair: option(bool),
  return_value: string,
};
let root_id = model => Ui_attr.part_id(model.id, "dialog");
let init =
    (
      ~id,
      ~kind=Modal,
      ~initial_focus=?,
      ~dismissible=true,
      ~close_on_backdrop=?,
      (),
    ) => {
  id,
  kind,
  initial_focus,
  dismissible,
  close_on_backdrop:
    Option.value(
      ~default=
        switch (kind) {
        | Modal => true
        | Alertdialog(_) => false
        },
      close_on_backdrop,
    ),
  animation:
    Animation.init(~id=Ui_attr.part_id(id, "dialog"), ~open_=false, ()),
  pointer_down: false,
  pointer_up: false,
  ready: false,
  tab: None,
  native_repair: None,
  return_value: "",
};
let phase = model => Animation.phase(model.animation);
let is_visible = model => Animation.is_visible(model.animation);
let is_open = model =>
  switch (phase(model)) {
  | Animation.Entering
  | Animation.Open => true
  | _ => false
  };
let return_value = model => model.return_value;
let open_ = model => {
  let (animation, command) = Animation.show(model.animation);
  (
    {
      ...model,
      animation,
      return_value: is_visible(model) ? model.return_value : "",
      native_repair: is_visible(model) ? model.native_repair : None,
      tab: is_visible(model) ? model.tab : None,
    },
    Cmd.map(message => Animation(message), command),
  );
};
let close = model => {
  let (animation, command) = Animation.hide(model.animation);
  (
    {
      ...model,
      animation,
      pointer_down: false,
      pointer_up: false,
    },
    Cmd.map(message => Animation(message), command),
  );
};
let request = (reason, model) =>
  if (!is_open(model)) {
    (model, Cmd.none, []);
  } else {
    let (model, command) =
      model.dismissible ? close(model) : (model, Cmd.none);
    (model, command, [Close_requested(reason)]);
  };
let update = (message, model) =>
  switch (message) {
  | Animation(message) =>
    let (animation, command, outs) =
      Animation.update(message, model.animation);
    let left =
      List.exists(
        out =>
          switch (out) {
          | Animation.Left => true
          | _ => false
          },
        outs,
      );
    (
      {
        ...model,
        animation,
        ready: left ? false : model.ready,
      },
      Cmd.map(message => Animation(message), command),
      left ? [Closed] : [],
    );
  | Ready when is_visible(model) && !model.ready => (
      {
        ...model,
        ready: true,
      },
      Cmd.none,
      [Opened],
    )
  | Ready => (model, Cmd.none, [])
  | Tab(backwards) => (
      {
        ...model,
        tab:
          Some((
            backwards,
            !
              Option.value(
                ~default=false,
                Option.map(((_, token)) => token, model.tab),
              ),
          )),
      },
      Cmd.none,
      [],
    )
  | Request(reason) => request(reason, model)
  | Submit(value) =>
    request(
      Native,
      {
        ...model,
        return_value: value,
      },
    )
  | Native_closed(value, false) when is_visible(model) =>
    /* An imperative close cannot be cancelled. Repair native modality before
       continuing the model's guard/leave policy. Stale queued close events see
       open=true and are ignored. Prefer method=dialog forms (intercepted below). */
    request(
      Native,
      {
        ...model,
        return_value: value,
        native_repair:
          Some(!Option.value(~default=false, model.native_repair)),
      },
    )
  | Native_closed(_, _) => (model, Cmd.none, [])
  | Pointer_down(outside) => (
      {
        ...model,
        pointer_down: outside,
        pointer_up: false,
      },
      Cmd.none,
      [],
    )
  | Pointer_up(outside) => (
      {
        ...model,
        pointer_up: outside,
      },
      Cmd.none,
      [],
    )
  | Pointer_cancel => (
      {
        ...model,
        pointer_down: false,
        pointer_up: false,
      },
      Cmd.none,
      [],
    )
  | Click(outside) =>
    let dismiss =
      outside
      && model.pointer_down
      && model.pointer_up
      && model.close_on_backdrop;
    let model = {
      ...model,
      pointer_down: false,
      pointer_up: false,
    };
    dismiss ? request(Backdrop, model) : (model, Cmd.none, []);
  };
/* Runs synchronously in the same frame as showModal(). Deferring to a later
   frame let a Tab pressed in between move focus, which this then stole back. */
let initial_focus = (model, live) =>
  if (live^) {
    let id = root_id(model);
    switch (Js.Nullable.toOption(Browser.find(Browser.document, id))) {
    | None => ()
    | Some(root) =>
      let focus = node =>
        Browser.focus_without_scroll(node, { "preventScroll": true });
      let fallback = () =>
        focus(
          Option.value(
            ~default=root,
            Browser.tabbable(~backwards=false, root),
          ),
        );
      let target =
        switch (model.kind) {
        | Alertdialog(id) => Some(id)
        | Modal => model.initial_focus
        };
      switch (target) {
      | Some(target) when Browser.inside(id, target) =>
        switch (Js.Nullable.toOption(Browser.find(Browser.document, target))) {
        | Some(node) =>
          focus(node);
          if (!Browser.focused(target)) {
            fallback();
          };
        | None => fallback()
        }
      | _ => fallback()
      };
    };
  };
let subscriptions = model => {
  let id = root_id(model);
  Sub.batch([
    /* First in the list so native close is scheduled before focus restoration. */
    is_visible(model)
      ? Sub.make(
          ~home="Tea_ui.Dialog.native",
          ~key=id,
          ~tagger=message => message,
          ~start=
            dispatch => {
              let live = ref(true);
              Dom.show_modal(id, result =>
                if (live^) {
                  switch (result) {
                  | Ok () =>
                    dispatch(Ready);
                    initial_focus(model, live);
                  | Error(_) => ()
                  };
                }
              );
              () => {
                live := false;
                Dom.close(id, _ => ());
              };
            },
        )
      : Sub.none,
    switch (model.native_repair) {
    | Some(token) when is_visible(model) =>
      Sub.make(
        ~home="Tea_ui.Dialog.repair",
        ~key=id ++ (token ? ":1" : ":0"),
        ~tagger=message => message,
        ~start=
          _ => {
            let live = ref(true);
            Dom.show_modal(id, result =>
              if (live^) {
                switch (result) {
                | Ok () => initial_focus(model, live)
                | Error(_) => ()
                };
              }
            );
            () => live := false;
          },
      )
    | _ => Sub.none
    },
    switch (model.tab) {
    | Some((backwards, token)) when is_visible(model) =>
      Sub.make(
        ~home="Tea_ui.Dialog.tab",
        ~key=id ++ (token ? ":1" : ":0"),
        ~tagger=message => message,
        ~start=
          _ => {
            let live = ref(true);
            Browser.repair_tab(id, backwards, live);
            () => live := false;
          },
      )
    | _ => Sub.none
    },
    Sub.map(
      message => Animation(message),
      Animation.subscriptions(model.animation),
    ),
    is_visible(model) ? Events.scroll_lock(~key=id) : Sub.none,
    is_visible(model)
      ? Events.restore_focus(~key=id, ~within=id, ()) : Sub.none,
  ]);
};
type parts('parent) = {
  root: list(Html.attr('parent)),
  title: list(Html.attr('parent)),
  description: list(Html.attr('parent)),
  close_button: list(Html.attr('parent)),
  phase: Animation.phase,
  is_open: bool,
  is_visible: bool,
  return_value: string,
};
let view =
    (
      ~model,
      ~name=Title,
      ~has_description=false,
      ~labels=default_labels,
      ~on_key_down=_ => None,
      ~to_parent,
      ~to_view,
      (),
    ) => {
  let id = root_id(model);
  let point = constructor =>
    D.map3(
      (target, x, y) => constructor(Browser.outside(id, target, x, y)),
      D.one_of([D.at(["target", "id"], D.string), D.succeed("")]),
      D.field("clientX", D.float),
      D.field("clientY", D.float),
    );
  let keyboard =
    Ui_keyboard.custom(event =>
      !Ui_keyboard.plain(event)
        ? None
        : String.equal(event.key, "Escape") && is_visible(model)
            ? Some((Some(to_parent(Request(Escape))), true))
            : String.equal(event.key, "Tab")
                ? Some((Some(to_parent(Tab(event.shift))), false))
                : Option.map(
                    message => (Some(message), false),
                    on_key_down(event.key),
                  )
    );
  let own = attributes => List.map(V.map_fact(to_parent), attributes);
  to_view({
    root:
      Animation.attrs(model.animation)
      @ [
        Html.id(id),
        Html.tabindex(-1),
        /* Never let a native close watcher bypass the model's guard. Plain
           Escape is handled by keydown; cancel also covers requestClose(). */
        Html.attribute("closedby", "none"),
        switch (model.kind) {
        | Modal => Html.none
        | Alertdialog(_) => Html.role("alertdialog")
        },
        Ui_attr.aria_bool("modal", true),
        switch (name) {
        | Title =>
          Ui_attr.aria_ids(
            "labelledby",
            [Ui_attr.part_id(model.id, "title")],
          )
        | Label(label) => Html.aria("label", label)
        },
        Ui_attr.aria_ids(
          "describedby",
          has_description ? [Ui_attr.part_id(model.id, "description")] : [],
        ),
        Html.property("returnValue", V.String(model.return_value)),
        /* Only Escape stops propagation (nested dialogs) and native default
           (Chromium's repeated-Escape close watcher). Tab is never cancelled. */
        Html.on(
          "keydown",
          V.Custom_optional(
            D.map(
              event =>
                {
                  ...event,
                  V.stop_propagation: event.V.prevent_default,
                },
              keyboard,
            ),
          ),
        ),
      ]
      @ own([
          Html.custom(
            "cancel",
            D.succeed({
              V.message: Request(Escape),
              stop_propagation: true,
              prevent_default: true,
            }),
          ),
          Html.on(
            "close",
            V.Normal(
              D.map2(
                (value, open_) => Native_closed(value, open_),
                D.at(["target", "returnValue"], D.string),
                D.at(["target", "open"], D.bool),
              ),
            ),
          ),
          Html.on(
            "pointerdown",
            V.Normal(point(outside => Pointer_down(outside))),
          ),
          Html.on(
            "pointerup",
            V.Normal(point(outside => Pointer_up(outside))),
          ),
          Html.on("pointercancel", V.Normal(D.succeed(Pointer_cancel))),
          Html.on("click", V.Normal(point(outside => Click(outside)))),
          /* Submit bubbles; only claim a method=dialog submission, including a
             submitter's formmethod override. No other form behavior is changed. */
          Html.custom(
            "submit",
            D.and_then(
              method_ =>
                String.equal(method_, "dialog")
                  ? D.map(
                      value =>
                        {
                          V.message: Submit(value),
                          stop_propagation: true,
                          prevent_default: true,
                        },
                      D.one_of([
                        D.at(["submitter", "value"], D.string),
                        D.succeed(""),
                      ]),
                    )
                  : D.fail("not a dialog form"),
              D.and_then(
                override =>
                  String.equal(override, "")
                    ? D.at(["target", "method"], D.string)
                    : D.succeed(override),
                D.one_of([
                  D.at(["submitter", "formMethod"], D.string),
                  D.succeed(""),
                ]),
              ),
            ),
          ),
        ]),
    title: [Html.id(Ui_attr.part_id(model.id, "title"))],
    description: [Html.id(Ui_attr.part_id(model.id, "description"))],
    close_button:
      [
        Html.id(Ui_attr.part_id(model.id, "close")),
        Html.type_("button"),
        Html.aria("label", labels.close),
      ]
      @ own([Html.on_click(Request(Close_button))]),
    phase: phase(model),
    is_open: is_open(model),
    is_visible: is_visible(model),
    return_value: model.return_value,
  });
};
