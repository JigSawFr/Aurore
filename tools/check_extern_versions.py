#!/usr/bin/env python3
"""Checks that AURORE_FETCH_DEPS downloads the versions vendored in library/lib/extern.

library/CMakeLists.txt declares fmt, tinyxml2, GLFW, SDL2 and SDL3 with
FetchContent for packagers; the copies in library/lib/extern (and the GLFW, SDL
and SDL3 submodules) are what every other build uses. The two must name the same
version. Run from anywhere: python3 tools/check_extern_versions.py
"""

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LIBRARY = ROOT / "library"


def define(path, name):
    match = re.search(r"#define\s+" + name + r"\s+(\d+)", path.read_text(encoding="utf-8", errors="replace"))
    if not match:
        sys.exit(f"{path}: no {name}")
    return int(match.group(1))


def declared():
    """name -> GIT_TAG of each FetchContent_Declare in library/CMakeLists.txt"""
    text = (LIBRARY / "CMakeLists.txt").read_text(encoding="utf-8")
    tags = {}
    for name, body in re.findall(r"FetchContent_Declare\(\s*(\w+)(.*?)\)", text, re.S):
        tag = re.search(r"GIT_TAG\s+(\S+)", body)
        if tag:
            tags[name] = tag.group(1)
    return tags


def submodule_commit(path):
    out = subprocess.run(["git", "-C", str(ROOT), "ls-tree", "HEAD", path], capture_output=True, text=True, check=True)
    fields = out.stdout.split()
    if len(fields) < 3 or fields[1] != "commit":
        sys.exit(f"{path}: not a submodule")
    return fields[2]


def vendored():
    fmt = define(LIBRARY / "lib/extern/fmt/include/fmt/base.h", "FMT_VERSION")
    xml = LIBRARY / "include/aurore/extern/tinyxml2/tinyxml2.h"
    sdl = LIBRARY / "lib/extern/SDL/include/SDL_version.h"
    sdl3 = LIBRARY / "lib/extern/SDL3/include/SDL3/SDL_version.h"
    return {
        "fmt": f"{fmt // 10000}.{fmt // 100 % 100}.{fmt % 100}",
        "tinyxml2": ".".join(str(define(xml, f"TINYXML2_{part}_VERSION")) for part in ("MAJOR", "MINOR", "PATCH")),
        "glfw": submodule_commit("library/lib/extern/glfw"),
        "SDL2": "release-"
        + ".".join(str(define(sdl, name)) for name in ("SDL_MAJOR_VERSION", "SDL_MINOR_VERSION", "SDL_PATCHLEVEL")),
        "SDL3": "release-"
        + ".".join(str(define(sdl3, name)) for name in ("SDL_MAJOR_VERSION", "SDL_MINOR_VERSION", "SDL_MICRO_VERSION")),
    }


def main():
    tags = declared()
    errors = 0
    for name, version in vendored().items():
        tag = tags.get(name)
        if tag is None:
            print(f"{name}: no FetchContent_Declare in library/CMakeLists.txt")
            errors += 1
        elif tag != version:
            print(f"{name}: vendored {version}, FetchContent downloads {tag}")
            errors += 1
        else:
            print(f"{name}: {version}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
