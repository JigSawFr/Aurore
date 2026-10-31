#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
#
# The page of a GitHub release, laid out (tools/release_notes.py) as PlayGuard's
# is: "Aurore X.Y.Z" as its title, then the figures, the summary
# (.github/release-notes/<version>.md), the commit list folded (what app
# authors notice, then what contributors do), how to use it, the build report
# and the contributors.
#
#   tools/release_page.sh vX.Y.Z [--apply]
#
# Without --apply the new notes are only printed (and written to the job
# summary in Actions). Needs gh (GH_TOKEN, GH_REPO) and python3; run from the
# repository's root. A release laid out before is laid out again from its
# lists (a changed summary file, a new layout), keeping its summary when there
# is no file for it. The same page is not edited. Nothing here fails a
# release: a part that cannot be had is left out.
set -euo pipefail
tag=${1:?usage: tools/release_page.sh vX.Y.Z [--apply]}
apply=${2:-}
version=${tag#v}
repo_url="${GITHUB_SERVER_URL:-https://github.com}/${GH_REPO:?GH_REPO is needed}"
summary_out=${GITHUB_STEP_SUMMARY:-/dev/null}
work=$(mktemp -d)
here=$(cd "$(dirname "$0")" && pwd)

gh release view "$tag" --json body --jq .body > "$work/notes.md"
python3 "$here/release_notes.py" --version "$version" --repo-url "$repo_url" --existing-summary < "$work/notes.md" > "$work/existing.md"

args=()
summary=".github/release-notes/$version.md"
if [ -f "$summary" ]; then
  args=(--summary "$summary"); how="the hand-written summary ($summary)"
elif [ -s "$work/existing.md" ]; then
  how="the summary already on the page"
else
  how="no summary (no $summary)"
fi

# The previous release, from the compare link release-please puts in the heading.
prev=$(sed -n '1,/^## /s|.*/compare/\([^/ ]*\)\.\.\..*|\1|p' "$work/notes.md" | head -n 1)
if [ -n "$prev" ] && gh api "repos/$GH_REPO/compare/$prev...$tag" > "$work/compare.json" 2>/dev/null; then
  args+=(--compare "$work/compare.json")
elif [ -z "$prev" ] && gh api --paginate "repos/$GH_REPO/contributors" --jq '[.[] | select(.type == "User") | .login]' 2>/dev/null \
    | python3 -c 'import json, sys; d = json.JSONDecoder(); s = sys.stdin.read(); i = 0; out = []
while i < len(s):
    v, i = d.raw_decode(s, i); out += v
    while i < len(s) and s[i].isspace(): i += 1
print(json.dumps(out))' > "$work/contributors.json"; then
  args+=(--contributors "$work/contributors.json")   # a first release: everyone so far
fi

# The build report: the CI run of the tagged commit.
sha=$(gh api "repos/$GH_REPO/commits/$tag" --jq .sha 2>/dev/null || true)
run_url=
[ -z "$sha" ] || run_url=$(gh run list -w ci.yaml --commit "$sha" -L 1 --json url --jq '.[0].url // empty' 2>/dev/null || true)
[ -z "$run_url" ] || args+=(--run-url "$run_url")

python3 "$here/release_notes.py" --version "$version" --repo-url "$repo_url" "${args[@]}" < "$work/notes.md" > "$work/new.md"

if [ "$apply" != --apply ]; then
  cat "$work/new.md"
  { echo "## $tag laid out, with $how (not published)"; echo; cat "$work/new.md"; } >> "$summary_out"
  exit 0
fi

if cmp -s "$work/notes.md" "$work/new.md"; then
  echo "$tag already has this page: nothing to edit."
  echo "ℹ️ **Release page** — $tag already has this page" >> "$summary_out"
else
  gh release edit "$tag" --title "Aurore $version" --notes-file "$work/new.md"
  echo "✅ **Release page** — $tag laid out, with $how" >> "$summary_out"
fi
