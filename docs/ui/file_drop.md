# File_drop

A stateful, headless **file-only** drop target with a native button alternative.
There is no ARIA file-drop pattern; the picker follows the
[APG button pattern](https://www.w3.org/WAI/ARIA/apg/patterns/button/).
No upload, filesystem traversal, clipboard listener or file-byte copy is performed.

## Integration

```reason
open Tea;
module F = Ui.File_drop;
type msg = Files(F.msg);
type model = {files: F.model};

let init = () =>
  ({files: F.init(~id="attachments", ~multiple=true,
     ~accept=["image/*", ".txt"], ~max_size=1048576., ~max_count=3, ())},
   Cmd.none);

let update = (Files(intent), model) => {
  let (files, cmd, outcomes) = F.update(intent, model.files);
  /* Handle Accepted(files) and Rejected(files_with_reasons) here.
     Store selected files/upload state in your parent, not in the submodel. */
  ignore(outcomes);
  ({...model, files}, Cmd.map(intent => Files(intent), cmd));
};
let subscriptions = model =>
  Sub.map(intent => Files(intent), F.subscriptions(model.files));
let view = model =>
  F.view(~model=model.files, ~to_parent=intent => Files(intent),
    ~to_view=parts => Html.div(parts.root, [
      Html.p([], [Html.text("Images or text files, up to 1 MiB each.")]),
      Html.button(parts.button, [Html.text("Choose attachments")]),
      Html.input(parts.input, []),
    ]), ());
```

`subscriptions` is **required**, even for picker-only use. Mount all three parts,
with the input and native button inside the root, for the same lifetime as the
subscription. Never nest independent File_drop roots. Use document-unique id
prefixes. The example in `examples/ui/file_drop/` renders batch outcomes in a
polite live region, demonstrates disabled controls, both capture preferences,
single/multiple inputs and all validation options.

## Public API

The fully documented [interface](../../src/ui/file_drop.rei) is authoritative.

```reason
type handle; /* original native JavaScript File */
type file = {
  handle: handle, name: string, size: float, mime: string,
  last_modified: float,
};
type rejection = Not_accepted | Too_large | Too_many;
type capture = User | Environment;
type out_msg = Accepted(list(file)) | Rejected(list((file, rejection)));
type model;
type msg;

let init:
  (~id: string, ~disabled: bool=?, ~multiple: bool=?,
   ~accept: list(string)=?, ~capture: capture=?, ~max_size: float=?,
   ~max_count: int=?, unit) => model;
let dragging_over: model => bool;
let disabled: model => bool;
let set_disabled: (bool, model) => (model, Cmd.t(msg));
let reset_dragging: model => (model, Cmd.t(msg));
let update: (msg, model) => (model, Cmd.t(msg), list(out_msg));
let subscriptions: model => Sub.t(msg);
type parts('parent) = {
  root: list(Html.attr('parent)),
  button: list(Html.attr('parent)),
  input: list(Html.attr('parent)),
  dragging_over: bool,
};
let view:
  (~model: model, ~to_parent: msg => 'parent,
   ~to_view: parts('parent) => Html.t('parent), unit) => Html.t('parent);
```

Defaults: enabled, single file, unrestricted type/size/count, no capture.
Configuration is fixed at initialization except `set_disabled`; replace/reinitialize
the submodel to change validation options. Setters emit no outcomes. Disabling
and `reset_dragging` clear outstanding drag enters. No selected files are retained
by the component: each drop/selection is an independent batch.

### Files and validation

- `handle` is the original JavaScript `File`, not a serialized copy. Pass it to
  your own typed JavaScript FFI for FormData, object URLs or reading bytes. The
  component does not allocate object URLs, buffers or file-reading resources.
- `name`, `size`, `mime` and `last_modified` are decoded from native `File.name`,
  `.size`, `.type` and `.lastModified`. Byte sizes and timestamps are floats to
  avoid signed 32-bit truncation. `name` is not a path. MIME may be empty and is
  not inferred from the extension. Unknown/malformed non-file entries are ignored.
- `accept` takes dot extensions (including compound extensions such as `.tar.gz`),
  exact MIME types and type wildcards such as `image/*`. Matching is ASCII
  case-insensitive. Entries can contain commas; surrounding whitespace and empty
  tokens are ignored. No tokens means unrestricted. The same accept hint is
  reflected on the input; update validates picker results as well as drops.
- First failure wins: `Not_accepted`, then `Too_large`, then `Too_many`. Size and
  count limits are inclusive. Negative limits clamp to zero; supply finite sizes.
  Invalid files do not consume count. `multiple=false` caps valid files at one
  even when a drop contains several or `max_count` is greater than one.
- Count is **per batch**, not cumulative upload count. Valid files beyond the
  quota are rejected individually; an oversized batch does not reject its valid
  prefix. A zero count accepts none. A zero size permits zero-byte files.
- Outcomes contain only nonempty lists. Mixed batches emit `Accepted` then
  `Rejected`, preserving input order within each list. Empty selections and
  cancellations produce no outcome. Every selection/drop clears drag feedback.
- Picker changes snapshot the FileList and clear the input value synchronously,
  allowing the same file to be selected again. Parent-held handles stay valid.

Accept/size checks are convenience validation, **not a security boundary**.
Names and MIME values are untrusted; validate content and limits on the server.
Render file names as text, never HTML.

## Keyboard and accessibility

| Key | Behavior |
| --- | --- |
| Tab / Shift+Tab | Native focus navigation; hidden input and disabled button skipped |
| Enter | Native button click opens picker synchronously |
| Space down / up | Native button activation on keyup; no synthetic key handling |
| Other keys / modifier chords / IME | Left to the browser |

Apply `button` only to a native `<button>` and give it a visible or accessible
name. It has `type=button`, `disabled` when appropriate, and `aria-controls`
referencing the hidden input. The input is hidden by the native `hidden` property
and has `tabindex=-1`; no CSS is needed for behavior. Do not label the root as an
interactive button or use deprecated `aria-dropeffect`/`aria-grabbed`. Provide
visible restrictions and optionally `aria-describedby` on the button. Announce
accepted/rejected results from the parent (e.g. `role=status`), not by making the
whole target live. Picker activation does not deliberately move focus; browser
picker dialogs manage their own focus and cancellation.

`capture=User|Environment` maps to `capture="user|environment"`. This is a native
mobile media preference, not a promise that the browser provides a camera.
Desktop browsers normally show a file chooser. No read-only or RTL behavior is
applicable to file acquisition.

## Events, state and styling

| Part | Owned facts/events | Data flags |
| --- | --- | --- |
| root | id; delegated dragenter, dragover, dragleave, drop | `data-dragging-over`, `data-disabled` |
| button | id, type, disabled, aria-controls; delegated click | `data-disabled` |
| input | id, type, hidden, tabindex, disabled, multiple, accept, capture; delegated change | none |

Ids are `PREFIX-root`, `PREFIX-button`, `PREFIX-input`. Append only nonconflicting
attributes; do not override owned events or properties. Root styling flags are
presence attributes (`=""`), removed when false. `parts.dragging_over` mirrors the
accessor for custom instructions. No inline styles, scroll locks or focus traps.

Only drags whose DataTransfer advertises `Files` are handled. File dragenter,
dragover and drop prevent default (including rejected files), permitting a drop
and preventing navigation. Dragover does not increment depth. Every nested
file dragenter increments a counter, every file dragleave decrements it (clamped
to zero), and drop clears it; two enters before a render never read stale depth.
Non-file drags, drags outside the target, paste and disabled targets are untouched.

Private, lifecycle-managed document listeners are delegated by containment.
They snapshot protected FileLists **before returning from the drop event**, and
call hidden-input `.click()` **within the native click event**, preserving transient
user activation. A deferred Browser_dom command is deliberately not used for
picker activation. The latest subscription model gates events even before the
next view frame. Listener identities are `(File_drop.events.EVENT, PREFIX)`;
unsubscribe/shutdown removes them synchronously. No listeners are installed by
`view`; map subscriptions and commands with the same parent constructor.

## Limits

- **Paste is excluded**: no clipboard API or paste handler. Keep text paste and
  application clipboard policy independent of this component.
- No directory enumeration, OS filesystem paths, drag source, upload transport,
  preview URL lifecycle or persistent selection model.
- Native `accept`/`capture` UX varies across browsers/devices. Chromium is tested;
  camera behavior on mobile Safari/Android is not claimed.
- Events must bubble to document; stopping propagation earlier prevents handling.
  Parts must be in the current light-DOM document, not an iframe/shadow root.
  Nested independent drop targets are unsupported.
- No document-wide drop blocking: files dropped outside the target, including
  disabled targets, retain the browser's normal behavior.
- If an application removes drag descendants or a platform cancels a drag without
  delivering balancing leaves, call `reset_dragging`. Ordinary leaves and drops
  clear feedback automatically. No global drag-end/watchdog listener is installed.

## Tests

```sh
dune build @test/ui/file_drop/runtest --force
dune build @examples/ui/file_drop/melange
TEA_E2E_CHROMIUM=/usr/bin/google-chrome npm run test:e2e -- file_drop
```

Mounted jsdom tests exercise batched nested drags, prevented defaults, validation
partitions/precedence/limits, native metadata and handle identity, repeated/empty
picker changes, synchronous activation, current disabled state, paste exclusion,
remount and shutdown listener cleanup. Chromium drives real DataTransfer/File
drops, `setInputFiles`, pointer/Enter/Space file choosers, Tab order and focus,
ARIA snapshots, and unfiltered axe in idle, dragging, accepted, rejected and
disabled states. jsdom does not pretend to synthesize native keyboard clicks.
