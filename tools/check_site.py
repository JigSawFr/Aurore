#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

"""Checks the built website (site/README.md):

- what every one of Boréale's sites passes (library/boreale-site,
  boreale_site.check.pages()): no {{key}} left, every link and image to the
  site leads to a file, its absolute links to its own address included, and
  every #anchor to an id of the page it names, links out by the rule "liens
  sortants anonymes" and their UTM tags, nothing the Content-Security-Policy
  blocks (an inline script but schema.org's data, a style attribute or
  element, an event handler, a file from elsewhere), Umami's tracker (not in
  a pull request's preview, SITE_NOINDEX=1), no e-mail address, the
  schema.org data read as JSON; the API reference's pages (tools/build_api.py's)
  included, the demo, built apart, left out; a page that only sends its
  visitor on (docs/, Doxygen's old addresses) needs neither tracker nor
  policy;
- the catalogs of site/i18n/ and boreale-site's (check_catalogs()): the same
  keys as en-US.json, none empty but English's notes that a page is in
  English, the same {placeholders} and <em> as English's, no common text
  repeated here;
- Aurore's own: no stylesheet from elsewhere, search.json and the manifest
  are JSON.

Usage: tools/check_site.py <site-dir> [site-url]
(site-url: the site's own address, whose absolute links are followed too;
SITE_URL's by default, else aurore.boreale.dev)
"""
import json
import os
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "library" / "boreale-site"))
import boreale_site as boreale  # noqa: E402
from boreale_site import check  # noqa: E402


class Stylesheets(HTMLParser):
    """The stylesheets a page loads from elsewhere."""

    def __init__(self):
        super().__init__()
        self.elsewhere = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if tag == "link" and a.get("rel") == "stylesheet" and urlsplit(a.get("href", "")).scheme:
            self.elsewhere.append(a["href"])


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = Path(sys.argv[1])
    site = boreale.Site("aurore", sys.argv[2] if len(sys.argv) > 2 else os.environ.get("SITE_URL"))
    problems, count = check.pages(out, site, apart=("demo",), tracker=os.environ.get("SITE_NOINDEX") != "1")
    # English's texts left empty on purpose: the note that a page is in
    # English, shown in the other languages alone.
    problems += boreale.check_catalogs(ROOT / "site" / "i18n",
                                       optional={"en-US": {"doc_untranslated", "changelog_english"}})
    for path in sorted(out.rglob("*")):
        name = path.relative_to(out).as_posix()
        if path.parts[len(out.parts)] in ("demo", "assets") or not path.is_file():
            continue
        if path.suffix == ".html":
            page = Stylesheets()
            page.feed(path.read_text(encoding="utf-8"))
            problems += [f"{name}: a stylesheet from elsewhere: {href}" for href in page.elsewhere]
        elif path.suffix in (".json", ".webmanifest"):
            try:
                json.loads(path.read_text(encoding="utf-8"))
            except ValueError as e:
                problems.append(f"{name}: {e}")
    for problem in problems:
        print(problem)
    print(f"site: {count} pages checked, {len(boreale.CODES)} catalogs, {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
