module Cmd = Tea_core.Cmd;
module Sub = Tea_core.Sub;
module Task = Tea_core.Task;
module Program = Tea_core.Program;
module Runtime = Tea_core.Runtime;
module Decode = Tea_core.Decode;
module Vdom = Tea_vdom.Vdom;
module Html = Tea_html.Html;
module Svg = Tea_html.Svg;
module Attr = Tea_html.Attr;
module Attributes = Tea_html.Attributes;
module Events = Tea_html.Events;
module Keyed = Tea_html.Keyed;
module Http = Tea_http.Http;
module Url = Tea_url.Url;
module Navigation = Tea_nav.Navigation;
module Time = Tea_effects.Time;
module Animation_frame = Tea_effects.Animation_frame;
module AnimationFrame = Tea_effects.Animation_frame;
/** Pure seeded generators and browser generation commands. A module alias,
    so unused generators are tree-shaken from bundles. */
module Random = Tea_random;
/** Browser program entry points; see [Tea_app]. A module alias, so programs
    that never call [application] do not bundle the navigation runtime. */
module App = Tea_app;
/** Post-render DOM tasks, kept as a tree-shakeable module alias. */
module Browser_dom = Tea_browser.Browser_dom;
/** Lifecycle-owned document/window subscriptions. */
module Browser_events = Tea_browser.Browser_events;
/** Accessible headless components; new modules appear under this alias. */
module Ui = Tea_ui;
