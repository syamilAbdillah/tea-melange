module Animator = {
  type state =
    | No_request
    | Pending
    | Extra_request;
  type t = {
    mutable state,
    raf: (float => unit) => unit,
    draw: unit => unit,
  };
  let make = (~raf, ~draw) => {
    state: No_request,
    raf,
    draw,
  };
  let rec frame = t => {
    switch (t.state) {
    | No_request => ()
    | Extra_request => t.state = No_request
    | Pending =>
      /* Install the spare frame BEFORE drawing: a request made by draw must
         change Extra_request back to Pending, not be overwritten afterward. */
      t.raf(_ => frame(t));
      t.state = Extra_request;
      t.draw();
    };
  };
  let request = t => {
    switch (t.state) {
    | No_request =>
      t.state = Pending;
      t.raf(_ => frame(t));
    | Pending => ()
    | Extra_request => t.state = Pending
    };
  };
  let state = t => t.state;
};

type handle('model, 'msg) = {
  dispatch: 'msg => unit,
  model: unit => 'model,
  shutdown: unit => unit,
};

/* The existential payload never escapes its source's dispatch closure. */
type running('msg) = {
  id: (string, string),
  refresh: Sub.t('msg) => unit,
  stop: unit => unit,
};

/* Stdlib's Fun.protect registers an exception printer at load time, which
   pulls Printexc and all of Printf into browser bundles. */
let protect = (~finally, f) =>
  switch (f()) {
  | v =>
    finally();
    v;
  | exception e =>
    finally();
    raise(e);
  };
let source_key = ((home, key)) => home ++ "\000" ++ key;
let same_id = ((h: string, k: string), (h', k')) => h == h' && k == k';

module Make = (P: Platform.S) => {
  let start =
      (
        ~program: Program.t(_, _, _, _),
        ~flags,
        ~render,
        ~on_msg=(_, _) => (),
        (),
      ) => {
    let (initial, init_cmd) = program.init(flags);
    let model = ref(initial);
    let alive = ref(true);
    let draining = ref(true);
    let queue = Queue.create();
    let running = ref([]);
    let animator = ref(None);
    let rec run_cmd = cmd =>
      if (alive^) {
        switch (cmd) {
        | Cmd.None => ()
        | Cmd.Batch(cmds) => List.iter(run_cmd, cmds)
        | Cmd.Effect(run) => run(dispatch)
        };
      }
    and dispatch = msg =>
      if (alive^) {
        Queue.add(msg, queue);
        drain();
      }
    and drain = () =>
      if (alive^ && ! draining^) {
        draining := true;
        protect(
          ~finally=() => draining := false,
          () => {
            while (alive^ && !Queue.is_empty(queue)) {
              let msg = Queue.take(queue);
              let (next, cmd) = program.update(msg, model^);
              model := next;
              on_msg(msg, next);
              run_cmd(cmd);
              if (alive^) {
                diff_subs(program.subscriptions(model^));
                switch (animator^) {
                | Some(a) => Animator.request(a)
                | None => ()
                };
              };
            }
          },
        );
      }
    and diff_subs = tree =>
      switch (tree, running^) {
      | (Sub.None, []) => ()
      | _ => diff_subs_tree(tree)
      }
    and diff_subs_tree = tree => {
      let seen = String_table.create(16);
      let leaves = ref([]);
      let rec flatten = sub =>
        switch (sub) {
        | Sub.None => ()
        | Sub.Batch(subs) => List.iter(flatten, subs)
        | Sub.Leaf(leaf) =>
          let id = (leaf.home, leaf.key);
          /* Duplicate identities use the first depth-first, left-to-right leaf. */
          if (!String_table.mem(seen, source_key(id))) {
            String_table.replace(seen, source_key(id), ());
            leaves := [(id, sub), ...leaves^];
          };
        };
      flatten(tree);
      let leaves = List.rev(leaves^);
      let old = running^;
      let kept =
        List.filter(
          source => String_table.mem(seen, source_key(source.id)),
          old,
        );
      /* Publish retained sources before stopping removed ones. */
      running := kept;
      List.iter(
        source =>
          if (!String_table.mem(seen, source_key(source.id))) {
            source.stop();
          },
        old,
      );
      /* Refresh every retained tagger before starting any new source: a
         source's start can synchronously trigger another source's callback. */
      List.iter(
        ((id, sub)) =>
          switch (List.find_opt(source => same_id(source.id, id), kept)) {
          | Some(source) => source.refresh(sub)
          | None => ()
          },
        leaves,
      );
      List.iter(
        ((id, sub)) =>
          if (alive^) {
            switch (List.find_opt(source => same_id(source.id, id), kept)) {
            | Some(_) => ()
            | None =>
              switch (sub) {
              | Sub.Leaf(leaf) =>
                let tagger = ref(leaf.tagger);
                let live = ref(true);
                let stop =
                  leaf.start(raw =>
                    if (alive^ && live^) {
                      dispatch(tagger^(raw));
                    }
                  );
                let source = {
                  id,
                  refresh: newest =>
                    switch (newest) {
                    | Sub.Leaf(next) =>
                      /* A stable (home,key) denotes a source with a stable raw
                         payload type. The existential hides that equality from
                         the compiler; only this per-leaf tagger cell needs a cast. */
                      tagger := Obj.magic(next.tagger)
                    | Sub.None
                    | Sub.Batch(_) => ()
                    },
                  stop: () => {
                    live := false;
                    stop();
                  },
                };
                if (alive^) {
                  running := List.append(running^, [source]);
                } else {
                  source.stop();
                };
              | Sub.None
              | Sub.Batch(_) => ()
              }
            };
          },
        leaves,
      );
    };
    let draw = () =>
      if (alive^) {
        draining := true;
        protect(
          ~finally=
            () => {
              draining := false;
              drain();
            },
          () => render(program.view(model^), dispatch),
        );
      };
    animator := Some(Animator.make(~raf=P.request_animation_frame, ~draw));
    /* Initialization is one protected transaction: render first, then commands,
       then subscriptions, and only then process any messages they emitted. */
    protect(
      ~finally=() => draining := false,
      () => {
        render(program.view(model^), dispatch);
        run_cmd(init_cmd);
        diff_subs(program.subscriptions(model^));
      },
    );
    drain();
    {
      dispatch,
      model: () => model^,
      shutdown: () =>
        if (alive^) {
          alive := false;
          Queue.clear(queue);
          let sources = running^;
          running := [];
          List.iter(source => source.stop(), sources);
        },
    };
  };
};
