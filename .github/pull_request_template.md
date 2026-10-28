<!--
The title is a conventional commit with a scope (feat(theme): …, fix(cadran): …): CI checks it, and the squash merge
makes it the commit release-please writes the changelog from. A breaking change says so (feat(view)!: …).
-->

## What this changes, and why

<!-- The diff says what; this says what it is for. Link the issue it closes, if any. -->

## How it was verified

<!-- The tests added or run; the platforms tried beyond CI (a console, Windows, macOS…). -->

## Checklist

- [ ] `ctest` passes (desktop build with `-DAURORE_BUILD_TESTS=ON`)
- [ ] New behaviour has a test; a fix has a test that fails without it
- [ ] Changed screenshots in `tests/references/` were looked at and are meant
- [ ] Files touched are formatted (`clang-format-18`)
- [ ] The public API still builds for PlayGuard and PlayShelf, or the break is in the title (`!`) and the body
- [ ] Docs (`docs/`, `README.md`) follow a change app authors can see

Merge: squash
