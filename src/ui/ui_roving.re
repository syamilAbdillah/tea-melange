/* Keyed items with one roving tab stop, shared by Radio_group and Tabs. */
type item = {
  key: string,
  label: string,
  disabled: bool,
};
let same = Option.equal(String.equal);
let index = (items: array(item), key) => {
  let rec find = i =>
    i >= Array.length(items)
      ? (-1) : same(Some(items[i].key), key) ? i : find(i + 1);
  find(0);
};
let flags = items => Array.map((item: item) => item.disabled, items);
let key_at = (items: array(item), i) => Option.map(i => items[i].key, i);
/** The key when it names an enabled item. */
let valid = (items: array(item), key) => {
  let i = index(items, key);
  i >= 0 && !items[i].disabled ? key : None;
};
let is_valid = (items, key) => Option.is_some(valid(items, Some(key)));
let first = items => key_at(items, Ui_nav.first(flags(items)));
let last = items => key_at(items, Ui_nav.last(flags(items)));
/** Next/previous enabled key from [from], wrapping. */
let step = (items, ~from, delta) => {
  let from = index(items, from);
  key_at(
    items,
    delta > 0
      ? Ui_nav.next(~from, flags(items)) : Ui_nav.prev(~from, flags(items)),
  );
};
/** Whether a focusout's relatedTarget id is the root or one of its items. */
let inside = (~id, items: array(item), target) =>
  same(target, Some(id))
  || Array.exists(
       (item: item) => same(target, Some(Ui_attr.item_id(id, item.key))),
       items,
     );
