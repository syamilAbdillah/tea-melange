/** A TEA program, independent of how its view is rendered.
    ['view] is [Tea_vdom.Vdom.t('msg)] for browser programs, but the runtime
    never inspects it. */
type t('flags, 'model, 'msg, 'view) = {
  /** Initialize from caller flags and describe startup effects. */
  init: 'flags => ('model, Cmd.t('msg)),
  /** Pure model-first transition and its effects. */
  update: ('model, 'msg) => ('model, Cmd.t('msg)),
  /** Pure projection of the current model. */
  view: 'model => 'view,
  /** Describe active sources for the committed model. */
  subscriptions: 'model => Sub.t('msg),
};
