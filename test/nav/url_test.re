module U = Tea_url.Url;
module P = U.Parser;
let url = s => Option.get(U.from_string(s));
let run = () => {
  List.iter(
    s => assert(U.to_string(url(s)) == s),
    [
      "https://example.com/",
      "http://example.com:8080/a%20b/?q=a+b#hi%21",
      "https://[::1]:443/x",
      "https://host/?#",
    ],
  );
  assert(U.to_string(url("https://EXAMPLE.com")) == "https://example.com/");
  List.iter(
    s => assert(U.from_string(s) == None),
    [
      "relative",
      "ftp://host/",
      "https:///",
      "https://x:70000/",
      "http://u:p@host/",
      "http://x:wat/",
      "http://x:1:2/",
      "http://[::1",
      "http://a b/",
    ],
  );
  assert(P.parse(P.top, url("http://x/")) == Some());
  assert(P.parse(P.int, url("http://x/42/")) == Some(42));
  assert(P.parse(P.int, url("http://x/no")) == None);
  assert(P.parse(P.int, url("http://x/0xff")) == None);
  assert(P.parse(P.int, url("http://x/2147483648")) == None);
  assert(P.parse(P.int, url("http://x/-12")) == Some(-12));
  assert(P.parse(P.int, url("http://x/1/2")) == None);
  assert(P.parse(P.string, url("http://x/a%20b")) == Some("a b"));
  assert(P.parse(P.string, url("http://x/%ZZ")) == None);
  assert(P.parse(P.string, url("http://x/a//")) == None);
  let route = P.map((((), n)) => n, P.slash(P.s("users"), P.int));
  assert(P.parse(route, url("http://x/users/23")) == Some(23));
  assert(
    P.parse(
      P.one_of([P.map(_ => 0, P.s("users")), route]),
      url("http://x/users/23"),
    )
    == Some(23),
  );
  assert(
    P.parse(
      P.slash(P.one_of([P.s("users"), P.s("items")]), P.int),
      url("http://x/items/7"),
    )
    == Some(((), 7)),
  );
  assert(
    P.parse(
      P.one_of([P.map(_ => 0, P.s("home")), route]),
      url("http://x/home/"),
    )
    == Some(0),
  );
  let q =
    P.slash(P.query(U.Query.string("q")), P.query(U.Query.int("n")));
  assert(
    P.parse(q, url("http://x/?q=a+b%21&n=42"))
    == Some((Some("a b!"), Some(42))),
  );
  assert(P.parse(q, url("http://x/?n=bad")) == Some((None, None)));
  assert(
    P.parse(P.fragment, url("http://x/#hello%20world"))
    == Some(Some("hello world")),
  );
  assert(P.parse(P.fragment, url("http://x/")) == Some(None));
  assert(U.percent_decode("%E2%9C%93") == Some("✓"));
  print_endline("url: roundtrips and parser cases passed");
};
