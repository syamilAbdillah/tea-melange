let subscribe = (key, tagger) =>
  Tea_core.Sub.make(
    ~home="AnimationFrame",
    ~key,
    ~tagger,
    ~start=dispatch => {
      let active = ref(true);
      let cancel = ref(() => ());
      let previous = ref(None);
      let rec tick = timestamp =>
        if (active^) {
          let delta =
            switch (previous^) {
            | None => 0.
            | Some(t) => timestamp -. t
            };
          previous := Some(timestamp);
          dispatch(key == "delta" ? delta : timestamp);
          if (active^) {
            cancel := Clock.frame(tick);
          };
        };
      cancel := Clock.frame(tick);
      () => {
        active := false;
        cancel^();
      };
    },
  );
let on_animation_frame = tagger => subscribe("frame", tagger);
let on_animation_frame_delta = tagger => subscribe("delta", tagger);
