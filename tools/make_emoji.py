#!/usr/bin/env python3
# Copyright 2026 JigSawFr
# Licensed under the Apache License, Version 2.0.
#
# Fetches the emoji of resources/img/emoji (README.md there) from one
# Twemoji release, VERSION below: its 72x72 PNGs, as they are.
#
#     python3 tools/make_emoji.py               every PNG already there, again
#     python3 tools/make_emoji.py 1f600 1f680   these too (a code point in
#                                               lowercase hex, flags joined by
#                                               a dash: 1f1eb-1f1f7)
#
# From jsDelivr, which serves the release's tag of jdecked/twemoji, the
# maintained Twemoji. The output is the same on every run.
# tools/check_notices.py checks the notices name VERSION.

import pathlib
import re
import sys
import urllib.request

VERSION = "17.0.3"
URL = "https://cdn.jsdelivr.net/gh/jdecked/twemoji@{version}/assets/72x72/{name}.png"
FOLDER = pathlib.Path(__file__).resolve().parent.parent / "resources" / "img" / "emoji"


def main(names):
    for name in names:
        if not re.fullmatch(r"[0-9a-f]{2,6}(-[0-9a-f]{2,6})*", name):
            sys.exit(f"{name}: not a code point in lowercase hex")
    wanted = sorted({p.stem for p in FOLDER.glob("*.png")} | set(names))
    for name in wanted:
        with urllib.request.urlopen(URL.format(version=VERSION, name=name)) as response:
            (FOLDER / f"{name}.png").write_bytes(response.read())
    print(f"{FOLDER.relative_to(FOLDER.parents[2])}: {len(wanted)} emoji from Twemoji {VERSION}")


if __name__ == "__main__":
    main(sys.argv[1:])
