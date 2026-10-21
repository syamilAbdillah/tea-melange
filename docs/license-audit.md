# License audit: melange-tea (gate P0)

Date: 2026-10-21

| Source | Says |
|---|---|
| `melange-tea.opam` | `license: "MIT"` |
| `LICENSE` | "Copyright (C) 2016-2017 OvermindDL1 … GNU **Lesser** General Public License v3 or later", plus a static-linking exception |
| `COPYING` / `COPYING.LESSER` | Full GPL-3 and LGPL-3 texts |

The opam metadata contradicts the repository's own license file. The file in the
repository wins, so the code is LGPL-3 (with exception), not MIT.

## Decision

Fallback from the plan's risk table applies: **no code is vendored or ported
from melange-tea.** tea-melange (MIT) is written from scratch, using only
public algorithm descriptions:

- Elm `VirtualDom.js` (BSD-3-Clause) — pre-order indexed diff, keyed diff,
  thunk comparison, `descendants_count` skipping. Algorithm reimplemented, not
  copied.
- Elm `Browser.js` — `_Browser_makeAnimator` state machine (NO_REQUEST →
  PENDING → EXTRA_REQUEST).

ADR-02 ("port the pure parts") is therefore superseded: everything is a rewrite.
