type timer;
external set_timeout: (unit => unit, float) => timer = "setTimeout";
external clear_timeout: timer => unit = "clearTimeout";
external set_interval: (unit => unit, float) => timer = "setInterval";
external clear_interval: timer => unit = "clearInterval";
[@mel.scope "Date"] external now: unit => float = "now";
let frame: (float => unit, unit) => unit = [%mel.raw
  {|function(cb) { if (typeof requestAnimationFrame === 'function') { var id = requestAnimationFrame(cb); return function() { cancelAnimationFrame(id); }; } var id = setTimeout(function() { cb(typeof performance !== 'undefined' ? performance.now() : Date.now()); }, 16); return function() { clearTimeout(id); }; }|}
];
