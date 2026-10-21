# js-framework-benchmark style suite (headless Chrome)

A keyed 1,000/10,000-row table with the standard jsfb row shape and operations
(create, replace, partial update, select, swap, remove, append, clear), in two
views: `baseline` rebuilds every row; `lazy` wraps rows in `Html.lazy2`.

```sh
opam exec -- dune build @bench/jsfb/melange
node bench/jsfb/run.mjs 10          # samples per operation (fresh page each)
PROFILE="create 10k" node bench/jsfb/run.mjs   # hottest functions, self time
```

Requires Google Chrome (`CHROME=/path/to/chrome` to override). Each sample is
timed from the click to the end of a forced layout in the frame that patches
the DOM; idle time before that frame is excluded. `js` columns exclude the
layout, i.e. update + view + diff + patch. Not part of `@bench`, which must run
without a browser.
