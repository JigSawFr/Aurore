<!--
Title: a Conventional Commit with a scope, e.g. feat(cadran): …, fix(theme): …. CI checks it, and the squash merge
makes it the changelog entry release-please writes. A breaking change uses `!` (feat(view)!: …) and a `BREAKING CHANGE:` paragraph.
-->

## ✨ At a glance

<!-- One or two sentences: what changes for someone using or building Aurore. -->

## 💡 Why

<!-- The problem or the goal. Link the issue it closes, if any (Closes #…). -->

## 🔧 What changes

<!-- The diff says what; this says what it is for and what to look at first. -->

## 🖼️ Before / After

<!-- Only when the change is visible. Screenshots of the same screen, before and after, at the same size. -->

| Before | After |
|---|---|
| | |

## ✅ Checked

<!-- CI already runs format, REUSE licences, the desktop tests and the platform builds: only what CI does not check goes here. -->

- [ ] New behaviour has a test; a fix has a test that fails without it
- [ ] Changed screenshots in `tests/references/` were looked at and are meant
- [ ] Public API still builds for PlayGuard and PlayShelf, or the break is in the title (`!`) and the body
- [ ] Docs (`docs/`, `README.md`) follow a change app authors can see

Verified on: <!-- platforms run beyond CI (a console, Windows, macOS…), or "CI only". -->

Merge: squash
