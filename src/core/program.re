/** A TEA program, independent of how its view is rendered.
    ['view] is [Tea_vdom.Vdom.t('msg)] for browser programs, but the runtime
    never inspects it. */
type t('flags, 'model, 'msg, 'view) = {
  init: 'flags => ('model, Cmd.t('msg)),
  update: ('model, 'msg) => ('model, Cmd.t('msg)),
  view: 'model => 'view,
  subscriptions: 'model => Sub.t('msg),
};
