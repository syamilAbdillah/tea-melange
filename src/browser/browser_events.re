module B = Bindings;
module Sub = Tea_core.Sub;
type viewport = {
  width: float,
  height: float,
};
type element_size = {
  width: float,
  height: float,
};
let deferred = install => {
  let active = ref(true);
  let stop = ref(() => ());
  B.after_render(() =>
    if (active^) {
      let dispose = install();
      /* Installation may synchronously dispatch (media queries), removing this
         subscription before it returns its disposer. Do not lose that cleanup. */
      if (active^) {
        stop := dispose;
      } else {
        dispose();
      };
    }
  );
  () =>
    if (active^) {
      active := false;
      stop^();
    };
};
let listener = (target, name, capture, callback) => {
  B.listen(target, name, callback, capture);
  () => B.unlisten(target, name, callback, capture);
};
let source = (~home, ~key, ~tagger, ~install) =>
  Sub.make(~home, ~key, ~tagger, ~start=dispatch =>
    deferred(() => install(dispatch))
  );
let on_key_down = (~key, ~capture=false, decoder) =>
  Sub.filter_map(
    event =>
      switch (Tea_dom.Dom_decode.run(decoder, event)) {
      | Ok(msg) => Some(msg)
      | Error(_) => None
      },
    source(
      ~home=
        capture
          ? "Browser_events.keydown.capture" : "Browser_events.keydown.bubble",
      ~key,
      ~tagger=x => x,
      ~install=dispatch => listener(B.document, "keydown", capture, dispatch),
    ),
  );
let encode_ids = ids =>
  String.concat(
    "",
    List.map(id => string_of_int(String.length(id)) ++ ":" ++ id, ids),
  );
let on_pointer_down_outside = (~ids, ~focus=false, msg) =>
  source(
    ~home="Browser_events.outside",
    ~key=(focus ? "focus:" : "pointer:") ++ encode_ids(ids),
    ~tagger=() => msg,
    ~install=
      dispatch => {
        let callback = event => {
          let target = B.get(event, "target");
          let inside =
            !Js.testAny(target)
            && Js.typeof(B.get(target, "nodeType")) == "number"
            && List.exists(
                 id =>
                   switch (B.find(id)) {
                   | Some(element) => B.contains(element, B.cast(target))
                   | None => false
                   },
                 ids,
               );
          if (!inside) {
            dispatch();
          };
        };
        let pointer = listener(B.document, "pointerdown", true, callback);
        let focus_stop =
          focus
            ? listener(B.document, "focusin", true, callback) : (() => ());
        () => {
          pointer();
          focus_stop();
        };
      },
  );
let on_resize = tagger =>
  source(
    ~home="Browser_events.resize", ~key="window", ~tagger, ~install=dispatch =>
    listener(B.window, "resize", false, _ =>
      dispatch(
        {
          width: B.inner_width(B.window),
          height: B.inner_height(B.window),
        }: viewport,
      )
    )
  );
let on_scroll = msg =>
  source(
    ~home="Browser_events.scroll",
    ~key="document",
    ~tagger=() => msg,
    ~install=dispatch => listener(B.document, "scroll", true, _ => dispatch()),
  );
let on_visibility_change = tagger =>
  source(
    ~home="Browser_events.visibility",
    ~key="document",
    ~tagger,
    ~install=dispatch =>
    listener(B.document, "visibilitychange", false, _ =>
      dispatch(
        String.equal(B.string(B.document, "visibilityState"), "visible"),
      )
    )
  );
let on_element_resize = (~id, tagger) =>
  source(
    ~home="Browser_events.element_resize", ~key=id, ~tagger, ~install=dispatch =>
    switch (B.find(id)) {
    | Some(element) when B.available("ResizeObserver") =>
      let observer =
        B.resize_observer(entries =>
          Array.iter(
            entry => {
              let rect = B.get(entry, "contentRect");
              dispatch(
                {
                  width: B.number(rect, "width"),
                  height: B.number(rect, "height"),
                }: element_size,
              );
            },
            entries,
          )
        );
      B.observe_size(observer, element);
      (() => B.disconnect(observer));
    | _ => (() => ())
    }
  );
let on_media_query = (~query, tagger) =>
  source(
    ~home="Browser_events.media_query", ~key=query, ~tagger, ~install=dispatch =>
    if (B.method_(B.window, "matchMedia")) {
      let media = B.match_media(B.window, query);
      let callback = _ => dispatch(B.media_matches(media));
      let stop =
        if (B.method_(media, "addEventListener")) {
          listener(media, "change", false, callback);
        } else if (B.method_(media, "addListener")) {
          B.media_listen(media, callback);
          () => B.media_unlisten(media, callback);
        } else {
          () => ();
        };
      dispatch(B.media_matches(media));
      stop;
    } else {
      dispatch(false);
      () => ();
    }
  );
type lock_snapshot = {
  html: option(string),
  body: option(string),
  ios: bool,
  x: float,
  y: float,
};
let locks = ref(0);
let snapshot = ref(None);
let acquire = () => {
  if (locks^ == 0) {
    let html = B.root(B.document);
    let body = B.body(B.document);
    let ios = B.is_ios();
    let x = B.scroll_x(B.window);
    let y = B.scroll_y(B.window);
    snapshot :=
      Some({
        html: Js.Nullable.toOption(B.attr(html, "style")),
        body: Js.Nullable.toOption(B.attr(body, "style")),
        ios,
        x,
        y,
      });
    let scrollbar =
      max(0., B.inner_width(B.window) -. B.client_width(html));
    let previous_padding =
      B.style_get(B.computed_style(B.window, body), "padding-right");
    B.style_set(B.style(html), "overflow", "hidden");
    B.style_set(B.style(body), "overflow", "hidden");
    B.style_set(B.style(html), "overscroll-behavior", "none");
    if (scrollbar > 0.) {
      B.style_set(
        B.style(body),
        "padding-right",
        "calc("
        ++ (String.equal(previous_padding, "") ? "0px" : previous_padding)
        ++ " + "
        ++ Js.Float.toString(scrollbar)
        ++ "px)",
      );
    };
    if (ios) {
      B.style_set(B.style(body), "position", "fixed");
      B.style_set(B.style(body), "top", Js.Float.toString(-. y) ++ "px");
      B.style_set(B.style(body), "left", Js.Float.toString(-. x) ++ "px");
      B.style_set(B.style(body), "width", "100%");
    };
  };
  incr(locks);
};
let release = () =>
  if (locks^ > 0) {
    decr(locks);
    if (locks^ == 0) {
      switch (snapshot^) {
      | None => ()
      | Some(s) =>
        B.restore_style(B.root(B.document), s.html);
        B.restore_style(B.body(B.document), s.body);
        if (s.ios) {
          B.scroll_to(B.window, s.x, s.y);
        };
      };
      snapshot := None;
    };
  };
let scroll_lock = (~key) =>
  Sub.filter_map(
    _ => None,
    source(
      ~home="Browser_events.scroll_lock",
      ~key,
      ~tagger=() => (),
      ~install=
        _ => {
          acquire();
          () => B.after_render(release);
        },
    ),
  );
type focus_target =
  | Element(string)
  | First_tabbable(string)
  | Last_tabbable(string);
let focus =
    (~key, ~target, ~ready=true, ~from=[], ~prevent_scroll=true, tagger) =>
  /* A retained subscription's filter is refreshed by the runtime. Each frame
     offers a guarded attempt to that newest filter, so readiness/target can
     change without discarding the original focus snapshot. */
  Sub.filter_map(
    attempt => Option.map(tagger, attempt(ready, target, prevent_scroll)),
    Sub.make(
      ~home="Browser_events.focus",
      ~key,
      ~tagger=attempt => attempt,
      ~start=
        dispatch => {
          let live = ref(true);
          let moved = ref(false);
          let blurred = ref(false);
          let remembered = Js.Nullable.toOption(B.active(B.document));
          let original_id = Option.map(B.id, remembered);
          let had_tabindex =
            Option.fold(
              ~none=false,
              ~some=node => B.has_attr(node, "tabindex"),
              remembered,
            );
          let removed_by_render = node =>
            !B.connected(node)
            || Option.fold(
                 ~none=false,
                 ~some=id => !String.equal(id, B.id(node)),
                 original_id,
               )
            || had_tabindex
            && !B.has_attr(node, "tabindex")
            || B.matches(node, ":disabled");
          let same_active = () =>
            switch (remembered, Js.Nullable.toOption(B.active(B.document))) {
            | (None, None) => true
            | (Some(before), Some(now)) =>
              before === now
              || now === B.body(B.document)
              && (! blurred^ || removed_by_render(before))
            | _ => false
            };
          let allowed = () =>
            List.is_empty(from)
            || (
              switch (Js.Nullable.toOption(B.active(B.document))) {
              | None => true
              | Some(now) =>
                now === B.body(B.document)
                || List.exists(
                     id =>
                       switch (B.find(id)) {
                       | Some(root) => B.contains(root, now)
                       | None => false
                       },
                     from,
                   )
              }
            );
          let initially_allowed = allowed();
          /* Observe before render, not in the deferred callback: even an away-and-
             back focus journey is an intentional user action that supersedes us. */
          let stop_in =
            listener(B.document, "focusin", true, _ => moved := true);
          let stop_out =
            listener(B.document, "focusout", true, event =>
              if (Js.testAny(B.get(event, "relatedTarget"))) {
                /* Removal/recycling can blur to body during render. Distinguish
                   it at the attempt from an explicit blur of an intact node. */
                blurred := true;
              } else {
                moved := true;
              }
            );
          let stop = () => {
            stop_in();
            stop_out();
          };

          let finished = ref(false);
          /* A kept mapper may observe readiness in the same frame that produced
             its model update, before that update has drawn. Cross one more frame
             when readiness first becomes true so focus always follows render. */
          let armed = ref(ready);
          let rec tick = () =>
            B.after_render(() =>
              if (live^ && ! finished^) {
                dispatch((ready, target, prevent_scroll) =>
                  if ((!ready || ! armed^)
                      && ! moved^
                      && initially_allowed
                      && same_active()
                      && allowed()) {
                    armed := ready;
                    tick();
                    None;
                  } else {
                    finished := true;
                    stop();
                    let candidate =
                      if (! moved^
                          && initially_allowed
                          && same_active()
                          && allowed()) {
                        switch (target) {
                        | Element(id) => B.find(id)
                        | First_tabbable(id) =>
                          Option.bind(B.find(id), root =>
                            Browser_dom.first_tabbable(root)
                          )
                        | Last_tabbable(id) =>
                          Option.bind(B.find(id), root =>
                            Browser_dom.last_tabbable(root)
                          )
                        };
                      } else {
                        None;
                      };
                    let focused =
                      switch (candidate) {
                      | Some(element)
                          when
                            B.connected(element)
                            && B.method_(element, "focus") =>
                        B.focus(
                          element,
                          { "preventScroll": prevent_scroll },
                        );
                        switch (Js.Nullable.toOption(B.active(B.document))) {
                        | Some(now) => now === element
                        | None => false
                        };
                      | _ => false
                      };

                    live^ ? Some(focused) : None;
                  }
                );
              }
            );
          tick();
          () => {
            live := false;
            stop();
          };
        },
    ),
  );
let restore_focus = (~key, ~within=?, ()) =>
  Sub.filter_map(
    _ => None,
    Sub.make(
      ~home="Browser_events.restore_focus",
      ~key,
      ~tagger=() => (),
      ~start=
        _ => {
          let remembered = Js.Nullable.toOption(B.active(B.document));
          () =>
            B.after_render(() =>
              Option.iter(
                element => {
                  let safe =
                    switch (Js.Nullable.toOption(B.active(B.document))) {
                    | None => true
                    | Some(current) =>
                      current === B.body(B.document)
                      || !B.connected(current)
                      || (
                        switch (Option.bind(within, B.find)) {
                        | Some(container) => B.contains(container, current)
                        | None => false
                        }
                      )
                    };
                  if (safe
                      && B.connected(element)
                      && B.method_(element, "focus")) {
                    B.focus(element, { "preventScroll": true });
                  };
                },
                remembered,
              )
            );
        },
    ),
  );
