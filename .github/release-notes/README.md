# Release summaries

`X.Y.Z.md` (no `v`) is the hand-written summary of a release. When the
`chore(main): release X.Y.Z` pull request is merged, the release-please
workflow puts it above release-please's lists on the release page. No file for
a version: the page keeps release-please's lists alone.

Write it in a pull request while the release pull request is open (its title
gives the version) and merge it before that one. Start with a `### ` heading, a
line or two, then a few bullets of what an app author notices, not one line per
pull request. `CHANGELOG.md` stays release-please's.
