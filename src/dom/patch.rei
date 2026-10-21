/** Apply a sorted Diff plan in old pre-order index space, returning the
    possibly replaced root. Root must come from Render.create or a prior apply.
    One span-skipping walk resolves child references before parent mutations;
    keyed reuse moves existing nodes. Controlled form properties are restored
    even when a user's edit did not change the virtual facts. */
let apply: (Dom.node, array(Tea_vdom.Diff.patch('msg))) => Dom.node;
