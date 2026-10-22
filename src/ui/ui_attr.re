module Attr = Tea_html.Attr;
let data_flag = (name, enabled) => enabled ? Attr.data(name, "") : Attr.none;
let aria_bool = (name, value) => Attr.aria(name, value ? "true" : "false");
let aria_ids = (name, ids) => {
  let ids = List.filter(id => !String.equal(id, ""), ids);
  switch (ids) {
  | [] => Attr.none
  | _ => Attr.aria(name, String.concat(" ", ids))
  };
};
let part_id = (id, part) => id ++ "-" ++ part;
let safe_key = key => {
  let hex = "0123456789abcdef";
  let out = Buffer.create(String.length(key));
  String.iter(
    c =>
      switch (c) {
      | 'a' .. 'z'
      | 'A' .. 'Z'
      | '0' .. '9'
      | '-' => Buffer.add_char(out, c)
      | _ =>
        let code = Char.code(c);
        Buffer.add_char(out, '_');
        Buffer.add_char(out, hex.[code lsr 12 land 15]);
        Buffer.add_char(out, hex.[code lsr 8 land 15]);
        Buffer.add_char(out, hex.[code lsr 4 land 15]);
        Buffer.add_char(out, hex.[code land 15]);
      },
    key,
  );
  String.equal(key, "") ? "_" : Buffer.contents(out);
};
let item_id = (id, key) => part_id(id, "item-" ++ safe_key(key));
let without = (name, attrs) =>
  List.filter(
    fact =>
      switch (fact) {
      | Tea_vdom.Vdom.Property(key, _)
      | Tea_vdom.Vdom.Attribute(key, _)
      | Tea_vdom.Vdom.Event(key, _) => !String.equal(key, name)
      | _ => true
      },
    attrs,
  );
let unique_keys = keys =>
  List.rev(
    List.fold_left(
      (seen, key) =>
        List.exists(String.equal(key), seen) ? seen : [key, ...seen],
      [],
      keys,
    ),
  );
