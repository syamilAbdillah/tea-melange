/** Headless actions-menu tree using DOM focus on menus and aria-activedescendant.
    APG: https://www.w3.org/WAI/ARIA/apg/patterns/menu-button/
    and https://www.w3.org/WAI/ARIA/apg/patterns/menubar/

    Keyboard table:
    - Trigger Enter/Space/Down: open first enabled; Up: open last item.
    - Menu Down/Up: wrap through items, including disabled; Home/End: endpoints.
    - Printable characters (including surrogate pairs): timestamp-based,
      lowercase/NFD/accent-folded typeahead with explicit ligature folds.
    - Enter: activate and close tree; Space: activate, but keep checkbox/radio open.
    - Forward arrow (Right in LTR, Left in RTL), Enter/Space: open submenu.
    - Back arrow or Escape in submenu: close that branch, focus parent menu.
    - Escape in root or open trigger: close tree, focus trigger.
    - Claimed Escape stops propagation to enclosing overlays; closed-trigger
      Escape, modifiers, composing events and unhandled keys propagate.
      Other claimed keys also propagate (arrows, Enter, Space, Tab).
    - Tab/Shift+Tab: close tree, native traversal from trigger; never prevented.
    - Ctrl/Alt/Meta chords and composition: unchanged.

    Presence data: data-open, data-active, data-disabled, data-checked;
    Animation adds data-closed/enter/leave/transition, Anchor data-placement and
    data-reference-hidden. No CSS is needed. Apply every panel, including Closed
    panels, and keep leaving panels mounted. Menus/items must have no interactive
    descendants or extra tab stops. Keys must be unique throughout the tree. */

/** Writing direction, including submenu opening and closing arrows. */
type direction =
  | Ltr
  | Rtl;
/** A stable-keyed descriptor. Collections belong to the parent, not the model. */
type item = {
  /** Globally unique stable identity; ids are safely encoded from this key. */
  key: string,
  /** Accessible name and typeahead text, excluding decorative shortcut hints. */
  label: string,
  /** Remains navigable; activation and submenu opening are suppressed. */
  disabled: bool,
  /** Action, toggle, radio member, submenu, labelled group, or separator. */
  kind,
}
/** Radio members name their containing Group's key. Group labels are rendered
    using item_parts.label; Group children remain in the containing menu. */
and kind =
  | Action
  | Checkbox
  | Radio(string)
  | Submenu(array(item))
  | Group(array(item))
  | Separator;
/** Reason for a user-initiated whole-tree close. */
type close_reason =
  | Escape
  | Tab
  | Outside
  | Trigger
  | Activation;
/** Immutable, abstract tree lifecycle and domain state. */
type model;
/** Opaque event intent or subcomponent progress. */
type msg;
/** Domain changes precede Closed; submenu changes do not emit Opened/Closed. */
type out_msg =
  | Selected(string)
  | Checked(string, bool)
  | Radio_selected(string, string)
  | Opened
  | Closed(close_reason);
/** Initially closed. Defaults: Ltr, top_layer=true, hover_delay=220 milliseconds,
    no checked keys, no radio selections. Radio associations are (group,key).
    id is a document-unique prefix. hover_delay is nonnegative and finite. */
let init:
  (
    ~id: string,
    ~dir: direction=?,
    ~top_layer: bool=?,
    ~hover_delay: float=?,
    ~checked: list(string)=?,
    ~radio: list((string, string))=?,
    unit
  ) =>
  model;
/** Whether the root accepts interaction (false throughout Leaving). */
let is_open: model => bool;
/** Lifecycle phase; omit submenu for the root. */
let phase: (~submenu: string=?, model) => Animation.phase;
/** Active stable key; omit submenu for root. */
let active_key: (~submenu: string=?, model) => option(string);
/** Last action selection; checkbox/radio activation does not replace it. */
let selected: model => option(string);
/** Programmatic action selection, without messages. */
let set_selected: (option(string), model) => model;
/** Checkbox state by key, including keys not currently in the collection. */
let checked: (~key: string, model) => bool;
/** Programmatic checkbox state, without messages. */
let set_checked: (~key: string, bool, model) => model;
/** Selected member key of a radio group, if any. */
let radio_value: (~group: string, model) => option(string);
/** Programmatic radio selection; None clears the group. No messages. */
let set_radio: (~group: string, ~key: option(string), model) => model;
/** Open first enabled item, or first navigable if all disabled. Idempotent;
    reverses Leaving. Programmatic controls do not emit out messages. */
let open_: (~items: array(item), model) => (model, Tea_core.Cmd.t(msg));
/** Close whole tree and return focus to trigger, without out messages. */
let close: model => (model, Tea_core.Cmd.t(msg));
/** Repair missing active keys against the current collection, then interpret
    the intent. Retain the returned model and map commands into the parent. */
let update:
  (~items: array(item), msg, model) =>
  (model, Tea_core.Cmd.t(msg), list(out_msg));
/** Forward throughout Leaving. Owns outside pointerdown, cancellable hover
    delay, Anchor/Animation sources and applicable focus restoration. */
let subscriptions: model => Tea_core.Sub.t(msg);
/** One entry in a panel; Group children are recursive but submenu panels are
    separate entries in parts.panels. Render neutral divs, not native buttons. */
type item_parts('parent) = {
  /** Original parent descriptor, for custom icons and shortcut hints. */
  item,
  /** Owns id, role, ARIA, tabindex=-1, data flags; click, pointermove and
      pointerdown (prevents focus loss only). No keydown handler. */
  attrs: list(Tea_html.Html.attr('parent)),
  /** Group's visible label attrs; empty on other kinds. */
  label: list(Tea_html.Html.attr('parent)),
  /** Group members; empty on other kinds. */
  children: array(item_parts('parent)),
  /** Whether this is the panel's active descendant. */
  active: bool,
  /** Derived checkbox/radio state. False for other kinds. */
  checked: bool,
};
/** A separately rendered floating menu, root first in depth-first order. */
type panel_parts('parent) = {
  /** None identifies root; Some(key) identifies a submenu's owning item. */
  owner: option(string),
  /** Owns id, role, hidden, tabindex=-1, labelledby, activedescendant, dir,
      data flags, Anchor geometry/popover and Animation flags. Owns keydown,
      pointerenter and pointerleave. Append only nonconflicting facts. */
  attrs: list(Tea_html.Html.attr('parent)),
  /** Direct entries, with groups retained for correct radio semantics. */
  items: array(item_parts('parent)),
  /** Accepts interaction; distinct from animation visibility. */
  open_: bool,
  /** Closed alone sets hidden=true. */
  phase: Animation.phase,
};
/** Wrapper-free rendering. Render panels as siblings, outside menuitem nodes,
    preserving their order after the trigger. All panels stay mounted. */
type parts('parent) = {
  /** Native button attrs; owns click, keydown, keyup and button/ARIA semantics.
      aria-controls references the always-rendered root panel. */
  trigger: list(Tea_html.Html.attr('parent)),
  /** Root and every submenu in collection order, including closed panels. */
  panels: array(panel_parts('parent)),
};
/** Build parent-message-typed parts. Unhandled keyboard hooks see key strings,
    are called only for plain noncomposing events, and never prevent default.
    Required keys, including Tab, take precedence. */
let view:
  (
    ~model: model,
    ~items: array(item),
    ~to_parent: msg => 'parent,
    ~on_trigger_key_down: string => option('parent)=?,
    ~on_menu_key_down: string => option('parent)=?,
    ~to_view: parts('parent) => Tea_html.Html.t('parent),
    unit
  ) =>
  Tea_html.Html.t('parent);
