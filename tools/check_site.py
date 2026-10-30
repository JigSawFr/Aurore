#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

"""Checks the built website (site/README.md):

- what every one of Boréale's sites passes (library/boreale-site,
  boreale_site.check): no {{key}} left, every link and image to the site
  leads to a file and every #anchor to an id of its page, links out by the
  rule "liens sortants anonymes" and their UTM tags, nothing the
  Content-Security-Policy blocks (an inline script but schema.org's data, a
  style attribute or element, a file from elsewhere), Umami's tracker, no
  e-mail address, the schema.org data read as JSON; the API reference's
  pages (tools/build_api.py's) included, the demo, built apart, left out;
- the catalogs of site/i18n/ and boreale-site's: the same keys as
  en-US.json, none empty (but English's notes that a page is in English),
  the same {placeholders} and <em> as English's, no
  common text repeated here;
- Aurore's own: no event handler attribute (the CSP blocks it), no
  stylesheet from elsewhere, search.json and the manifest are JSON, a
  link's #anchor in another page of the site is there, and so is a page an
  absolute link to the site itself leads to (site-url).

A pull request's preview (SITE_NOINDEX=1) has no tracker, and a page that
only sends its visitor on (docs/, Doxygen's old addresses) has neither
tracker nor policy: boreale_site.check asks both of every page, so their
absence is not counted there.

Usage: tools/check_site.py <site-dir> [site-url]
(site-url: the site's own address, whose absolute links are checked too)
"""
import json
import os
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "library" / "boreale-site"))
import boreale_site as boreale  # noqa: E402
from boreale_site import check  # noqa: E402

NOINDEX = os.environ.get("SITE_NOINDEX") == "1"
NO_TRACKER = "no Umami tracker (assets/umami.js)"
NO_POLICY = "no Content-Security-Policy"
# English's texts left empty on purpose: the note that a page is in
# English, shown in the other languages alone. boreale_site.check_catalogs()
# finds every empty text; until it takes such keys, they are let be here.
EMPTY = {f"i18n/en-US.json: {key} is empty" for key in ("doc_untranslated", "changelog_english")}


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links, self.ids, self.blocked = [], set(), []
        self.redirect = False

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if "id" in a:
            self.ids.add(a["id"])
        if tag == "meta" and (a.get("http-equiv") or "").lower() == "refresh":
            self.redirect = True
        for key in ("href", "src"):
            if a.get(key):
                self.links.append(a[key])
        self.blocked += [f"<{tag} {k}>" for k in a if k.startswith("on")]
        if tag == "link" and a.get("rel") == "stylesheet" and urlsplit(a.get("href", "")).scheme:
            self.blocked.append(f"a stylesheet from elsewhere: {a['href']}")


def expected(error: str, redirects: set) -> bool:
    """A problem boreale_site.check finds that this site has on purpose."""
    name, _, problem = error.partition(": ")
    if problem == NO_TRACKER and (NOINDEX or name in redirects):
        return True
    return problem == NO_POLICY and name in redirects


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    root = os.path.abspath(sys.argv[1])
    site = sys.argv[2].rstrip("/") + "/" if len(sys.argv) > 2 else None
    problems = [e for e in boreale.check_catalogs(ROOT / "site" / "i18n") if e not in EMPTY]
    pages = {}
    for folder, dirs, files in os.walk(root):
        if os.path.relpath(folder, root).split(os.sep)[0] in ("demo", "assets"):
            continue
        for name in files:
            path = os.path.join(folder, name)
            if name.endswith(".html"):
                page = Page()
                page.feed(open(path, encoding="utf-8").read())
                pages[path] = page
            elif name.endswith((".json", ".webmanifest")):
                try:
                    json.load(open(path, encoding="utf-8"))
                except ValueError as e:
                    problems.append(f"{os.path.relpath(path, root)}: {e}")
    redirects = {os.path.relpath(p, root).replace(os.sep, "/") for p, page in pages.items() if page.redirect}
    found, _ = check.pages(Path(root), boreale.Site("aurore", os.environ.get("SITE_URL")), skip=("assets", "demo"))
    problems += [e for e in found if not expected(e, redirects)]

    for path, page in sorted(pages.items()):
        rel_page = os.path.relpath(path, root)
        problems += [f"{rel_page}: {what} (the Content-Security-Policy blocks it)" for what in page.blocked]
        if os.path.basename(path) == "404.html":
            continue   # served at any path: boreale_site.check follows its links from the site's root
        for link in page.links:
            absolute = bool(site and link.startswith(site))
            if absolute:
                link = os.path.relpath(os.path.join(root, link[len(site):]), os.path.dirname(path)) + \
                       ("/" if link.endswith("/") else "")
            parts = urlsplit(link)
            if parts.scheme or link.startswith(("mailto:", "data:", "//")):
                continue
            file = os.path.normpath(os.path.join(os.path.dirname(path), unquote(parts.path))) if parts.path else path
            if os.path.isdir(file):
                file = os.path.join(file, "index.html")
            rel = os.path.relpath(file, root)
            if rel.split(os.sep)[0] == "demo" or rel.startswith(".."):
                continue
            # A relative link to a file that is not there: boreale_site.check says so.
            if absolute and not os.path.exists(file):
                problems.append(f"{rel_page}: {link}: no {rel}")
            elif parts.fragment and file != path and file in pages and unquote(parts.fragment) not in pages[file].ids:
                problems.append(f"{rel_page}: {link}: no #{unquote(parts.fragment)} in {rel}")
    for problem in problems:
        print(problem)
    print(f"site: {len(pages)} pages checked, {len(boreale.CODES)} catalogs, {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
