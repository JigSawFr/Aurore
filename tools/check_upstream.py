#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
#
# Which of the components Aurore bundles or builds with have a newer release
# upstream: the versions tools/stack.py reads from the repository, against
# each project's latest stable release (no release candidate, beta or
# preview). The workflow upstream.yml runs it every week and keeps one issue
# open with what it finds. Dependabot (.github/dependabot.yml) already
# watches the GitHub Actions and the Android Gradle Plugin and Gradle.
#
#     python3 tools/check_upstream.py              the table, all components
#     python3 tools/check_upstream.py --behind     only those behind; exit 1 if any
#
# Reads jsDelivr (a GitHub project's tags), PyPI and Google's SDK repository.
# Needs PyYAML (python3-yaml), as tools/stack.py.

import json
import re
import sys
import urllib.request
import xml.etree.ElementTree as ET

import stack

# Component (as tools/stack.py names it) -> where its releases are
#   ("gh", owner/repo, tag prefix): the tags of a GitHub project
#   ("pypi", package): a Python package
#   ("ndk",): Google's SDK repository
SOURCES = {
    "SDL3": ("gh", "libsdl-org/SDL", "release-3."),
    "SDL": ("gh", "libsdl-org/SDL", "release-2."),
    "GLFW": ("gh", "glfw/glfw", ""),
    "glad": ("pypi", "glad2"),
    "Yoga": ("gh", "facebook/yoga", "v"),
    "TinyXML-2": ("gh", "leethomason/tinyxml2", ""),
    "JSON for Modern C++": ("gh", "nlohmann/json", "v"),
    "Tweeny": ("gh", "mobius3/tweeny", "v"),
    "Twemoji": ("gh", "jdecked/twemoji", "v"),
    "Emscripten": ("gh", "emscripten-core/emsdk", ""),
    "Android NDK": ("ndk",),
}
HOMES = {
    "pypi": "https://pypi.org/project/{}/",
    "gh": "https://github.com/{}/releases",
    "ndk": "https://developer.android.com/ndk/downloads",
}
PRERELEASE = re.compile(r"(rc|beta|alpha|preview|pre|dev)", re.I)


def fetch(url):
    request = urllib.request.Request(url, headers={"User-Agent": "aurore-check-upstream"})
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read()


def numbers(version):
    """(3, 4, 18) for "3.4.18", "release-3.4.18" or "v3.4.18"; None otherwise,
    and for a date (Yoga's old 2017.02.07.00)"""
    m = re.search(r"(\d+(?:\.\d+)*)$", version)
    n = tuple(int(p) for p in m.group(1).split(".")) if m else None
    return n if n and n[0] < 1000 else None


def newest(versions):
    best = None
    for v in versions:
        n = numbers(v)
        if n and not PRERELEASE.search(v) and (best is None or n > numbers(best)):
            best = v
    return best


def latest(source):
    kind = source[0]
    if kind == "gh":
        repo, prefix = source[1], source[2]
        data = json.loads(fetch(f"https://data.jsdelivr.com/v1/packages/gh/{repo}"))
        tags = [t["version"] for t in data.get("versions", [])]
        # jsDelivr gives "v3.12.0" as "3.12.0": the prefix is checked when it is kept
        tags = [t for t in tags if t.startswith(prefix) or (prefix in ("v", "") and re.match(r"\d", t))]
        found = newest(tags)
        return ".".join(map(str, numbers(found))) if found else None
    if kind == "pypi":
        return json.loads(fetch(f"https://pypi.org/pypi/{source[1]}/json"))["info"]["version"]
    if kind == "ndk":
        root = ET.fromstring(fetch("https://dl.google.com/android/repository/repository2-3.xml"))
        best = None
        for package in root.iter("remotePackage"):
            if not package.get("path", "").startswith("ndk;"):
                continue
            revision = package.find("revision")
            if revision.find("preview") is not None:
                continue
            n = tuple(int(revision.findtext(k, "0")) for k in ("major", "minor", "micro"))
            best = max(best, n) if best else n
        return f"r{best[0]}" if best else None
    raise ValueError(kind)


def comparable(name, version):
    """The part of our version to compare: the NDK's release number (r30)"""
    if name == "Android NDK":
        return (int(version.lstrip("r")),)
    return numbers(version)


def report():
    ours = dict(stack.versions())
    rows = []
    for name, source in SOURCES.items():
        if name not in ours:
            sys.exit(f"check_upstream: tools/stack.py has no {name}")
        try:
            theirs = latest(source)
        except Exception as e:  # one upstream out of reach does not hide the others
            rows.append((name, ours[name], f"unknown ({e.__class__.__name__})", source, None))
            continue
        if theirs is None:
            rows.append((name, ours[name], "unknown", source, None))
            continue
        behind = comparable(name, theirs) > comparable(name, ours[name])
        rows.append((name, ours[name], theirs, source, behind))
    return rows


def table(rows):
    lines = ["| Component | Aurore | Upstream | |", "| --- | --- | --- | --- |"]
    for name, ours, theirs, source, behind in rows:
        home = HOMES[source[0]].format(*source[1:2])
        mark = "⬆️ behind" if behind else ("✅" if behind is False else "❔")
        lines.append(f"| {name} | {ours} | [{theirs}]({home}) | {mark} |")
    return "\n".join(lines)


def main(args):
    rows = report()
    if args == ["--behind"]:
        behind = [r for r in rows if r[4]]
        if behind:
            print(table(behind))
        sys.exit(1 if behind else 0)
    if args:
        sys.exit("usage: tools/check_upstream.py [--behind]")
    print(table(rows))


if __name__ == "__main__":
    main(sys.argv[1:])
