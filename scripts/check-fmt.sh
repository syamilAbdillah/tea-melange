#!/usr/bin/env sh
# Check without modifying source files. Keep the suffix for refmt's parser.
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
find src examples bench test -type f \( -name '*.re' -o -name '*.rei' \) | sort > "$tmp/files"
status=0
while IFS= read -r file; do
  formatted="$tmp/formatted.${file##*.}"
  refmt "$file" > "$formatted"
  if ! diff -u "$file" "$formatted"; then
    status=1
  fi
done < "$tmp/files"
exit "$status"
