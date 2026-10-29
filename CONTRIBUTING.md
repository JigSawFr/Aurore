# Contributing to Aurore

Thanks for helping. Aurore is a fork of [borealis](https://github.com/xfangfang/borealis) (its `wiliwili` branch) used by
[PlayGuard](https://github.com/JigSawFr/playguard) and [PlayShelf](https://github.com/JigSawFr/playshelf): a change here
reaches both apps, so it comes with its tests and keeps the public API working (add, deprecate, then remove).

## Build and test

```sh
git clone --recursive https://github.com/JigSawFr/aurore.git && cd aurore
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DAURORE_PLATFORM=DESKTOP -DAURORE_WINDOW=GLFW \
      -DAURORE_LIBROMFS=ON -DAURORE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

With CMake 3.25 or later, `cmake --workflow --preset tests` does the same in `build/tests`. The build configurations
other jobs do not cover (install and `find_package`, `AURORE_FETCH_DEPS`, FreeType and FriBidi, the unity build, the
template) are in `tools/ci_variant.sh`, which the `linux (variants)` job runs.

On Linux the tests draw with Mesa's software renderer and need no display (`libegl1 libegl-mesa0 libgl1-mesa-dri`).
A compiler cache speeds rebuilds up: add `-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`.
The demo is `./build/aurore_demo`; [`docs/testing.md`](docs/testing.md) explains the harness, the references and the
bench.

- **Screenshots**: a test that draws compares with `tests/references/`. When a change alters a screen on purpose, run
  the test with `AURORE_TEST_UPDATE=1`, look at the new pictures, and commit them with the change. In CI a missing
  reference fails.
- **Bench**: `ctest` checks the main loop's counts against `tests/bench_baseline.json`. A change that costs more on
  purpose updates it with `aurore_bench --write-baseline tests/bench_baseline.json` and says why.
- **Format**: `clang-format-18 -i <files>` on what you touch (`.clang-format`); CI checks every file outside `extern/`.
  Reformatting commits are listed in `.git-blame-ignore-revs` (`git config blame.ignoreRevsFile .git-blame-ignore-revs`).
- **Other platforms** (Switch, PS Vita, PS4, Android, iOS, Windows, macOS) are built by CI on every pull request. The macOS and iOS jobs are not required: GitHub's macOS runners sometimes leave them without a machine until they are cancelled, so their failure does not fail the run; a failure that comes from the change is still fixed.

## Pull requests

- One change per pull request, from a branch named `type/subject` (`fix/rail-focus`, `feat/theme-particles`).
- The title is a [Conventional Commit](https://www.conventionalcommits.org/) with a scope (`fix(theme): …`,
  `feat(cadran): …`): it is checked, it becomes the squash commit, and release-please writes `CHANGELOG.md` from it.
  A breaking change says so (`feat(view)!: …` and a `BREAKING CHANGE:` paragraph).
- New behaviour comes with a test; a fix with a test that fails without it; a visible change with the docs
  (`docs/`, `README.md`).
- Merges: **squash** for a change of ours; **merge** for picks from upstream or other forks, which keep their author
  and their `cherry picked from` line. No rebase of a shared branch.
- A fix that is not specific to Aurore is worth sending [upstream](https://github.com/xfangfang/borealis) too.

## Licences

borealis' code is Apache 2.0, Cadran (`library/lib/cadran`, `library/include/aurore/cadran`) is GPLv3: a new file
takes the licence of the part it belongs to. It starts with two SPDX tags, as its neighbours do:

```cpp
// SPDX-FileCopyrightText: 2026 Your Name
// SPDX-License-Identifier: Apache-2.0
```

(`GPL-3.0-or-later` in Cadran; `#` in a script or a CMake file). A file that cannot hold a comment (an image, a font,
JSON) is covered by [`REUSE.toml`](REUSE.toml), as is third-party code, which goes in `extern/` with its licence, in
[`NOTICE`](NOTICE) and in `resources/THIRD_PARTY_NOTICES.md`. CI runs [`reuse lint`](https://reuse.software)
(`pip install reuse`, then `reuse lint`): a file whose licence cannot be told fails it.

## Security

Do not open a public issue for a vulnerability: see [`SECURITY.md`](SECURITY.md).
