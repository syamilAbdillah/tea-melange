/** Stateful file-only drop target with a native file-picker button.
    APG button: https://www.w3.org/WAI/ARIA/apg/patterns/button/
    There is no ARIA file-drop pattern; the native button is the accessible
    alternative to dragging.

    Keyboard: Tab/Shift+Tab navigate normally; Enter and Space activate the
    native button using browser defaults. No key handlers or paste support.
    Data flags: data-dragging-over on root; data-disabled on root and button.
    Mount all parts and map [subscriptions] for as long as the view is mounted. */

/** Opaque native JavaScript File object, retained without copying its bytes.
    Consumers can pass it to their own typed JavaScript FFI (e.g. FormData). */
type handle;
/** A decoded file with its original native handle. Sizes/timestamps are floats
    to preserve JavaScript numbers beyond the signed 32-bit integer range. */
type file = {
  /** Original JavaScript File. */
  handle,
  /** File.name, not a filesystem path. */
  name: string,
  /** File.size in bytes. */
  size: float,
  /** File.type (possibly empty); never inferred from the filename. */
  mime: string,
  /** File.lastModified in milliseconds since the Unix epoch. */
  last_modified: float,
};
/** First failing validation rule, in this order. */
type rejection =
  | /** No configured MIME/extension token matches. */
    Not_accepted
  | /** Size exceeds max_size; equality is accepted. */
    Too_large
  | /** Accepted-file quota for this batch is full. */
    Too_many;
/** Native media-capture preference; browser support varies. */
type capture =
  | /** Front-facing camera/microphone. */
    User
  | /** Outward-facing camera/microphone. */
    Environment;
/** Batch outcomes, in original file order within each list. Empty lists are
    not emitted. Accepted precedes Rejected for mixed batches. No files are
    stored in the submodel; upload/selection state belongs to the parent. */
type out_msg =
  | /** Valid files in this batch. */
    Accepted(list(file))
  | /** Invalid/excess files and the first reason each was rejected. */
    Rejected(
      list((file, rejection)),
    );
/** Abstract drag-depth and configuration state. */
type model;
/** Opaque event intents; update, not view, computes drag depth and validation. */
type msg;
/** Create a document-unique id prefix. Defaults: enabled, single file,
    unrestricted type/size/count, no capture. accept uses HTML accept tokens:
    exact MIME, MIME wildcard (an image type followed by slash-star), or dot extension (.tar.gz), matched
    case-insensitively. Comma-separated entries are also allowed. Empty tokens
    are ignored; malformed nonempty tokens do not match. Limits are per batch;
    invalid files do not consume count. multiple=false caps count at one.
    Negative limits clamp to zero. Size/count equality is allowed. */
let init:
  (
    ~id: string,
    ~disabled: bool=?,
    ~multiple: bool=?,
    ~accept: list(string)=?,
    ~capture: capture=?,
    ~max_size: float=?,
    ~max_count: int=?,
    unit
  ) =>
  model;
/** Whether any nested file drag enters remain outstanding. */
let dragging_over: model => bool;
/** Current disabled setting. */
let disabled: model => bool;
/** Programmatic enable/disable; clears drag depth, emits no outcomes. */
let set_disabled: (bool, model) => (model, Tea_core.Cmd.t(msg));
/** Clear drag feedback without emitting outcomes (e.g. after removing a target).
    Native drag cancellation normally delivers dragleave. */
let reset_dragging: model => (model, Tea_core.Cmd.t(msg));
/** Apply an intent using current state. File selection/drop resets depth;
    disabled models ignore all intents. Empty/cancelled selections emit nothing. */
let update: (msg, model) => (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Required delegated document listeners, scoped by containment to the part
    ids and keyed by (File_drop.events.EVENT, id). The latest model is used
    without restarting listeners. Picker .click() and FileList snapshots run
    synchronously in the native event stack; no post-render picker command.
    Cleanup is synchronous when unsubscribed/shutdown. Events outside the
    target and non-file drags are untouched. No locks or global drop blocking.
    Do not stop propagation before events reach document. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** Attribute bundles typed in the parent's message type. */
type parts('parent) = {
  /** Apply to a container enclosing the button/input. Owns id and dragenter,
      dragover, dragleave, drop through delegation; do not stack those handlers.
      File dragenter/over/drop prevent default; leave does not. */
  root: list(Tea_html.Html.attr('parent)),
  /** Apply only to a native button with a visible/accessibly named label.
      Owns id, type, disabled, aria-controls, click through delegation. Native
      keyboard activation is untouched; never render a neutral substitute. */
  button: list(Tea_html.Html.attr('parent)),
  /** Apply to input. Owns id, type=file, hidden, tabindex, disabled, multiple,
      accept, capture, change through delegation. Hidden by DOM property, not
      CSS. Its value is cleared after snapshot so the same file can be chosen
      again. Do not give it a controlled value or override these facts. */
  input: list(Tea_html.Html.attr('parent)),
  /** Derived drag state for optional instructional content. */
  dragging_over: bool,
};
/** Render without a wrapper. Derived ids are id-root, id-button and id-input.
    Map update commands/subscriptions with the same to_parent function.
    Append only nonconflicting attributes. Keep targets disjoint (not nested).
    No files are accepted from paste; validate again on the server. */
let view:
  (
    ~model: model,
    ~to_parent: msg => 'parent,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
