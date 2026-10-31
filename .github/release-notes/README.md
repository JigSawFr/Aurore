# Release summaries

`X.Y.Z.md` (no `v`) is the hand-written summary of a release. When the
`chore(main): release X.Y.Z` pull request is merged, the release-please
workflow lays out the release's page (`tools/release_page.sh`,
`tools/release_notes.py`), as PlayGuard's is, under the title `Aurore X.Y.Z`
and the heading `## X.Y.Z · <date>`:

1. a line of links (the source, the documentation, the migration guide) and
   the **figures**: breaking changes, features, fixes, security, performance,
   the commits;
2. the **summary**: `<version>.md` from this folder, else none. Each bullet's
   bold lead-in sits on a line of its own, its text under it; a paragraph after
   the last bullet of a part becomes an *Important* box. A `### ` heading
   starts a new part, with its own bullets and its own closing paragraph;
3. **what changed**: release-please's lists, folded: what app authors notice
   (breaking changes, features, fixes, security, performance, refactorings),
   then **for contributors** (build, tests, docs);
4. **📦 Use it**: the submodule at the tag, the migration guide, the install,
   and the assets with their size: `aurore-template.zip` (a minimal app) and
   `aurore-api.zip` (the API reference as web pages), attached by the `assets`
   job with `SHA256SUMS.txt` and the build provenance;
5. **🔍 Build**: the CI run of the tagged commit, the checksums and the
   attestation (`gh attestation verify aurore-api.zip -R JigSawFr/Aurore`),
   and the contributors, `@`-mentioned.

`CHANGELOG.md` stays release-please's, whole.

Write the summary in a pull request while the `chore(main): release X.Y.Z`
pull request is open (its title gives the version), and merge it before that
one. Start with a `### ` heading, a line or two and a few bullets: what an app
author notices, not one line per pull request. A bullet reads best as a card:

    * 🎛️ **Cadran**: the only look, …

## An existing release

Actions › **release-page** › Run workflow, with its tag (empty: the latest):
the page as the release job would lay it out, on the run page only; tick
*apply* to update the release. A way to lay out a release published before
this existed, or to add a summary merged after its release. A page laid out
before is laid out again from its folded lists, with the file's summary if
there is one, else the one already on the page; the same page is not edited.
