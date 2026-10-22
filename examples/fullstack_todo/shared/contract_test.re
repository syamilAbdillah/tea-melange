open Todo_shared;
module D = Tea_core.Decode;
module V = Tea_core.Decode.Value;

let equal = (label, actual, expected) =>
  if (actual != expected) {
    failwith(label);
  };

let todo: Contract.todo = {
  id: 7,
  title: "Write the contract",
  completed: true,
};
equal(
  "todo round-trips",
  D.run(Contract.Decode.todo, Contract.Encode.todo(todo)),
  Ok(todo),
);
equal(
  "list round-trips",
  D.run(Contract.Decode.todos, Contract.Encode.todos([todo, todo])),
  Ok([todo, todo]),
);

let patch: Contract.patch = {
  title: None,
  completed: Some(false),
};
equal(
  "absent patch fields stay absent",
  Contract.Encode.patch(patch),
  V.Object([("completed", V.Bool(false))]),
);
equal(
  "patch round-trips",
  D.run(Contract.Decode.patch, Contract.Encode.patch(patch)),
  Ok(patch),
);
equal(
  "null patch field means unchanged",
  D.run(Contract.Decode.patch, V.Object([("title", V.Null)])),
  Ok({
    title: None,
    completed: None,
  }),
);
equal(
  "mistyped patch field is rejected, not ignored",
  Result.is_error(
    D.run(
      Contract.Decode.patch,
      V.Object([("completed", V.String("yes"))]),
    ),
  ),
  true,
);

equal("titles are trimmed", Contract.validate_title("  milk "), Ok("milk"));
equal(
  "blank titles are rejected",
  Result.is_error(Contract.validate_title("   ")),
  true,
);
equal("paths", Contract.Path.todo(3), "/api/todos/3");
