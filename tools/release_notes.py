#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
"""The notes of a GitHub release, laid out for reading, as PlayGuard's are.

  tools/release_notes.py --version 1.0.0 --repo-url https://github.com/o/r \\
      [--summary SUMMARY] [--assets assets.json] [--compare compare.json] \\
      [--contributors logins.json] [--run-url URL] < notes.md > new-notes.md

release-please writes the release's notes from the conventional commits: a
"## <version> (<date>)" heading, then one line per commit under "### "
sections. This lays out, under "## <version> · <date>":

  a line of links (the source, the documentation, the migration guide) and a
  line of figures (breaking changes, features, fixes, security, performance,
  the commits), in HTML;
  the summary (.github/release-notes/<version>.md), when there is one: its
  bullets on two lines (the bold lead-in, then the text), a closing paragraph
  after the last bullet of a section as an "Important" box;
  "What changed": the sections app authors notice, then those for
  contributors (build, tests, docs), each folded;
  "## 📦 Use it": the submodule at the tag, the migration guide, the install,
  the template and the API reference with their size;
  "## 🔍 Build": the CI run of the tagged commit, the checksums and the
  attestation, the contributors (mentioned).

assets.json is `gh release view --json assets`; compare.json the GitHub API's compare of the previous tag with this one
(its commits give the contributors); logins.json a list of logins, the
contributors of a first release. Each is optional: what it feeds is left out
without it.

Notes laid out before are laid out again from the lists in their <details>
blocks, keeping their summary unless another is given: the same input gives
the same page, so a re-run edits nothing. Standard library only.
"""
import argparse
import json
import re
import sys

# release-please-config.json's sections: these are for contributors, the
# others for everyone who builds on Aurore.
CONTRIBUTOR_SECTIONS = {"Build System": "build", "Tests": "tests", "Documentation": "docs"}
# The figures line, by section title: (emoji, singular, plural).
FIGURES = {
    "BREAKING CHANGES": ("⚠️", "breaking change", "breaking changes"),
    "Features": ("🚀", "feature", "features"),
    "Bug Fixes": ("🐞", "fix", "fixes"),
    "Security": ("🛡️", "security fix", "security fixes"),
    "Performance": ("⚡", "performance change", "performance changes"),
}
# The website (site/): its links say where the visit comes from, for the
# audience report: the release notes, of this release.
SITE_URL = "https://aurore.boreale.dev/"
UTM = "?utm_source=release&utm_medium=release-notes&utm_campaign={tag}"

USE_IT = """## 📦 Use it

1. **In an app**: add Aurore as a submodule at this release, then follow [Getting started]({repo}/blob/{tag}/docs/getting-started.md).

   ```
   git submodule add -b main https://github.com/JigSawFr/aurore.git library/aurore
   git -C library/aurore checkout {tag}
   ```

2. **Coming from borealis?** [Migrating from borealis]({repo}/blob/{tag}/docs/migrating-from-borealis.md) lists each rename, the CMake changes and what changes on screen.

3. **Installed on the desktop?** Configure Aurore with `-DAURORE_INSTALL=ON`, then `find_package(Aurore {series} REQUIRED)` and link `Aurore::aurore`. In code, `AURORE_VERSION_AT_LEAST({major}, {minor}, {patch})` tells the version.
{extras}
> [!NOTE]
> Consoles, phones, the desktop and the browser: [what runs where]({repo}/blob/{tag}/docs/platforms.md). The documentation, the API reference and the demo in a browser are on [aurore.boreale.dev]({site}{utm})."""


def strip_emoji(s: str) -> str:
    return "".join(c for c in s if ord(c) < 0x2600 or 0x2BFF < ord(c) < 0x1F000 and not 0xFE00 <= ord(c) <= 0xFE0F).strip()


def sections(text: str):
    """[(title without emoji, block text)], the text before the first "### " as ("", …)."""
    out, title, block = [], "", []
    for line in text.splitlines():
        if line.startswith("### "):
            if title or any(l.strip() for l in block):
                out.append((title, "\n".join(block).strip("\n")))
            title, block = strip_emoji(line[4:]), [line]
        else:
            block.append(line)
    if title or any(l.strip() for l in block):
        out.append((title, "\n".join(block).strip("\n")))
    return out


def items(block: str) -> int:
    return sum(1 for line in block.splitlines() if line.startswith(("* ", "- ")))


def fmt_size(n) -> str:
    if not isinstance(n, int) or n <= 0:
        return ""
    return f"{n / 1048576:.1f} MB" if n >= 1048576 else f"{max(1, round(n / 1024))} KB"


def plural(n: int, word: str) -> str:
    return f"{n} {word}{'s' if n != 1 else ''}"


def folded(label: str, what: str, body: str) -> str:
    return f"<details>\n<summary><b>{label}</b> ({what})</summary>\n\n{body}\n\n</details>"


# --- the summary, plain (as written) and for the page ----------------------

LEAD_IN = re.compile(r"^([*-] (?:\S+ )?\*\*[^*]+\*\*:)\s*(.*)$")
BOX = "> [!IMPORTANT]"


def plain(summary: str) -> str:
    """A summary as written, out of what github() makes of it: the HTML lines
    and comments dropped, an "Important" box back as a paragraph, a bullet's
    two lines joined."""
    out, comment = [], False
    for line in summary.splitlines():
        t = line.strip()
        if comment:
            comment = "-->" not in t
            continue
        if t == BOX:
            continue
        if t.startswith("<!--"):
            comment = "-->" not in t
        elif t.startswith("<") and t.endswith(">"):
            continue
        elif t.startswith("> ") and out and not out[-1].startswith("> "):
            out.append(t[2:])
        elif t.startswith("> "):
            out[-1] = out[-1] + " " + t[2:]
        elif out and LEAD_IN.match(out[-1]) and not LEAD_IN.match(out[-1]).group(2) and line.startswith("  ") and t:
            out[-1] = out[-1].rstrip() + " " + t
        else:
            out.append(line.rstrip())
    return re.sub(r"\n{3,}", "\n\n", "\n".join(out)).strip()


def github_part(part: str) -> str:
    """One part of the summary (up to the next "### " heading) for the page: a
    blank line between bullets, each bullet's bold lead-in on a line of its
    own and its text, capitalised, under it; a paragraph after the last
    bullet in an "Important" box."""
    blocks = [b for b in re.split(r"\n\s*\n", part) if b.strip()]
    last = max((i for i, b in enumerate(blocks) if b.lstrip().startswith(("* ", "- "))), default=-1)
    out = []
    for i, block in enumerate(blocks):
        if block.lstrip().startswith(("* ", "- ")):
            for b in re.split(r"\n(?=[*-] )", block):
                b = " ".join(l.strip() for l in b.splitlines())
                m = LEAD_IN.match(b)
                if m and m.group(2):
                    text = m.group(2)
                    out.append(f"{m.group(1)}  \n  {text[:1].upper()}{text[1:]}")
                else:
                    out.append(b)
        elif last >= 0 and i > last and not block.startswith("#"):
            out.append(BOX + "\n" + "\n".join("> " + l for l in block.splitlines()))
        else:
            out.append(block)
    return "\n\n".join(out)


def github(summary: str) -> str:
    """The summary for the page: each "### " heading starts a part with its
    own bullets and its own closing "Important" paragraph."""
    parts = re.split(r"\n(?=### )", plain(summary))
    return "\n\n".join(github_part(p) for p in parts if p.strip())


# --- the page ---------------------------------------------------------------

def heading(line: str) -> str:
    """ "## [1.0.0](…) (2026-10-31)" → "## [1.0.0](…) · 2026-10-31"."""
    m = re.match(r"^(## .*?) \((\d{4}-\d\d-\d\d)\)\s*$", line)
    return f"{m.group(1)} · {m.group(2)}" if m else line.rstrip()


def unlayout(notes: str):
    """(release-please's notes, the summary on the page) of notes this laid
    out before; notes it did not lay out come back as they are, with no
    summary. The lists are taken back out of their <details> blocks."""
    lines = notes.splitlines()
    if "<details>" not in lines:
        return notes, ""
    at = next((i for i, line in enumerate(lines) if line.startswith("## ")), -1)
    end = next((i for i in range(at + 1, len(lines)) if lines[i].startswith("## ")), len(lines))
    first = next((i for i in range(at + 1, end) if lines[i] == "<details>"), end)
    heading_at = next((i for i in range(at + 1, first) if lines[i] == "### 📜 What changed"), first)
    summary = plain("\n".join(lines[at + 1 : heading_at]))
    lists, inside = [], None
    for line in lines[first:end]:
        if line.startswith("<summary>"):
            inside = []
        elif line == "</details>":
            if inside is not None:
                lists.append("\n".join(inside).strip("\n"))
            inside = None
        elif inside is not None:
            inside.append(line)
    head = lines[at] if at >= 0 else ""
    return head + "\n\n" + "\n\n".join(lists) + "\n", summary


def layout(notes: str, version: str, repo: str, summary: str = "", compare=None,
           run_url: str = "", contributors=None, assets=None) -> str:
    notes, existing = unlayout(notes)
    summary = (summary or existing).strip()
    lines = notes.splitlines(keepends=True)
    at = next((i for i, line in enumerate(lines) if line.startswith("## ")), None)
    head = heading(lines[at]) if at is not None else f"## {version}"
    rest = "".join(lines[at + 1 :] if at is not None else lines)
    parts = sections(rest.strip("\n"))
    users = [(t, b) for t, b in parts if t and t not in CONTRIBUTOR_SECTIONS and items(b)]
    contrib = [(t, b) for t, b in parts if t in CONTRIBUTOR_SECTIONS]

    tag = f"v{version}"
    slug = repo.split("://", 1)[-1].split("/", 1)[-1]
    m = re.search(r"/compare/([^/\s]+?)\.\.\.", head)
    prev = m.group(1) if m else ""
    nums = (re.findall(r"\d+", version) + ["0", "0", "0"])[:3]
    utm = UTM.format(tag=tag)
    dl = f"{repo}/releases/download/{tag}"
    sizes = {a.get("name"): fmt_size(a.get("size")) for a in (assets or []) if isinstance(a, dict)}

    def size(name):
        return f" ({sizes[name]})" if sizes.get(name) else ""

    out = [head]
    out.append(f'<p>📦 <a href="{repo}/archive/refs/tags/{tag}.zip"><b>Source code</b></a> · '
               f'<a href="{SITE_URL}{utm}">documentation</a> · '
               f'<a href="{repo}/blob/{tag}/docs/migrating-from-borealis.md">migrating from borealis</a> · '
               f'<a href="#-use-it">how to use it</a></p>')
    figures = []
    for title, (emoji, one, many) in FIGURES.items():
        n = sum(items(b) for t, b in users if t == title)
        if n:
            label = f"{emoji} <b>{n}</b> {one if n == 1 else many}"
            figures.append(f'<a href="#every-change">{label}</a>' if title == "BREAKING CHANGES" else label)
    commits = (compare or {}).get("total_commits")
    if prev:
        what = plural(commits, "commit") if isinstance(commits, int) else "every commit"
        figures.append(f'<a href="{repo}/compare/{prev}...{tag}">{what} since {prev}</a>')
    else:
        figures.append(f'<a href="{repo}/commits/{tag}">every commit</a>')
    out.append("<p>" + " · ".join(figures) + "</p>")
    out.append("<hr>")
    if summary:
        out += [github(summary), "<hr>"]

    if users or contrib:
        out.append("### 📜 What changed")
        out.append('<a name="every-change"></a>')
    if users:
        out.append(folded(f"Every change in {version}", plural(sum(items(b) for _, b in users), "change"),
                          "\n\n".join(b for _, b in users)))
    if contrib:
        names = ", ".join(CONTRIBUTOR_SECTIONS[t] for t, _ in contrib)
        out.append(folded(f"🛠️ For contributors: {names}", plural(sum(items(b) for _, b in contrib), "change"),
                          "\n\n".join(b for _, b in contrib)))

    extras = ""
    if "aurore-template.zip" in sizes:
        extras += (f"\n4. **Starting from scratch?** [`aurore-template.zip`]({dl}/aurore-template.zip){size('aurore-template.zip')} "
                   "is a minimal app (one XML screen, its strings, an icon) that builds a desktop binary and a Switch `.nro`.\n")
    if "aurore-api.zip" in sizes:
        extras += (f"\n{5 if extras else 4}. **Offline?** [`aurore-api.zip`]({dl}/aurore-api.zip){size('aurore-api.zip')} "
                   "is the API reference of this release, as web pages.\n")
    out.append(USE_IT.format(repo=repo, tag=tag, extras=extras, site=SITE_URL, utm=utm, series=f"{nums[0]}.{nums[1]}",
                             major=nums[0], minor=nums[1], patch=nums[2]))

    build = []
    if run_url:
        build.append(f"[**Build report**]({run_url}) (the CI run of the tagged commit: every platform, the tests, the sanitizers)")
    if "SHA256SUMS.txt" in sizes:
        build.append("`SHA256SUMS.txt`")
        build.append(f"provenance: `gh attestation verify aurore-api.zip -R {slug}`")
    build.append(f"[Every commit]({repo}/commits/{tag})")
    people = {(c.get("author") or {}).get("login", "") for c in (compare or {}).get("commits") or []}
    if not compare:
        people |= set(contributors or [])
    people = sorted((p for p in people if p and not p.endswith("[bot]")), key=str.lower)
    section = "## 🔍 Build\n\n" + " · ".join(build)
    if people:
        section += "\n\n**Contributors**: " + ", ".join(f"@{p}" for p in people)
    out.append(section)
    return "\n\n".join(out) + "\n"


def read_json(path):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument("--version", required=True, help="1.0.0 (a leading v is dropped)")
    ap.add_argument("--repo-url", required=True, help="https://github.com/owner/repo")
    ap.add_argument("--summary", help="the summary file, if any")
    ap.add_argument("--assets", help="gh release view --json assets, for the sizes")
    ap.add_argument("--compare", help="the API's compare of the previous tag, for the contributors")
    ap.add_argument("--contributors", help="a JSON list of logins, the contributors when there is no compare")
    ap.add_argument("--run-url", default="", help="the CI run of the tagged commit, for the build report")
    ap.add_argument("--raw", action="store_true", help="only print release-please's notes, out of a laid-out page")
    ap.add_argument("--existing-summary", action="store_true", help="only print the summary a laid-out page has")
    a = ap.parse_args()
    if a.raw or a.existing_summary:
        raw, existing = unlayout(sys.stdin.read())
        sys.stdout.write(raw if a.raw else (existing + "\n" if existing else ""))
        return 0
    summary = ""
    if a.summary:
        with open(a.summary, encoding="utf-8") as f:
            summary = f.read()
    assets = (read_json(a.assets) or {}).get("assets") if a.assets else None
    compare = read_json(a.compare) if a.compare else None
    contributors = read_json(a.contributors) if a.contributors else None
    sys.stdout.write(layout(sys.stdin.read(), a.version.lstrip("v"), a.repo_url.rstrip("/"), summary,
                            compare if isinstance(compare, dict) else None, a.run_url,
                            contributors if isinstance(contributors, list) else None, assets))
    return 0


if __name__ == "__main__":
    sys.exit(main())
