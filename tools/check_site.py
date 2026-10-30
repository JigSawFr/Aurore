#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

"""Checks the built website (site/README.md):

- the catalogs (site/i18n/*.json) have the same keys as en-US.json;
- every link and image of a page that stays on the site reaches a file of it,
  and every #anchor an id of the page it points to (the API reference's
  pages, tools/build_api.py's, included; the demo, built apart, is not
  followed);
- no page has what its Content-Security-Policy would block: an inline
  script (but schema.org's data), a style attribute or element, an event
  handler, a file from elsewhere;
- the schema.org data, search.json and the manifest are JSON;
- links out follow the rule "liens sortants anonymes": every page sends no
  referrer (<meta name="referrer" content="no-referrer">), a link to a third
  party has rel="noreferrer", one to boreale.dev, *.boreale.dev or a
  repository of github.com/JigSawFr/ has referrerpolicy="strict-origin" (the
  origin alone, for GitHub's traffic insights) and no noreferrer.

Usage: tools/check_site.py <site-dir> [site-url]
(site-url: the site's own address, whose absolute links are checked too)
"""
import json
import os
import sys
from html.parser import HTMLParser
from urllib.parse import unquote, urlparse, urlsplit

HERE = os.path.dirname(os.path.abspath(__file__))



def own_site(url: str) -> bool:
    """boreale.dev, its subdomains, and the repositories of github.com/JigSawFr/
    (that account's alone: another owner, a gist or GitHub's own pages are
    third parties)."""
    u = urlparse(url)
    host = (u.hostname or "").lower()
    return u.scheme == "https" and (host == "boreale.dev" or host.endswith(".boreale.dev") or
                                    (host == "github.com" and u.path.lower().startswith("/jigsawfr/")))


def check_own_site() -> None:
    """own_site() against look-alikes, at each run (the site has no unit tests)."""
    for url in ("https://boreale.dev/", "https://shelf.boreale.dev/fr/", "https://github.com/JigSawFr/PlayShelf",
                "https://github.com/jigsawfr/boreale/releases"):
        assert own_site(url), url
    for url in ("https://store.example.com/app/1", "https://github.com/JigSawFr", "https://github.com/jigsawfrx/x",
                "https://github.com/someone/JigSawFr", "https://gist.github.com/JigSawFr/1",
                "https://boreale.dev.example.com/", "https://notboreale.dev/", "http://boreale.dev/"):
        assert not own_site(url), url


def link_problem(attrs: dict) -> str:
    """What a link out does wrong by the rule "liens sortants anonymes": a
    third party gets rel="noreferrer"; *.boreale.dev and github.com/JigSawFr
    keep the origin alone (referrerpolicy="strict-origin", no noreferrer)."""
    href = attrs.get("href") or ""
    if urlparse(href).scheme not in ("http", "https"):
        return ""
    rel = (attrs.get("rel") or "").split()
    if own_site(href):
        if "noreferrer" in rel or attrs.get("referrerpolicy") != "strict-origin":
            return f'link to {href} needs referrerpolicy="strict-origin" and no noreferrer'
    elif "noreferrer" not in rel:
        return f'link to {href} without rel="noreferrer"'
    return ""


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links, self.ids, self.blocked, self.data, self.links_out = [], set(), [], [], []
        self.in_script = None
        self.no_referrer = False

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if "id" in a:
            self.ids.add(a["id"])
        if tag == "meta" and a.get("name") == "referrer" and a.get("content") == "no-referrer":
            self.no_referrer = True
        if tag == "a" and link_problem(a):
            self.links_out.append(link_problem(a))
        for key in ("href", "src"):
            if a.get(key):
                self.links.append((tag, a[key]))
        if "style" in a or tag == "style":
            self.blocked.append(f"<{tag} style>")
        self.blocked += [f"<{tag} {k}>" for k in a if k.startswith("on")]
        if tag == "script":
            if a.get("type") == "application/ld+json":
                self.in_script = "ld"
            elif "src" not in a:
                self.blocked.append("an inline <script>")
            elif urlsplit(a["src"]).scheme:
                self.blocked.append(f"a script from elsewhere: {a['src']}")
        if tag == "link" and a.get("rel") == "stylesheet" and urlsplit(a.get("href", "")).scheme:
            self.blocked.append(f"a stylesheet from elsewhere: {a['href']}")
        if tag == "img" and urlsplit(a.get("src", "")).scheme in ("http", "https"):
            self.blocked.append(f"an image from elsewhere: {a['src']}")

    def handle_endtag(self, tag):
        if tag == "script":
            self.in_script = None

    def handle_data(self, data):
        if self.in_script == "ld":
            self.data.append(data)


def catalogs(root):
    folder = os.path.join(root, "site", "i18n")
    english = set(json.load(open(os.path.join(folder, "en-US.json"), encoding="utf-8")))
    errors = 0
    for name in sorted(os.listdir(folder)):
        keys = set(json.load(open(os.path.join(folder, name), encoding="utf-8")))
        for key in sorted(english - keys):
            print(f"site/i18n/{name}: no {key!r}")
            errors += 1
        for key in sorted(keys - english):
            print(f"site/i18n/{name}: {key!r} is not in en-US.json")
            errors += 1
    return errors


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    root = os.path.abspath(sys.argv[1])
    site = sys.argv[2].rstrip("/") + "/" if len(sys.argv) > 2 else None
    errors = catalogs(os.path.dirname(HERE))
    pages = {}
    for folder, dirs, files in os.walk(root):
        if os.path.relpath(folder, root).split(os.sep)[0] == "demo":
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
                    print(f"{os.path.relpath(path, root)}: {e}")
                    errors += 1

    check_own_site()
    for path, page in sorted(pages.items()):
        rel_page = os.path.relpath(path, root)
        for problem in page.links_out + ([] if page.no_referrer else ['no <meta name="referrer" content="no-referrer">']):
            print(f"{rel_page}: {problem}")
            errors += 1
        for what in page.blocked:
            print(f"{rel_page}: {what} (the Content-Security-Policy blocks it)")
            errors += 1
        for block in page.data:
            try:
                json.loads(block)
            except ValueError as e:
                print(f"{rel_page}: schema.org data: {e}")
                errors += 1
        if os.path.basename(path) == "404.html":
            continue   # served at any path: its links start from the site's root
        for _, link in page.links:
            if site and link.startswith(site):
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
            if not os.path.exists(file):
                print(f"{rel_page}: {link}: no {rel}")
                errors += 1
            elif parts.fragment and file.endswith(".html") and unquote(parts.fragment) not in pages[file].ids:
                print(f"{rel_page}: {link}: no #{unquote(parts.fragment)} in {rel}")
                errors += 1
    print(f"site: {len(pages)} pages checked, {errors} problem(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
