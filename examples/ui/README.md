# UI examples

Add a standalone `examples/ui/<name>/` directory with `main.re`, `dune` and
`index.html`. No shared-file edits or registry are needed. Copy
[Button's Dune pattern](button/dune), replacing only the page content. It emits
ES modules and copies HTML to the emission root; `dune build @examples/melange`
recursively discovers the alias, as does `dune build`.

Each HTML document needs lang, charset, viewport, title, `<base href="/">`,
a mount point, an import map and the emitted entry script:

```html
<script type="module" src="./examples/ui/your_name/main.mjs"></script>
```

Use [Button's complete import map](button/index.html). Include **both**
`"tea.browser/": "./node_modules/tea.browser/"` and
`"tea.ui/": "./node_modules/tea.ui/"` alongside all existing tea/Melange imports.
The bare-package map is necessary even for modules imported transitively.
Include a data favicon or a real favicon asset so console-error checks do not
see an unrelated 404. Assets belong at the emission root.

```sh
dune build @examples/melange
node scripts/serve.js ui/button
```

Open http://127.0.0.1:8000/. The same shared server is used by Chromium tests;
it serves the entire emitted dependency tree with correct module MIME types
and extensionless SPA fallback, never masking missing module files.

Examples are runnable documentation: show every option and relevant disabled,
read-only/RTL states. Keep CSS local and minimal, demonstrate the data hooks,
provide visible focus and sufficient contrast, and honor reduced-motion
preferences when adding transitions. CSS is presentation, never behavior.
All components belong in a main landmark with accessible names and labels.
