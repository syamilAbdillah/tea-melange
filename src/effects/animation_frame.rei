/** Frame timestamp in milliseconds (monotonic, not Unix epoch). Falls back to a 16ms timer. */
let on_animation_frame: (float => 'msg) => Tea_core.Sub.t('msg);
/** Elapsed milliseconds since the preceding frame; first delta is zero. */
let on_animation_frame_delta: (float => 'msg) => Tea_core.Sub.t('msg);
