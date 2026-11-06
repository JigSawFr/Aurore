#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
#
# Checks that the data members of the platform classes (library/include/aurore/
# platforms/*/) have an initial value: a number, a pointer, a bool, a plain C
# struct or an array of them left without one is read as garbage by whatever
# runs before the code that sets it (Aurore#145: the Switch's video context read
# `videoContext` from its own constructor and crashed at the first start).
#
#     python3 tools/check_member_init.py
#
# Exits 1 with the list of members without an initial value.

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADERS = sorted((ROOT / "library" / "include" / "aurore" / "platforms").glob("*/*.hpp"))

# A member line: `type name;`, `type name[N];`, `type a, b;` (no `=`, no `{`, no
# parentheses) whose type is a number, a bool or a pointer. Class types have a
# constructor of their own; the plain C structs of a console's SDK (Hid*, Pad*,
# NVGpaint…) are not recognised by name here, the review of a new one is the
# reader's. `static` members and the packet structs of ps4_sysmodule.hpp are out.
SCALAR = (r"(?:unsigned\s+)?(?:bool|char|short|int|long|float|double|size_t|ssize_t|u?int(?:8|16|32|64)_t|u(?:8|16|32|64)|s(?:8|16|32|64))")
MEMBER = re.compile(
    r"^\s{4}(?!static|inline|const|typedef|using|friend|return)"
    rf"((?:{SCALAR})|[A-Za-z_][\w:<>]*\s*\*)\s*"
    r"(\*?[A-Za-z_]\w*(?:\[[^\]]*\])*(?:\s*,\s*\*?[A-Za-z_]\w*(?:\[[^\]]*\])*)*);"
)
SKIP = {"ps4_sysmodule.hpp"}


def main():
    bad = []
    for header in HEADERS:
        if header.name in SKIP:
            continue
        for number, line in enumerate(header.read_text(encoding="utf-8").splitlines(), 1):
            code = line.split("//")[0]
            if "(" not in code and "=" not in code and "{" not in code and MEMBER.match(code):
                bad.append(f"{header.relative_to(ROOT)}:{number}: {code.strip()}")
    if bad:
        print("Members without an initial value (add `= 0`, `= nullptr` or `{}`):")
        print("\n".join(bad))
        return 1
    print(f"{len(HEADERS)} platform headers: every member has an initial value")
    return 0


if __name__ == "__main__":
    sys.exit(main())
