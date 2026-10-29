#!/usr/bin/env python3
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# What Aurore is built with and how it is checked, read from the repository:
# the versions from the build files and resources/THIRD_PARTY_NOTICES.md
# (tools/check_notices.py checks those against the sources), the figures
# from the tests and the CI's workflows. The README's "Under the hood"
# (between its under-the-hood markers) and the site's home page
# (tools/build_site.py) show them, so neither says more than the code.
#
#     python3 tools/stack.py            the README's section, written again
#     python3 tools/stack.py --check    fails when the README's is not that
#     python3 tools/stack.py --json     the facts, for a look
#
# Needs PyYAML (python3-yaml) for the workflows.

import json
import pathlib
import re
import sys

import yaml

ROOT = pathlib.Path(__file__).resolve().parent.parent
BEGIN = "<!-- under-the-hood: begin (tools/stack.py writes this) -->"
END = "<!-- under-the-hood: end -->"

# The components shown, as the notices name them, in this order
COMPONENTS = ["SDL3", "SDL", "GLFW", "glad", "Yoga", "{fmt}", "TinyXML-2", "JSON for Modern C++",
              "stb_image", "stb_truetype", "Tweeny", "Twemoji"]


def read(*parts):
    return (ROOT.joinpath(*parts)).read_text(encoding="utf-8")


def search(pattern, text, what):
    m = re.search(pattern, text, re.M)
    if not m:
        sys.exit(f"stack: {what} not found")
    return m.group(1)


def notices():
    """{component: its version} from the notices' tables, the first word"""
    found = {}
    for line in read("resources", "THIRD_PARTY_NOTICES.md").splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if line.startswith("|") and len(cells) >= 3 and re.match(r"\d", cells[1]):
            found[cells[0]] = cells[1].split()[0]
    return found


def versions():
    """[(name, version)]: the language and build, the libraries, the toolchains"""
    library = read("library", "CMakeLists.txt")
    cmake = search(r"cmake_minimum_required\(VERSION ([\d.]+\.\.\.[\d.]+)\)", library, "CMake's range")
    rows = [("Language", "C++" + search(r"cxx_std_(\d+)", library, "the C++ standard")),
            ("CMake", cmake.replace("...", "–"))]
    listed = notices()
    for name in COMPONENTS:
        if name not in listed:
            sys.exit(f"stack: {name} not in resources/THIRD_PARTY_NOTICES.md")
        rows.append((name, listed[name]))
    gradle = read("android-project", "app", "build.gradle")
    rows += [("Android Gradle Plugin", search(r"com\.android\.application' version '([\d.]+)'",
                                              read("android-project", "build.gradle"), "AGP's version")),
             ("Gradle", search(r"gradle-([\d.]+)-bin\.zip",
                               read("android-project", "gradle", "wrapper", "gradle-wrapper.properties"),
                               "Gradle's version")),
             ("Android NDK", "r" + search(r"ndkVersion = '(\d+)\.", gradle, "the NDK")),
             ("Android API", search(r"targetSdk = (\d+)", gradle, "the target API")),
             ("Emscripten", search(r"EMSDK_VERSION: ([\d.]+)", read(".github", "workflows", "site.yml"),
                                   "Emscripten's version"))]
    return rows


def matrix_size(matrix):
    """The jobs a matrix makes, as GitHub expands it: the product of its
    keys, less what `exclude` matches, and each `include` added to the
    combinations it matches (a new one when it matches none)"""
    combos = [{}]
    for key, values in matrix.items():
        if key not in ("include", "exclude"):
            combos = [dict(c, **{key: v}) for c in combos for v in values]
    combos = [c for c in combos
              if not any(all(c.get(k) == v for k, v in e.items()) for e in matrix.get("exclude", []))]
    base = combos != [{}]
    added = 0
    for extra in matrix.get("include", []):
        if not (base and any(all(c[k] == v for k, v in extra.items() if k in c) for c in combos)):
            added += 1
    return (len(combos) if base else 0) + added or 1


def jobs(workflow):
    """The jobs a workflow runs, each matrix entry one job"""
    count = 0
    for job in yaml.safe_load(read(".github", "workflows", workflow))["jobs"].values():
        matrix = (job.get("strategy") or {}).get("matrix") or {}
        if isinstance(matrix, str):
            sys.exit(f"stack: {workflow}: a matrix from an expression")
        count += matrix_size(matrix)
    return count


def figures():
    """What the tests and the CI check, counted"""
    tests = sum(len(re.findall(r"^AURORE_TEST\(", p.read_text(encoding="utf-8"), re.M))
                for p in (ROOT / "tests").glob("*.cpp"))
    checks = sum(len(re.findall(r"\bCHECK\(", p.read_text(encoding="utf-8")))
                 for p in (ROOT / "tests" / "host").glob("*.cpp"))
    workflows = sorted(p.name for p in (ROOT / ".github" / "workflows").glob("*.y*ml"))
    uses = []
    for name in workflows:
        uses += re.findall(r"^\s*(?:-\s*)?uses:\s*(\S+)", read(".github", "workflows", name), re.M)
    actions = [u for u in uses if not u.startswith("./")]
    return {
        "tests": tests,
        "host_checks": checks,
        "references": len(list((ROOT / "tests" / "references").glob("*.png"))),
        "ci_jobs": jobs("ci.yaml"),
        "languages": len([p for p in (ROOT / "resources" / "i18n").iterdir() if p.is_dir()]),
        "actions": len(actions),
        "actions_pinned": len([u for u in actions if re.search(r"@[0-9a-f]{40}$", u)]),
    }


def facts():
    return {"versions": versions(), "figures": figures()}


def readme_section(data):
    f = data["figures"]
    rows = "\n".join(f"| {name} | {version} |" for name, version in data["versions"])
    return f"""{BEGIN}
| | Version |
| --- | --- |
{rows}

Checked on every pull request: {f["tests"]} test cases in the headless harness, {f["references"]} reference screenshots compared pixel by pixel, and {f["host_checks"]} checks of Cadran's text code; {f["ci_jobs"]} CI jobs build every platform, consoles included; the demo runs under AddressSanitizer and UndefinedBehaviorSanitizer, and in headless Chromium for its build in a browser; a build fails on a deprecated API; {f["actions_pinned"]} of the {f["actions"]} GitHub Actions used are pinned to a commit; every Monday, `tools/check_upstream.py` compares the bundled components with their upstream's latest release and keeps an issue open with those behind. The versions are those of the build files and [`THIRD_PARTY_NOTICES.md`](resources/THIRD_PARTY_NOTICES.md), which `tools/check_notices.py` checks against the sources; `tools/stack.py` writes this section.
{END}"""


def main(args):
    data = facts()
    if args == ["--json"]:
        print(json.dumps(data, indent=2, ensure_ascii=False))
        return
    readme = read("README.md")
    if BEGIN not in readme or END not in readme:
        sys.exit("stack: README.md has no under-the-hood markers")
    start, end = readme.index(BEGIN), readme.index(END) + len(END)
    wanted = readme[:start] + readme_section(data) + readme[end:]
    if args == ["--check"]:
        if wanted != readme:
            sys.exit("stack: README.md's \"Under the hood\" is not what the repository says: "
                     "run python3 tools/stack.py")
        print("README.md: \"Under the hood\" as the repository says")
        return
    if args:
        sys.exit("usage: tools/stack.py [--check | --json]")
    (ROOT / "README.md").write_text(wanted, encoding="utf-8")


if __name__ == "__main__":
    main(sys.argv[1:])
