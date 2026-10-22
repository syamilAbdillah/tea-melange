# Foundation APIs and decisions

The `.rei` files are the authoritative API and odoc source. Examples below omit
`Tea_core.` qualifications on Task/Cmd/Sub/Decode and `Tea_html.` on Html.
Private UI helpers are available **inside** component implementations, not to
library consumers. Do not expose them through a component interface.

## Browser_dom

```reason
type error = Not_found(string);
type rect = {x: float, y: float, width: float, height: float,
             top: float, right: float, bottom: float, left: float};
type element_info = {rect, viewport_width: float, viewport_height: float,
                     scroll_x: float, scroll_y: float};
let focus: (~prevent_scroll: bool=?, string) => Task.t(unit, error);
let blur: string => Task.t(unit, error);
let focus_cmd: string => Cmd.t('msg);
let first_tabbable:
  (~exclude: (Dom.node => bool)=?, Dom.node) => option(Dom.node);
let last_tabbable:
  (~exclude: (Dom.node => bool)=?, Dom.node) => option(Dom.node);
let focus_first: (~within: string) => Task.t(unit, error);
let scroll_into_view:
  (~block: [ `Nearest | `Center | `Start | `End ]=?, string) => Task.t(unit, error);
let get_element: string => Task.t(element_info, error);
let show_modal: string => Task.t(unit, error);
let show: string => Task.t(unit, error);
let close: string => Task.t(unit, error);
let wait_for_animations: string => Task.t(unit, error);
let active_element_id: Task.t(option(string), 'e);
```

Every task is lazy and executes after render, even reads. Ids resolve then, not
when constructing the task. Missing ids return `Not_found`; `focus_first` also
returns it for an empty candidate set. Native dialog/scroll APIs are detected;
unsupported dialog methods only manipulate `open` (not a modality polyfill).
Focus returns success when the id exists; the platform ultimately decides
whether an element can actually take focus.

`first_tabbable` and `last_tabbable` are synchronous read-only queries, separate
from the deferred tasks. They return a descendant node (never the root), or
`None`, without moving focus or scheduling work. Call them after the required
render and inside the caller's cancellation/live check. Dialog uses this in the
same callback as `showModal()` so initial focus cannot steal a later Tab/click;
Popover and Date_picker share the same ordering and exclusions. `focus_first`
uses `first_tabbable` after render.

Tab order is ascending positive tabindex, then zero/implicit stops in DOM order;
ties keep DOM order. Negative tabindex, disabled controls/fieldsets (with the
first-legend exception), hidden inputs, hidden/inert ancestors, display:none,
visibility:hidden/collapse, and closed-details content outside its first direct
summary are skipped. Native editable/media stops are included unless explicitly
negative. Named radio groups compare names and form owners across the document:
a checked enabled peer is the group's stop, even outside the search root;
otherwise the first/last query follows entry direction. Unnamed radios remain
independent. `aria-disabled` alone does not remove a tab stop.

The optional `exclude` predicate rejects candidates; Date_picker uses it to
skip its boundary spans. Selection is computed afresh, including ancestor state
outside the root, with no layout-box measurement (so jsdom works). Visibility
checks are conservative through ancestors; there is no shadow-root or iframe
traversal.

`wait_for_animations` takes a subtree snapshot after render. It awaits all-settled
finished promises, resolves on removal, and has a 5000 ms watchdog for paused,
infinite or broken animations. This explicit bound implements “never hang”;
it can cut short an intentional animation longer than five seconds. All completion
paths release their timer/observer and complete once. No active animations or
missing getAnimations resolves immediately.

## Browser_events

```reason
type viewport = {width: float, height: float};
type element_size = {width: float, height: float};
let on_key_down:
  (~key: string, ~capture: bool=?, Decode.t('msg)) => Sub.t('msg);
let on_pointer_down_outside:
  (~ids: list(string), ~focus: bool=?, 'msg) => Sub.t('msg);
let on_resize: (viewport => 'msg) => Sub.t('msg);
let on_scroll: 'msg => Sub.t('msg);
let on_visibility_change: (bool => 'msg) => Sub.t('msg);
let on_element_resize: (~id: string, element_size => 'msg) => Sub.t('msg);
let on_media_query: (~query: string, bool => 'msg) => Sub.t('msg);
let scroll_lock: (~key: string) => Sub.t('msg);
type focus_target = Element(string) | First_tabbable(string) | Last_tabbable(string);
let focus:
  (~key: string, ~target: focus_target, ~ready: bool=?, ~from: list(string)=?,
   ~prevent_scroll: bool=?, bool => 'msg) => Sub.t('msg);
let restore_focus: (~key: string, ~within: string=?, unit) => Sub.t('msg);
```

Identities and timing are documented individually in
`src/browser/browser_events.rei`. Keydown's key is a **subscription identity**,
not a keyboard key; decoder failure dispatches nothing. The current decoder is
used even without listener restart. Outside detection listens in capture phase;
missing ids are outside and nested ids are safe. Focusin is opt-in.

Starts needing nodes are deferred and can be cancelled before installation.
Listeners/observers stop synchronously; scroll lease release and focus return
wait for the closing render. Even synchronous initial media delivery that
unsubscribes itself cannot lose its disposer. Media delivers current match on
start; unsupported matchMedia delivers false. ResizeObserver is a no-op when
unsupported or when the id is absent; resubscribe to observe a replaced node.

Scroll leases share a counter across keys and runtimes. The first lease stores
exact html/body inline style attributes, compensates scrollbar width, and sets
overflow hidden. Only iOS/iPadOS uses fixed body positioning (platform/touch
feature heuristic), restoring scroll coordinates afterwards. Other platforms do
not need that hack. Do not mutate owned inline styles while locked. Restoration
also flushes Chromium's lazy CSSOM empty-attribute serialization so an absent
style attribute remains absent, not `style=""`.

`focus` performs one cancellable post-render attempt. Put the owner id and an
immutable request generation in `key`, and remove the subscription on completion.
It captures focus immediately on start, observes intervening focusin/focusout,
and will not override an intentional focus movement (including away and back).
Render-induced loss to body can be repaired. Start the source immediately on
opening with `ready=false`, then update `ready` after Anchor reports visibility.
The retained source keeps the original focus snapshot and reads the latest target
and readiness without restarting. A false-to-true readiness transition crosses
one additional frame so its render precedes focus. Thus arbitrary programmatic
openers still work, while moving focus during positioning cancels the request.
Optional `from` additionally limits the origin to supplied id subtrees or body;
its configuration is fixed for the lifetime of a key. The completion boolean says
whether the resolved target took focus; missing targets complete false. Stopping
or shutdown removes both listeners immediately and suppresses work/completion.
Mutable cancellation cells belong only to the source's start closure, never to
component models. `Radio_group` and `Tabs` use immutable request generations.

Focus return remembers the element on subscription start, before the opening
render. Stop waits for the closing render, then restores only a connected
remembered element when current focus is body/absent/disconnected. It does not
steal intentional focus on another connected element.

## Private Ui_keyboard

```reason
type event = {key: string, shift: bool, ctrl: bool, alt: bool, meta: bool,
              composing: bool, time_stamp: float, repeat: bool};
let decoder: Decode.t(event);
let plain: event => bool;      /* no ctrl/alt/meta */
let available: event => bool;  /* not composing */
let custom:
  (event => option((option('msg), bool))) =>
  Decode.t(Tea_vdom.Vdom.custom(option('msg)));
let on_key_down: (event => option('msg)) => Html.attr('msg);
```

The callback's pair is `(optional_message, prevent_default)`. IME composition
(including keyCode 229) never reaches the callback. Modifiers do, because APG
models need them (Alt+ArrowDown opens a combobox, Tab closes a menu): check
`plain` before claiming keys that browsers or assistive technology bind with
ctrl/alt/meta, and never prevent default on Tab.
A handled event can prevent default without emitting a synthetic parent message.

## Private Ui_nav

```reason
type direction = Ltr | Rtl;
let first: array(bool) => option(int);
let last: array(bool) => option(int);
let next: (~wrap: bool=?, ~from: int, array(bool)) => option(int);
let prev: (~wrap: bool=?, ~from: int, array(bool)) => option(int);
let page: (~size: int, ~forward: bool, ~from: int, array(bool)) => option(int);
let repair: (~from: int, array(bool)) => option(int);
let horizontal: (~dir: direction, string) => option(int);
```

True flags mean disabled. Empty/all-disabled returns None. Wrapping defaults to
true; nonwrapping navigation retains an enabled current index at the boundary.
Invalid indices start before/after the array according to direction. Page sizes
saturate and search beyond a disabled target, falling back toward the current
index. Repair keeps an enabled current index or picks the first enabled one.
Horizontal arrows produce +1/-1, swapping in RTL.

## Private Ui_typeahead

```reason
type t;
let empty: t;
let buffer: t => string;
let last_time_stamp: t => option(float);
let normalize: string => string;
let feed:
  (~timeout: float=?, ~time_stamp: float, ~key: string, ~from: int,
   ~labels: array(string), ~disabled: array(bool), t) => (t, option(int));
```

Uses event timestamps, a 500 ms default reset, repeated-letter cycling and
case/Latin-1 accent/ligature-insensitive prefixes. Longer prefixes retain the
current candidate; a failed prefix retries the current character. Noncharacters
are ignored; missing disabled flags count as disabled. No locale-sensitive/full
Unicode casefold or combining-mark normalization is attempted.

## Private Ui_attr

```reason
let data_flag: (string, bool) => Html.attr('msg);
let aria_bool: (string, bool) => Html.attr('msg);
let aria_ids: (string, list(string)) => Html.attr('msg);
let part_id: (string, string) => string;
let safe_key: string => string;
let item_id: (string, string) => string;
```

Names exclude data-/aria-. False presence flags and empty id lists emit
Attr.none. Part ids are `id-part`; item ids are `id-item-<encoded key>`.
Melange UTF-16 code units outside letters/digits/hyphen become `_hhhh`.
Underscores are escaped too: otherwise a literal escape-looking key could
collide with another key. Empty encodes as `_`. The result uses only the
specification's safe alphabet, but is deliberately more restrictive to ensure
injectivity. Consumers should not depend on a particular encoded spelling.

## Private browser bindings

Keep a capability inside its component, hidden by the `.rei`, until at least two
consumers need the same behavior and ownership contract. Repeating a low-level
`getElementById`, timer, event getter or add/removeEventListener declaration alone
does not establish a shared lifecycle abstraction. Reuse Browser_dom and
Browser_events where their public contracts already fit; do not expand those APIs
for a single component or expose a generic bag of DOM bindings.

The Z1a review retains these component-specific bindings:

- **Hover_intent**: a cancellable delay with generation rejection, performance
  timestamps, window blur and document pointerout with relatedTarget/pointerType
  checks. Its two-region intent state and warm-group timing differ from Toast's
  per-entry remaining-duration accounting. Visibility already uses Browser_events;
  Tooltip consumes Hover_intent's public API rather than duplicating this source.
- **Anchor**: manual showPopover/hidePopover capability detection, connection and
  :popover-open checks, plus devicePixelRatio rounding. Measurements, scroll,
  resize and element resize already use the browser APIs. Menu, Popover and
  Tooltip consume Anchor's public API; they are not separate consumers needing
  a second implementation of the private popover lease.
- **Slider**: document pointermove/up/cancel and lostpointercapture, pointer-id
  filtering, window blur, and capture/release on the track. Its frame-coalesced
  moves and final commit belong to the drag session. Scroll/resize already reuse
  Browser_events. Generic pointer primitives also occur in Drag_and_drop, but its
  lift threshold, touch delay, measurements and cancellation contract differ;
  sharing the raw declarations would not share a session lifecycle.
- **Virtual_list**: viewport scrollTop/clientHeight reads, exact scrollTop writes,
  child-list observation, and ResizeObserver border-box measurements tied to
  stable row keys. Browser_events.on_element_resize observes one id's content
  box; it cannot replace dynamic row registration, generation-checked samples
  or anchor-preserving writes. These operations have one capability consumer.
- **Combobox**: active-element/value guards and setSelectionRange for inline
  completion after render. Existing browser helpers provide focus effects; no
  second component needs the guarded inline-completion selection operation.
  Its JavaScript lowercase binding is a string operation, not a browser source;
  shared search normalization already lives in Ui_typeahead.
- **Toast**: per-entry cancellable remaining-duration timers and initial
  document.hidden read; capture hotkeys that can cancel the configured chord;
  and region focusin/focusout containment boundaries. Browser_events keydown is
  deliberately observational and cannot consume the hotkey. Tooltip's global
  Escape source and Drag_and_drop's drag keys have different claiming rules;
  Toast alone needs the exact configurable chord plus hotkey-entry bookkeeping.
  Visibility, focus restoration and animation use the existing public helpers.

No new shared helper or browser API is required by this review. A future second
consumer should first specify matching timing, cancellation, identity and event
claiming semantics, then coordinate an extraction with the foundation owner.

Reason test literals that model browser text use `{js|…|js}` so Melange receives
JavaScript Unicode strings rather than OCaml UTF-8 byte strings. Deliberate
byte-domain id tests (the 0–255 `String.make`/`Char.chr` alphabet sweep) keep byte
inputs and explain that choice locally. Native HTML serialization and URL
percent-decoding tests also retain their explicit UTF-8 byte expectations.

## Existing infrastructure changes

- Runtime requests draw before commands/subscription reconciliation. Native
  tests exercise No_request, Pending and Extra_request. Initialization remains
  synchronous-render-first. The runtime contract and Unreleased notes agree.
- `Sub.filter_map` and its Filtered constructor are additive. Rejected events
  never enter update, while kept subscriptions use the newest filter. Browser
  keydown requires both properties; decoding inside a start closure would keep
  a stale decoder, and fake messages would violate the parent API.
- `Vdom.Custom_optional` is additive. Space keydown and disabled clicks must
  prevent defaults without a fake message. Message mapping, DOM interpretation
  and native Simulate understand it; existing handlers preserve their behavior.
- Tea adds module aliases and library dependencies, not eager module wrappers.
- The example server exports reusable ephemeral-port serving while preserving
  existing example names, MIME types, traversal checks and SPA fallback.
- Root dev dependencies/lockfile, npm script and CI add only the prescribed
  accessibility/browser test infrastructure. No runtime npm dependencies.

## Signature clarification

Stateless `Button.view` uses the specification's `~to_view` parts callback,
with a final unit argument so Reason can erase the optional arguments. This
corrects the all-labelled illustrative signature in §2.1; `Button.button` is an
additional native-by-default direct-element convenience, and `Button.parts`
allows explicit bundle composition. See [button.md](button.md) for all signatures.

## Scope and limitations

Button is the reference component, not a showcase application. Chromium is the
real-browser gate; iOS fixed-body behavior is feature-detected but not a claim
of Safari device coverage. Shadow-root traversal and cross-document/iframe focus
are not provided: ids refer to the current document, and outside containment uses
ordinary DOM containment. Focus-first visibility is deliberately conservative
for hidden/inert ancestors and closed details. Applications needing a specific
initial focus target should use `focus` instead.
