open Tea_core;
module B = Bindings;
type error =
  | Not_found(string);
type rect = {
  x: float,
  y: float,
  width: float,
  height: float,
  top: float,
  right: float,
  bottom: float,
  left: float,
};
type element_info = {
  rect,
  viewport_width: float,
  viewport_height: float,
  scroll_x: float,
  scroll_y: float,
};
let with_element = (id, f, done_) =>
  B.after_render(() =>
    switch (B.find(id)) {
    | None => done_(Error(Not_found(id)))
    | Some(element) => f(element, done_)
    }
  );
let effect = (id, f) =>
  with_element(
    id,
    (element, done_) => {
      f(element);
      done_(Ok());
    },
  );
let focus = (~prevent_scroll=false, id) =>
  effect(id, element =>
    if (B.method_(element, "focus")) {
      B.focus(element, { "preventScroll": prevent_scroll });
    }
  );
let blur = id =>
  effect(id, element =>
    if (B.method_(element, "blur")) {
      B.blur(element);
    }
  );
let focus_cmd = id => Cmd.effect(_ => focus(id, _ => ()));
/* These reads are synchronous so overlays can check their subscription's live
   flag, select a stop and focus it atomically in the render callback. */
let first_summary = (details, summary) =>
  Option.fold(
    ~none=false,
    ~some=first => first === summary,
    Array.find_opt(
      node =>
        Option.fold(
          ~none=false,
          ~some=parent => parent === details,
          Js.Nullable.toOption(B.parent(node)),
        ),
      B.query_all(details, "summary"),
    ),
  );
let rec visible = element => {
  let style = B.computed_style(B.window, element);
  !B.has_attr(element, "hidden")
  && !B.has_attr(element, "inert")
  && !String.equal(B.style_get(style, "display"), "none")
  && !String.equal(B.style_get(style, "visibility"), "hidden")
  && !String.equal(B.style_get(style, "visibility"), "collapse")
  && (
    switch (Js.Nullable.toOption(B.parent(element))) {
    | None => true
    | Some(parent) =>
      let closed_details =
        String.equal(B.local_name(parent), "details")
        && !B.has_attr(parent, "open");
      (!closed_details || first_summary(parent, element)) && visible(parent);
    }
  );
};
/* Check the whole document, including peers outside the search subtree.
   Names are compared directly, never interpolated into a CSS selector. */
let radio_tabbable = element =>
  if (!B.matches(element, "input[type=radio]")
      || String.equal(B.string(element, "name"), "")) {
    true;
  } else {
    !
      Array.exists(
        other =>
          other !== element
          && String.equal(
               B.string(other, "name"),
               B.string(element, "name"),
             )
          && B.get(other, "form") === B.get(element, "form")
          && B.bool(other, "checked")
          && !B.matches(other, ":disabled"),
        B.query_all(B.root(B.document), "input[type=radio]"),
      );
  };
let tabbable = (~backwards, ~exclude, root) => {
  let chosen = ref(None);
  Array.iter(
    element => {
      let tab = B.tab_index(element);
      let implicit_tab_stop =
        B.matches(
          element,
          "[contenteditable]:not([contenteditable=false]),audio[controls],video[controls]",
        )
        || String.equal(B.local_name(element), "summary")
        && Option.fold(
             ~none=false,
             ~some=
               parent =>
                 String.equal(B.local_name(parent), "details")
                 && first_summary(parent, element),
             Js.Nullable.toOption(B.parent(element)),
           );
      if ((tab >= 0 || implicit_tab_stop && !B.has_attr(element, "tabindex"))
          && !B.matches(element, ":disabled,input[type=hidden]")
          && !exclude(element)
          && visible(element)
          && radio_tabbable(element)) {
        let priority = tab > 0 ? tab : 0;
        switch (chosen^) {
        | Some((_, previous)) =>
          /* Zero is the final class, even after the largest positive tabindex.
             First keeps equal-priority DOM ties; last replaces them. */
          let earlier = priority > 0 && (previous == 0 || priority < previous);
          if (backwards ? !earlier : earlier) {
            chosen := Some((element, priority));
          };
        | None => chosen := Some((element, priority))
        };
      };
    },
    B.query_all(
      root,
      "a[href],area[href],button,input,select,textarea,iframe,summary,[tabindex],[contenteditable]:not([contenteditable=false]),audio[controls],video[controls]",
    ),
  );
  Option.map(((element, _)) => element, chosen^);
};
let first_tabbable = (~exclude=_ => false, root) =>
  tabbable(~backwards=false, ~exclude, root);
let last_tabbable = (~exclude=_ => false, root) =>
  tabbable(~backwards=true, ~exclude, root);
let focus_first = (~within) =>
  with_element(within, (root, done_) =>
    switch (first_tabbable(root)) {
    | None => done_(Error(Not_found(within)))
    | Some(element) =>
      B.focus(element, { "preventScroll": false });
      done_(Ok());
    }
  );
let scroll_into_view = (~block=`Nearest, id) =>
  effect(id, element =>
    if (B.method_(element, "scrollIntoView")) {
      B.scroll_into_view(
        element,
        {
          "block":
            switch (block) {
            | `Nearest => "nearest"
            | `Center => "center"
            | `Start => "start"
            | `End => "end"
            },
        },
      );
    }
  );
let get_element = id =>
  with_element(
    id,
    (element, done_) => {
      let r = B.rect(element);
      done_(
        Ok({
          rect: {
            x: B.number(r, "x"),
            y: B.number(r, "y"),
            width: B.number(r, "width"),
            height: B.number(r, "height"),
            top: B.number(r, "top"),
            right: B.number(r, "right"),
            bottom: B.number(r, "bottom"),
            left: B.number(r, "left"),
          },
          viewport_width: B.inner_width(B.window),
          viewport_height: B.inner_height(B.window),
          scroll_x: B.scroll_x(B.window),
          scroll_y: B.scroll_y(B.window),
        }),
      );
    },
  );
let open_dialog = (modal, id) =>
  effect(id, element =>
    if (!B.has_attr(element, "open")) {
      if (String.equal(B.local_name(element), "dialog")
          && B.method_(element, modal ? "showModal" : "show")) {
        modal ? B.show_modal(element) : B.show(element);
      } else {
        B.set_attr(element, "open", "");
      };
    }
  );
let show_modal = id => open_dialog(true, id);
let show = id => open_dialog(false, id);
let close = id =>
  effect(id, element =>
    if (B.has_attr(element, "open")) {
      if (String.equal(B.local_name(element), "dialog")
          && B.method_(element, "close")) {
        B.close(element);
      } else {
        B.remove_attr(element, "open");
      };
    }
  );
let watch_animations = (id, done_) => {
  let cancelled = ref(false);
  let cleanup = ref(() => ());
  B.after_render(() =>
    if (! cancelled^) {
      switch (B.find(id)) {
      | None => done_(Error(Not_found(id)))
      | Some(element) =>
        if (!B.method_(element, "getAnimations") || !B.connected(element)) {
          done_(Ok());
        } else {
          let animations = B.animations(element, { "subtree": true });
          if (Array.length(animations) == 0) {
            done_(Ok());
          } else {
            let completed = ref(false);
            let observer = ref(None);
            let timer = ref(None);
            let release = () => {
              completed := true;
              Option.iter(B.disconnect, observer^);
              Option.iter(B.clear_timeout, timer^);
            };
            let finish = () =>
              if (! completed^) {
                release();
                done_(Ok());
              };
            cleanup :=
              (
                () =>
                  if (! completed^) {
                    release();
                  }
              );
            if (B.available("MutationObserver")) {
              let o =
                B.mutation_observer(() =>
                  if (!B.connected(element)) {
                    finish();
                  }
                );
              observer := Some(o);
              B.observe_mutations(
                o,
                B.root(B.document),
                {
                  "childList": true,
                  "subtree": true,
                },
              );
            };
            timer := Some(B.timeout(finish, 5000));
            ignore(
              Js.Promise.then_(
                _ => {
                  finish();
                  Js.Promise.resolve();
                },
                B.all_settled(Array.map(B.finished, animations)),
              ),
            );
          };
        }
      };
    }
  );
  () =>
    if (! cancelled^) {
      cancelled := true;
      cleanup^();
    };
};
/* Keep the one-argument Task shape: callers (and the JS e2e probe) apply the
   id first and run the returned task later. */
let wait_for_animations = id => {
  let task = done_ => {
    let _cancel: unit => unit = watch_animations(id, done_);
    ();
  };
  task;
};
let active_element_id = done_ =>
  B.after_render(() => {
    let id =
      Option.bind(
        Js.Nullable.toOption(B.active(B.document)),
        element => {
          let id = B.id(element);
          String.equal(id, "") ? None : Some(id);
        },
      );
    done_(Ok(id));
  });
