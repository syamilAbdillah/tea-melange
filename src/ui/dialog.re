open Tea_core;
module Html = Tea_html.Html;
module V = Tea_vdom.Vdom;
module D = Decode;
module Events = Tea_browser.Browser_events;

/* Chromium lets focus reach the viewport at a native modal's Tab boundary.
   Repair after default navigation; never cancel Tab or steal focus from
   another connected element. */
let repair_tab = (id, backwards, live) =>
  Tea_dom.Browser_platform.request_animation_frame(_ =>
    switch (Ui_dom.find(id)) {
    | Some(root)
        when
          live^
          && Ui_dom.matches(root, "[open]:not([hidden])")
          && Ui_dom.is_active(Ui_dom.body(Ui_dom.document)) =>
      let edge =
        backwards
          ? Tea_browser.Browser_dom.last_tabbable(root)
          : Tea_browser.Browser_dom.first_tabbable(root);
      Ui_dom.focus(Option.value(~default=root, edge));
    | _ => ()
    }
  );
/* Whether a pointer event on the dialog element itself fell outside its box,
   i.e. on the ::backdrop. */
let outside = (id, target, x, y) =>
  String.equal(id, target)
  && (
    switch (Ui_dom.find(id)) {
    | None => false
    | Some(node) =>
      let r = Ui_dom.rect(node);
      x < r.left || x > r.right || y < r.top || y > r.bottom;
    }
  );
module Browser = Tea_browser.Browser_dom;
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
  /* Pending Tab-boundary repair (backwards, attempt) and native modality
     repair attempt; attempts re-key their subscriptions. */
  tab: option((bool, int)),
  native_repair: option(int),
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
    let left = List.mem(Animation.Left, outs);
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
            switch (model.tab) {
            | Some((_, attempt)) => attempt + 1
            | None => 0
            },
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
  | Native_closed(value, false) when is_visible(model) && model.ready =>
    /* An imperative close cannot be cancelled. Repair native modality before
       continuing the model's guard/leave policy. Prefer method=dialog forms
       (intercepted below). The close event is queued: one from our own previous
       close can arrive after a reopen, before this opening's showModal has run
       (ready=false, open=false) or after it (open=true); both are stale. */
    request(
      Native,
      {
        ...model,
        return_value: value,
        native_repair:
          Some(
            switch (model.native_repair) {
            | Some(attempt) => attempt + 1
            | None => 0
            },
          ),
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
    switch (Ui_dom.find(root_id(model))) {
    | None => ()
    | Some(root) =>
      let focus = Ui_dom.focus(~prevent_scroll=true);
      let fallback = () =>
        focus(Option.value(~default=root, Browser.first_tabbable(root)));
      let target =
        switch (model.kind) {
        | Alertdialog(id) => Some(id)
        | Modal => model.initial_focus
        };
      switch (Option.bind(target, Ui_dom.find)) {
      | Some(node) when Ui_dom.contains(root, node) =>
        focus(node);
        if (!Ui_dom.is_active(node)) {
          fallback();
        };
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
              Browser.show_modal(id, result =>
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
                Browser.close(id, _ => ());
              };
            },
        )
      : Sub.none,
    switch (model.native_repair) {
    | Some(attempt) when is_visible(model) =>
      Sub.make(
        ~home="Tea_ui.Dialog.repair",
        ~key=id ++ ":" ++ string_of_int(attempt),
        ~tagger=message => message,
        ~start=
          _ => {
            let live = ref(true);
            Browser.show_modal(id, result =>
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
    | Some((backwards, attempt)) when is_visible(model) =>
      Sub.make(
        ~home="Tea_ui.Dialog.tab",
        ~key=id ++ ":" ++ string_of_int(attempt),
        ~tagger=message => message,
        ~start=
          _ => {
            let live = ref(true);
            repair_tab(id, backwards, live);
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
      (target, x, y) => constructor(outside(id, target, x, y)),
      D.one_of([D.at(["target", "id"], D.string), D.succeed("")]),
      D.field("clientX", D.float),
      D.field("clientY", D.float),
    );
  let keyboard =
    Ui_keyboard.claiming(event =>
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
        Html.on("keydown", V.Custom_optional(keyboard)),
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
