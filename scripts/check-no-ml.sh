#!/usr/bin/env sh
# ADR-03: Reason is the only source language. Reject hand-written .ml/.mli outside _build.
set -eu
found=$(find src examples bench test -type f \( -name '*.ml' -o -name '*.mli' \) 2>/dev/null || true)
if [ -n "$found" ]; then
  echo "error: hand-written .ml/.mli files are not allowed (ADR-03):" >&2
  echo "$found" >&2
  exit 1
fi
