# Runtime contract (one page)

Everything after P0 depends on these shapes. Source of truth: the `.rei` files
in `src/core` and `src/vdom`.

## Program

```reason
type Program.t('flags, 'model, 'msg, 'view) = {
  init: 'flags => ('model, Cmd.t('msg)),
  update: ('msg, 'model) => ('model, Cmd.t('msg)),   // msg first, as in Elm
  view: 'model => 'view,
  subscriptions: 'model => Sub.t('msg),
};
```

`App.beginner/element/document` live in `tea.dom`; `Application.application`
lives in `tea.nav` to avoid a DOM-to-navigation dependency. All are exposed by
`Tea.App` and are thin builders around this record. The umbrella beginner
returns unit for the plan's top-level counter sample; lower-level builders and
the other umbrella builders retain runtime lifecycle handles.

## Cmd

`None | Batch(list) | Effect(dispatch => unit)`. Run in order (depth-first,
left-to-right) after the model is committed. `dispatch` may be called sync or
async; it always enqueues.

## Sub

Leaves carry `(home, key, start, tagger)`. After each update the runtime
flattens the tree and diffs by `(home, key)`:
- removed → call its stop fn; added → `start(raw_dispatch)`;
- kept → not restarted; messages go through the **newest** tagger.

## Queue

Single FIFO. `dispatch` while draining appends; draining is never re-entrant.
For each msg: `update` → commit → `on_msg(msg, model)` hook → run cmds → diff
subs. The message log is therefore the complete, replayable history.

## Render schedule (ADR-04)

`Runtime.Animator` = Elm's `_Browser_makeAnimator`:

```
No_request --request--> Pending  (calls raf)
Pending    --frame-->   draw(); Extra_request (calls raf once more)
Extra_request --request--> Pending (no new raf: one is already scheduled)
Extra_request --frame-->   No_request (nothing to draw)
```

`draw` computes `view(model)` and calls `render(view, dispatch)`. N messages in
one tick ⇒ 1 raf ⇒ 1 patch. The initial render is synchronous inside `start`.

## Platform

`Runtime.Make(P: Platform.S)` — `request_animation_frame`, `now`.
Browser impl in `tea.dom`; native tests use a fake with a manual frame queue.
(ADR-06: a functor instead of a dune virtual module, so pure libraries never
need an implementation linked just to run native tests.)

## Vdom ↔ DOM

`Tea_vdom.Vdom.t('msg)` (5 cases) is the view type. `Tea_vdom.Diff.diff(old, new)`
returns a patch plan (pre-order indices); `tea.dom` applies it. Event handlers
are `Decode.t` descriptions interpreted by the backend over the raw DOM event.
