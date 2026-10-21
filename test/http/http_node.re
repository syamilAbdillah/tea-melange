module H = Tea_http.Http;
module D = Tea_core.Decode;
module T = Tea_core.Task;
type timer;
external later: (unit => unit, int) => timer = "setTimeout";
external clear: timer => unit = "clearTimeout";
let install: unit => unit = [%mel.raw
  {|function() {
  var original = globalThis.fetch;
  globalThis.fetch = function(url, options) {
    if (url.startsWith('data:')) return original(url, options);
    if (url.endsWith('/network')) return Promise.reject(new TypeError('network unavailable'));
    if (url.endsWith('/timeout')) return new Promise(function(resolve,reject) { options.signal.addEventListener('abort', function() { reject(new Error('aborted')); }); });
    if (url.endsWith('/late')) return new Promise(function(resolve) { setTimeout(function() { resolve(new Response('late')); }, 50); });
    if (url.endsWith('/status')) return Promise.resolve(new Response('not found', {status:404}));
    if (url.endsWith('/post')) {
      if (options.method !== 'POST' || options.body !== 'payload' || !options.headers.some(function(h) { return h[0].toLowerCase() === 'content-type' && h[1] === 'text/plain'; })) throw Error('bad request options');
      return Promise.resolve(new Response(options.body));
    }
    return Promise.resolve(new Response('{"n":42}'));
  };
}|}
];
let check = (decoder, value) =>
  assert(
    H.Json.decode_value(decoder, H.Json.of_value(value))
    == D.run(decoder, value),
  );
let () = {
  let value: D.Value.t =
    Object([
      ("s", String("hi")),
      ("n", Number(42.)),
      ("b", Bool(true)),
      ("xs", Array([|Number(1.), Number(2.)|])),
      ("null", Null),
    ]);
  assert(H.Json.to_value(H.Json.of_value(value)) == value);
  check(D.value, value);
  check(D.field("s", D.string), value);
  check(D.field("n", D.int), value);
  check(D.field("n", D.float), value);
  check(D.field("b", D.bool), value);
  check(D.field("null", D.null(5)), value);
  check(D.field("xs", D.list(D.int)), value);
  check(D.field("xs", D.array(D.int)), value);
  check(D.at(["xs"], D.index(1, D.int)), value);
  check(D.field("xs", D.index(-1, D.int)), value);
  check(D.field("xs", D.index(10, D.int)), value);
  check(D.optional(D.string), value);
  check(D.optional(D.field("missing", D.string)), value);
  check(
    D.one_of([D.string, D.map(string_of_int, D.field("n", D.int))]),
    value,
  );
  check(D.one_of([D.string, D.fail("oops")]), value);
  check(
    D.map2(
      (a, b) => a ++ string_of_int(b),
      D.field("s", D.string),
      D.field("n", D.int),
    ),
    value,
  );
  check(D.and_then(n => D.succeed(n + 1), D.field("n", D.int)), value);
  check(D.field("missing", D.string), value);
  check(D.field("toString", D.string), value);
  let special: D.Value.t = Object([("__proto__", String("safe"))]);
  assert(H.Json.to_value(H.Json.of_value(special)) == special);
  check(D.field("__proto__", D.string), special);
  check(D.string, value);
  check(D.int, Number(1.5));
  check(D.bool, Bool(false));
  assert(
    H.Json.decode_value(D.int, Js.Json.number(2147483648.))
    == Error("expected an integer, got a number"),
  );
  assert(
    switch (H.decode_json(D.string, "{")) {
    | Error(H.Bad_body(_)) => true
    | _ => false
    },
  );
  assert(
    switch (H.decode_json(D.int, "\"no\"")) {
    | Error(H.Bad_body(_)) => true
    | _ => false
    },
  );
  install();
  let watchdog = later(() => failwith("HTTP tests timed out"), 2000);
  let tests = [
    (
      H.get_task(~url="data:text/plain,hello", ()),
      result => assert(result == Ok("hello")),
    ),
    (
      H.get_task(~url="not a url", ()),
      result => assert(result == Error(H.Bad_url("not a url"))),
    ),
    (
      H.get_task(~url="https://test/network", ()),
      result => assert(result == Error(H.Network_error)),
    ),
    (
      H.get_task(~url="https://test/status", ()),
      result => assert(result == Error(H.Bad_status(404))),
    ),
    (
      H.get_task(~url="https://test/timeout", ~timeout=10., ()),
      result => assert(result == Error(H.Timeout)),
    ),
    (
      H.post_task(
        ~url="https://test/post",
        ~body=H.string_body("text/plain", "payload"),
        (),
      ),
      result => assert(result == Ok("payload")),
    ),
  ];
  let tasks =
    List.map(
      ((task, check), done_) =>
        task(result => {
          check(result);
          done_(Ok());
        }),
      tests,
    );
  T.sequence(
    tasks,
    result => {
      assert(result == Ok(List.map(_ => (), tests)));
      let count = ref(0);
      H.get_task(
        ~url="https://test/late",
        ~timeout=5.,
        (),
        result => {
          assert(result == Error(H.Timeout));
          count := count^ + 1;
        },
      );
      let command =
        H.get(
          ~url="https://test/json",
          ~expect=H.expect_json(result => result, D.field("n", D.int)),
        );
      switch (command) {
      | Tea_core.Cmd.Effect(run) => run(result => assert(result == Ok(42)))
      | _ => assert(false)
      };
      let command =
        H.get(
          ~url="https://test/json",
          ~expect=H.expect_whatever(result => result),
        );
      switch (command) {
      | Tea_core.Cmd.Effect(run) => run(result => assert(result == Ok()))
      | _ => assert(false)
      };
      ignore(
        later(
          () => {
            assert(count^ == 1);
            clear(watchdog);
            print_endline(
              "http: raw JSON interpreter, real data fetch, mocked failures, timeout and commands passed",
            );
          },
          90,
        ),
      );
    },
  );
};
