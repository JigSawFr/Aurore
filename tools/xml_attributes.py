#!/usr/bin/env python3
# The XML attributes the library's views register (register*XMLAttribute,
# AURORE_REGISTER_ENUM_XML_ATTRIBUTE), written as docs/xml-attributes.md.
#
#   python3 tools/xml_attributes.py          writes the page
#   python3 tools/xml_attributes.py --check  1 when the page is not up to date
#
# Run from anywhere: the paths are the repository's.

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCES = os.path.join(ROOT, "library", "lib")
PAGE = os.path.join(ROOT, "docs", "xml-attributes.md")

KINDS = {
    "Float": "number",
    "Percentage": "percentage (`50%`)",
    "Auto": "`auto`",
    "String": "text (`@i18n/…`)",
    "Color": "colour (`#rrggbb`, `@theme/…`)",
    "Bool": "`true` / `false`",
    "FilePath": "file (`@res/…`)",
}

# Text attributes read with a grammar of their own: what they take
TEXT_VALUES = {
    "safeArea": "edges joined with `\\|`: `top` · `right` · `bottom` · `left` · `vertical` · `horizontal` · `all` · `none`; or `auto` ([safe areas](safe-areas.md))",
}

REGISTER = re.compile(r'register(Float|Percentage|Auto|String|Color|Bool|FilePath)XMLAttribute\(\s*"([^"]+)"')
ENUM = re.compile(r'AURORE_REGISTER_ENUM_XML_ATTRIBUTE\(\s*"([^"]+)"\s*,[^{]*\{(.*?)\}\s*\)', re.S)
ENUM_VALUE = re.compile(r'\{\s*"([^"]+)"\s*,')
FUNCTION = re.compile(r'^(?:[\w:<>*&]+\s+)?((?:\w+::)*\w+)::~?\w+\([^;]*$', re.M)
VIEW_TAG = re.compile(r'registerXMLView\("([^"]+)",\s*(?:\w+::)*(\w+)::create\)')
BASE = re.compile(r'^class\s+(\w+)(?:\s+final)?\s*:\s*public\s+(?:\w+::)*(\w+)', re.M)
HEADERS = os.path.join(ROOT, "library", "include")


def owner_at(text, offset):
    # The class whose function the registration is in
    owner = None
    for match in FUNCTION.finditer(text, 0, offset):
        owner = match.group(1).split("::")[-1]
    return owner


def collect():
    tags = {}
    attributes = {}  # class -> name -> (kinds, values)
    files = []
    for folder, _, names in os.walk(SOURCES):
        if os.sep + "extern" in folder:
            continue
        for name in names:
            if name.endswith(".cpp"):
                files.append(os.path.join(folder, name))
    for path in sorted(files):
        with open(path, encoding="utf-8") as f:
            text = f.read()
        for match in VIEW_TAG.finditer(text):
            tags.setdefault(match.group(2), match.group(1))
        for match in REGISTER.finditer(text):
            owner = owner_at(text, match.start())
            if not owner:
                continue
            entry = attributes.setdefault(owner, {}).setdefault(match.group(2), [set(), []])
            entry[0].add(match.group(1))
        for match in ENUM.finditer(text):
            owner = owner_at(text, match.start())
            if not owner:
                continue
            entry = attributes.setdefault(owner, {}).setdefault(match.group(1), [set(), []])
            entry[1] = sorted(ENUM_VALUE.findall(match.group(2)))
    # A tag registered by a class that adds no attributes (Cadran's
    # FooterHints for <aurore:Hints>) is its base's, which has them
    bases = {}
    for folder, _, names in os.walk(HEADERS):
        if os.sep + "extern" in folder:
            continue
        for name in names:
            if name.endswith(".hpp"):
                with open(os.path.join(folder, name), encoding="utf-8") as f:
                    bases.update(BASE.findall(f.read()))
    for owner, tag in list(tags.items()):
        base = owner
        while base not in attributes and base in bases:
            base = bases[base]
        if base != owner and base in attributes and base not in tags:
            tags[base] = tag
    return tags, attributes


def render(tags, attributes):
    lines = [
        "# XML attributes",
        "",
        "Every attribute the library's views read from XML, by view: those of `View` are on every view, those of `Box` on every box (most views are boxes). Written by `tools/xml_attributes.py` from the sources; a test checks it is up to date. Styles per state (`focused:…`) are in [styles.md](styles.md), the theme keys (`@theme/…`) in [themes.md](themes.md).",
        "",
        "A misspelt attribute or a value outside the list stops the app with the file, the line and the closest name or the values expected (`View::fatalXML`).",
        "",
        "A number is written `16`, `16px` (the same), `16dp` (16 at the density in use: 11.2 when dense, see [styles.md](styles.md#density-of-the-lengths)), `25%` where a percentage is taken, or `@style/<name>` (a length of the style follows the density too).",
        "",
        "Any attribute prefixed with `compact:`, `medium:` or `expanded:` applies on a screen of that width only, over the plain one ([styles.md](styles.md#screen-width)).",
    ]

    def order(owner):
        return (owner != "View", owner != "Box", tags.get(owner, owner))

    for owner in sorted(attributes, key=order):
        tag = tags.get(owner)
        title = "`<%s>`" % tag if tag else "`%s`" % owner
        if owner == "View":
            title = "Every view (`View`)"
        elif owner == "Box":
            title = "Every box (`<aurore:Box>`)"
        lines += ["", "## " + title, "", "| Attribute | Value |", "|---|---|"]
        for name in sorted(attributes[owner], key=str.lower):
            kinds, values = attributes[owner][name]
            described = [KINDS[k] for k in sorted(kinds) if not ((values or name in TEXT_VALUES) and k == "String")]
            if name in TEXT_VALUES:
                described.append(TEXT_VALUES[name])
            if values:
                described.append(" · ".join("`%s`" % v for v in values))
            lines.append("| `%s` | %s |" % (name, ", ".join(described)))
    return "\n".join(lines) + "\n"


def main():
    page = render(*collect())
    if "--check" in sys.argv:
        try:
            with open(PAGE, encoding="utf-8") as f:
                current = f.read()
        except OSError:
            current = ""
        if current != page:
            sys.stderr.write("docs/xml-attributes.md is not up to date: run python3 tools/xml_attributes.py\n")
            return 1
        return 0
    with open(PAGE, "w", encoding="utf-8", newline="\n") as f:
        f.write(page)
    return 0


if __name__ == "__main__":
    sys.exit(main())
