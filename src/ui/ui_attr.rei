/** Private attribute and collision-free id helpers. */
/** Presence data flag, or [Attr.none]. Name excludes the data- prefix. */
let data_flag: (string, bool) => Tea_html.Html.attr('msg);
/** Explicit true/false ARIA string. Name excludes the aria- prefix. */
let aria_bool: (string, bool) => Tea_html.Html.attr('msg);
/** Space-joined nonempty ids; omitted when none remain. */
let aria_ids: (string, list(string)) => Tea_html.Html.attr('msg);
/** Derive [id ++ "-" ++ part]. */
let part_id: (string, string) => string;
/** Encode UTF-16 code units outside letters/digits/hyphen as _hhhh in Melange.
    Underscores are
    escaped too so literal escape-looking keys cannot collide. Empty is "_". */
let safe_key: string => string;
/** Derive [id ++ "-item-" ++ safe_key(key)]. Never use an array index as key. */
let item_id: (string, string) => string;
/** Drop every property, attribute or event listener named [name], to
    replace part of another component's bundle. */
let without:
  (string, list(Tea_html.Html.attr('msg))) =>
  list(Tea_html.Html.attr('msg));
/** Keys without duplicates, keeping each first occurrence in order. */
let unique_keys: list(string) => list(string);
