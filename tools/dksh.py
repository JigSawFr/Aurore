#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
#
# The deko3d shaders compiled once and kept
# (library/lib/extern/nanovg/deko3d/shaders/prebuilt/): uam does not always
# give the same bytes for the same GLSL, so library/CMakeLists.txt takes the
# kept copy while the GLSL still hashes to the one it was compiled from, and
# runs uam only when it does not.
#
#     python3 tools/dksh.py            checks each kept copy against its GLSL
#     python3 tools/dksh.py --update   compiles them again with uam (devkitPro)
#                                      and writes the hashes
#
# Exits 1 with the list of what is out of date.

import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

SHADERS = Path(__file__).resolve().parent.parent / "library" / "lib" / "extern" / "nanovg" / "deko3d" / "shaders"
PREBUILT = SHADERS / "prebuilt"
# The shaders and their stage, as library/CMakeLists.txt compiles them
STAGES = {"fill_vsh": "vert", "fill_aa_fsh": "frag", "fill_fsh": "frag"}


def digest(glsl: Path) -> str:
    return hashlib.sha256(glsl.read_bytes()).hexdigest()


def check() -> list[str]:
    problems = []
    for name in STAGES:
        glsl, dksh, stamp = SHADERS / f"{name}.glsl", PREBUILT / f"{name}.dksh", PREBUILT / f"{name}.glsl.sha256"
        if not glsl.is_file():
            problems.append(f"{name}.glsl is missing")
        elif not dksh.is_file() or not stamp.is_file():
            problems.append(f"{name}: no kept copy (prebuilt/{name}.dksh and prebuilt/{name}.glsl.sha256)")
        elif stamp.read_text().strip() != digest(glsl):
            problems.append(f"{name}: {name}.glsl changed since prebuilt/{name}.dksh was compiled")
    return problems


def update() -> None:
    uam = shutil.which("uam") or "/opt/devkitpro/tools/bin/uam"
    PREBUILT.mkdir(exist_ok=True)
    for name, stage in STAGES.items():
        glsl = SHADERS / f"{name}.glsl"
        subprocess.run([uam, "-s", stage, "-o", str(PREBUILT / f"{name}.dksh"), str(glsl)], check=True, timeout=60)
        (PREBUILT / f"{name}.glsl.sha256").write_text(digest(glsl) + "\n")
        print(f"{name}: compiled")


def main() -> int:
    if sys.argv[1:] == ["--update"]:
        update()
        return 0
    if sys.argv[1:]:
        print(__doc__)
        return 1
    problems = check()
    for problem in problems:
        print(f"dksh: {problem}")
    if problems:
        print("dksh: run tools/dksh.py --update with devkitPro's uam, and commit prebuilt/")
        return 1
    print(f"dksh: {len(STAGES)} kept shaders match their GLSL")
    return 0


if __name__ == "__main__":
    sys.exit(main())
