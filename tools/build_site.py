#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

"""Builds Aurore's website (site/README.md) into <out-dir>: a home page, the
documentation (docs/*.md), the changelog (CHANGELOG.md) and the page not
found, in the consoles' 15 languages (LANGUAGES): English at the root, the
others each in its folder (/fr/, /de/, /zh-hant/…).

- The texts around the content are site/i18n/<language>.json (en-US.json
  the reference; a key missing elsewhere is an error, tools/check_site.py
  says which). They are HTML: `{{repo}}` in them is the repository's address.
- The documentation is the Markdown of docs/, in English: every other
  language shows the same pages under its own header, saying so. Links between the
  docs and to the README become the site's pages, any other file of the
  repository links to GitHub, and GitHub's `> [!NOTE]` blocks become callouts.
  Each page is published in Markdown next to it too, listed in llms.txt.
- The screenshots are the README's (.github/readme/), the preview image and
  the icon Aurore's own (.github/branding/).
- SITE_API names a folder holding the API reference tools/build_api.py
  built (from Doxygen's XML): it is published in /api/, and the pages
  link to it. Without it, they do not.
- SITE_DEMO names the folder of the demo built for a browser (web/README.md,
  <build>/web/): it is published in /demo/, and the pages offer to try it.
  Without it, they do not.
- SITE_URL is the site's address (https://aurore.boreale.dev/ by default), for the links that must be absolute: the canonical page, the
  other languages, the preview image, the sitemap. SITE_NOINDEX=1 keeps a
  preview out of search engines.

Usage: tools/build_site.py <out-dir> [repo-root]          (needs Python-Markdown and PyYAML)
"""
import html
import json
import os
import re
import shutil
import struct
import sys
import urllib.request
from urllib.parse import urlparse, urlsplit

import markdown

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import stack as under_the_hood  # noqa: E402 (tools/stack.py: the versions and figures of "Under the hood")

REPO_URL = "https://github.com/JigSawFr/Aurore"
SITE_URL = os.environ.get("SITE_URL", "https://aurore.boreale.dev/").rstrip("/") + "/"
VERSIONS = {}  # the projects' latest releases, for the menu of Boréale's sites (main())
NOINDEX = os.environ.get("SITE_NOINDEX") == "1"
# The audience, counted by Umami Cloud (site/README.md): its tracker, served
# from assets/, sends each page seen to gateway.umami.is, without cookie, and
# only from the site's own address; a pull request's preview has none. The
# site in Umami is Boréale's, shared by its projects' sites: Umami tells them
# apart by their address (its "Hostname" filter).
UMAMI_WEBSITE = "6410ffdd-ae2e-4ddb-8e18-0a77e9fa374e"
UMAMI_DOMAIN = urlsplit(SITE_URL).hostname
API_DIR = os.environ.get("SITE_API", "")
DEMO_DIR = os.environ.get("SITE_DEMO", "")
# The consoles' languages, Aurore's own (resources/i18n/): the catalog's code
# (site/i18n/<code>.json), the site's folder ('' for the root), the name in
# its own words, the html lang. PlayGuard's and PlayShelf's sites have the same.
LANGUAGES = [
    ("en-US", "", "English", "en"),
    ("fr", "fr", "Français", "fr"),
    ("fr-CA", "fr-ca", "Français (Canada)", "fr-CA"),
    ("de", "de", "Deutsch", "de"),
    ("es", "es", "Español", "es"),
    ("es-419", "es-419", "Español (Latinoamérica)", "es-419"),
    ("it", "it", "Italiano", "it"),
    ("nl", "nl", "Nederlands", "nl"),
    ("pt", "pt", "Português", "pt-PT"),
    ("pt-BR", "pt-br", "Português (Brasil)", "pt-BR"),
    ("ru", "ru", "Русский", "ru"),
    ("ja", "ja", "日本語", "ja"),
    ("ko", "ko", "한국어", "ko"),
    ("zh-Hans", "zh-hans", "简体中文", "zh-Hans"),
    ("zh-Hant", "zh-hant", "繁體中文", "zh-Hant"),
]
# Each language's flag in the language menu (fonts/twemoji-flags.woff2 draws
# them the same everywhere, Windows included): the country of the console's
# language, Mexico for Latin America's Spanish, Taiwan for Traditional Chinese.
FLAGS = {"en-US": "US", "fr": "FR", "fr-CA": "CA", "de": "DE", "es": "ES", "es-419": "MX", "it": "IT", "nl": "NL",
         "pt": "PT", "pt-BR": "BR", "ru": "RU", "ja": "JP", "ko": "KR", "zh-Hans": "CN", "zh-Hant": "TW"}
# Open Graph's name of each language.
OG_LOCALES = {"en-US": "en_US", "fr": "fr_FR", "fr-CA": "fr_CA", "de": "de_DE", "es": "es_ES", "es-419": "es_LA",
              "it": "it_IT", "nl": "nl_NL", "pt": "pt_PT", "pt-BR": "pt_BR", "ru": "ru_RU", "ja": "ja_JP",
              "ko": "ko_KR", "zh-Hans": "zh_CN", "zh-Hant": "zh_TW"}
# The documentation in the sidebar's order: (page, group, short name).
DOCS = [
    ("getting-started", "start", "Aurore in 15 minutes"),
    ("migrating-from-borealis", "start", "Migrating from borealis"),
    ("platforms", "start", "Platforms"),
    ("porting", "start", "Porting an app"),
    ("architecture", "guides", "Architecture"),
    ("views", "guides", "Views"),
    ("themes", "guides", "Themes"),
    ("cadran", "guides", "Cadran"),
    ("styles", "guides", "Styles"),
    ("safe-areas", "guides", "Safe areas"),
    ("components", "guides", "Components"),
    ("xml-attributes", "reference", "XML attributes"),
    ("input", "reference", "Keyboard and mouse"),
    ("fonts", "reference", "Fonts"),
    ("environment", "reference", "Environment variables"),
    ("testing", "tools", "Testing"),
    ("inspector", "tools", "Inspector"),
    ("hot_reload", "tools", "Hot reload"),
    ("overlay", "tools", "Overlay (design note)"),
]
GROUPS = ["start", "guides", "reference", "tools"]
# The gallery, the README's screenshots (.github/readme/<name>.png, 1280x720).
SHOTS = ["cadran", "sheet", "picker", "checklist", "markdown", "components",
         "winter", "prism", "aurora", "palettes", "light", "french"]
# The demo on each simulated console (docs/platforms.md, Simulated platforms):
# .github/readme/console-<id>.png, made by capture.sh; the name, the screen
CONSOLES = [("switch", "Switch", "1280 × 720"), ("vita", "PS Vita", "960 × 544"),
            ("ps4", "PS4", "1920 × 1080"), ("3ds", "New 3DS", "320 × 240 + 400 × 240"),
            ("phone", None, "1600 × 720")]
FEATURES = ["controllers", "layout", "graphics", "components", "themes", "languages", "tools", "toolbox"]
ADDS = ["themes", "styles", "views", "layout", "text", "focus", "platforms", "speed"]
PLATFORMS = ["Nintendo Switch", "Android", "iOS", "Windows", "macOS", "Linux", "PS4", "PS Vita", "New 3DS", "Browser"]
MADE_WITH = [
    ("PlayGuard", "https://github.com/JigSawFr/PlayGuard", "playguard"),
    ("PlayShelf", "https://github.com/JigSawFr/PlayShelf", "playshelf"),
    ("Moonlight-Switch", "https://github.com/XITRIX/Moonlight-Switch", "moonlight"),
    ("wiliwili", "https://github.com/xfangfang/wiliwili", "wiliwili"),
    ("switchfin", "https://github.com/dragonflylee/switchfin", "switchfin"),
    ("SimpleModDownloader", "https://github.com/PoloNX/SimpleModDownloader", "smd"),
]
# Only the site's own files, no inline script or style (a <meta>: GitHub
# Pages sends no headers). connect-src: the documentation's search index, and
# Umami, which counts the audience (UMAMI_WEBSITE).
CSP = ("default-src 'none'; script-src 'self'; style-src 'self'; img-src 'self' data:; font-src 'self'; "
       "connect-src 'self' https://gateway.umami.is; manifest-src 'self'; base-uri 'none'; form-action 'none'")
# Each feature's icon: a line drawing on a 24px grid, in the text's colour.
ICONS = {
    "controllers": '<path d="M7 9v4M5 11h4"/><circle cx="15.5" cy="10" r=".8"/><circle cx="17.5" cy="12.5" r=".8"/>'
                   '<path d="M7.5 6h9a4.5 4.5 0 0 1 4.4 5.5l-1 4.5a2.5 2.5 0 0 1-4.3 1L14 15h-4l-1.6 2a2.5 2.5 0 0 1-4.3-1l-1-4.5A4.5 4.5 0 0 1 7.5 6z"/>',
    "layout": '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M9 4v16M9 12h12"/>',
    "graphics": '<path d="M4 20c4-1 6-4 7-8s4-7 9-8"/><circle cx="4" cy="20" r="1.5"/><circle cx="20" cy="4" r="1.5"/>'
                '<path d="M11 12h6"/><circle cx="17" cy="12" r="1"/>',
    "components": '<rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/>'
                  '<rect x="3" y="14" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/>',
    "themes": '<path d="M12 3a9 9 0 1 0 0 18c1.1 0 1.6-.9 1.2-1.8-.5-1-.1-2.2 1.2-2.2H17a4 4 0 0 0 4-4c0-5.5-4-10-9-10z"/>'
              '<circle cx="7.5" cy="11" r="1"/><circle cx="10" cy="7" r="1"/><circle cx="15" cy="7.5" r="1"/>',
    "languages": '<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3a14 14 0 0 1 0 18 14 14 0 0 1 0-18"/>',
    "tools": '<path d="M14.7 6.3a4 4 0 0 0-5.4 5.4L3 18l3 3 6.3-6.3a4 4 0 0 0 5.4-5.4l-2.6 2.6-2.4-.6-.6-2.4z"/>',
    "toolbox": '<rect x="3" y="8" width="18" height="12" rx="2"/><path d="M9 8V5h6v3M3 13h18"/>',
    # The page on borealis' (vs_body()): Cadran's dial, the styles' sliders,
    # accessibility's figure, the harness' flask, the density's lines.
    "cadran": '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.5 2"/>',
    "styles": '<path d="M4 6h9M17 6h3M4 12h3M11 12h9M4 18h11M19 18h1"/>'
              '<circle cx="15" cy="6" r="2"/><circle cx="9" cy="12" r="2"/><circle cx="17" cy="18" r="2"/>',
    "a11y": '<circle cx="12" cy="4.5" r="1.8"/><path d="M5 8.5 12 10l7-1.5M12 10v5M12 15l-3.5 6M12 15l3.5 6"/>',
    "test": '<path d="M9 3h6M10 3v6l-5.5 9.5A1.7 1.7 0 0 0 6 21h12a1.7 1.7 0 0 0 1.5-2.5L14 9V3M7.5 15h9"/>',
    "density": '<rect x="3" y="3" width="18" height="18" rx="2"/><path d="M7.5 8h9M7.5 12h9M7.5 16h9"/>',
}

# The README's sections (GitHub's anchors) and the home page's that say the same.
README_SECTIONS = {"-features": "features", "-what-aurore-adds": "adds", "-use-it-in-your-project": "start",
                   "-build-the-demo": "start", "-documentation": "docs", "-made-with-borealis": "made"}

ROOT = "."


# The console's two sides around the hero's screen (site.css's .device): a
# stick and a cross on the left, the four buttons and a stick on the right;
# a handheld of our own, no maker's. The same on Boréale's four sites.
DEVICE_GRIPS = (
    '<svg class="grip grip-l" viewBox="0 0 140 470" aria-hidden="true">'
    '<path d="M86 44h20"/><circle class="stick" cx="70" cy="128" r="34"/><circle class="cap" cx="70" cy="128" r="22"/>'
    '<circle cx="70" cy="262" r="12"/><circle cx="70" cy="330" r="12"/><circle cx="36" cy="296" r="12"/><circle cx="104" cy="296" r="12"/></svg>'
    '<svg class="grip grip-r" viewBox="0 0 140 470" aria-hidden="true">'
    '<path d="M34 44h20M44 34v20"/><circle cx="70" cy="92" r="12"/><circle cx="70" cy="160" r="12"/><circle cx="36" cy="126" r="12"/><circle cx="104" cy="126" r="12"/>'
    '<circle class="stick" cx="70" cy="300" r="34"/><circle class="cap" cx="70" cy="300" r="22"/></svg>')



# Links out say nothing of the visit (workbench rule "liens sortants
# anonymes"): the page sends no referrer (template.html's meta), a link to a
# third party carries rel="noreferrer"; a link to *.boreale.dev or to
# github.com/JigSawFr keeps the origin alone (referrerpolicy="strict-origin"),
# so GitHub's traffic insights still see our sites.
LINK_OUT = re.compile(r'<a\b[^<>]*?\bhref="(https?://[^"]*)"[^<>]*>')


def own_site(url: str) -> bool:
    """boreale.dev, its subdomains, and the repositories of github.com/JigSawFr/
    (that account's alone: another owner, a gist or GitHub's own pages are
    third parties)."""
    u = urlparse(url)
    host = (u.hostname or "").lower()
    return u.scheme == "https" and (host == "boreale.dev" or host.endswith(".boreale.dev") or
                                    (host == "github.com" and u.path.lower().startswith("/jigsawfr/")))


def link_out(tag: str, url: str) -> str:
    tag = re.sub(r'\sreferrerpolicy="[^"]*"', "", tag)
    rel = re.search(r'\srel="([^"]*)"', tag)
    words = [w for w in (rel.group(1).split() if rel else []) if w != "noreferrer"]
    if rel:
        tag = tag[:rel.start()] + tag[rel.end():]
    if own_site(url):
        extra = ' referrerpolicy="strict-origin"'
    else:
        words.append("noreferrer")
        extra = ""
    if words:
        extra = f' rel="{" ".join(words)}"' + extra
    return tag[:-1] + extra + ">"


def links_out(text: str) -> str:
    return LINK_OUT.sub(lambda m: link_out(m.group(0), m.group(1)), text)


def icon(name):
    return (f'<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" '
            f'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">{ICONS[name]}</svg>')


def read(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8") as f:
        return f.read()


class Texts:
    """The site's texts in one language: HTML, `{{repo}}` filled in."""

    def __init__(self, lang):
        self.lang = lang
        self.texts = json.loads(read("site", "i18n", f"{lang}.json"))

    def __call__(self, key):
        if key not in self.texts:
            raise SystemExit(f"site: no text {key!r} in site/i18n/{self.lang}.json")
        return self.texts[key].replace("{{repo}}", REPO_URL)

    def plain(self, key):
        """The text without its markup, for an attribute or a title."""
        return html.unescape(re.sub(r"<[^>]+>", "", self(key)))


# Boréale's sites, in the header's menu (switcher()): the same in each of
# Boréale's sites (boreale/tools/build_site.py's the reference). boreale.dev
# first, then the homebrew, then the libraries, each group under its title.
SITES = [(None, [("boreale", "Boréale", "https://boreale.dev/")]),
         ("sites_homebrew", [("playguard", "PlayGuard", "https://guard.boreale.dev/"),
                             ("playshelf", "PlayShelf", "https://shelf.boreale.dev/")]),
         ("sites_libraries", [("aurore", "Aurore", "https://aurore.boreale.dev/")])]


def boreale_versions():
    """The projects' latest releases as boreale.dev publishes them
    (versions.json), for the menu; none when it cannot be read (offline, a
    local build), and the menu shows no versions."""
    try:
        with urllib.request.urlopen("https://boreale.dev/versions.json", timeout=10) as r:
            return {k: v for k, v in json.load(r).items() if isinstance(v, str)}
    except (OSError, ValueError):
        return {}


def switcher(t, lang, to_root, home, here, name, versions):
    """The header's name (`name`), a button that opens the menu of Boréale's
    sites, `here` marked and leading to `home`, each in the page's language
    (the four sites have the same 15 folders), with its latest release (`versions`). A
    popover: the browser closes it on Esc or a click elsewhere; site.css's
    and site.js's "site switcher" blocks."""
    label = t("sites_label")
    folder = lang_dir(lang)   # the same folder on each of Boréale's sites
    groups = []
    for title, sites in SITES:
        items = []
        for key, site, url in sites:
            href = home if key == here else url + folder
            soon = f'<em class="soon">{t("sites_soon")}</em>' if key == "playshelf" else ""
            version = f' <span class="sites-version">{html.escape(versions[key])}</span>' if key in versions else ""
            current = ' aria-current="true"' if key == here else ""
            items.append(f'<li><a href="{href}"{current}>'
                         f'<img src="{to_root}assets/marks/{key}.svg" alt="" width="36" height="36">'
                         f'<span><strong>{site}{version}</strong><small>{t(f"sites_{key}")}</small></span>{soon}</a></li>')
        heading = f'<p class="sites-title">{t(title)}</p>' if title else ""
        groups.append(f'        {heading}<ul>\n          ' + "\n          ".join(items) + '\n        </ul>')
    title = next(site for _, sites in SITES for key, site, _ in sites if key == here)
    return (f'<div class="switcher">\n'
            f'      <button class="brand switch" type="button" popovertarget="sites" aria-label="{title} · {label}">'
            f'{name}<svg class="chevron" viewBox="0 0 16 16" aria-hidden="true"><path d="m4 6 4 4 4-4" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/></svg></button>\n'
            f'      <div id="sites" class="sites" popover aria-label="{label}">\n'
            + "\n".join(groups) + '\n      </div>\n    </div>')


SUN_ICON = ('<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" aria-hidden="true"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M2 12h2M20 12h2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></svg>')


def reading_toggle(t):
    """The documentation on a light ground or on the night (site.js, kept as
    aurore-reading; reading.js sets it before the first paint)."""
    return (f'<button class="reading-toggle" type="button" aria-pressed="false" aria-label="{t.plain("reading_light")}">'
            f'{SUN_ICON}<span>{t("reading_light")}</span></button>')


def punctuation(lang, mark):
    """A colon or a full stop the code adds after a text, as `lang` writes it:
    French's space before the colon, Chinese and Japanese full-width marks."""
    if lang.startswith(("ja", "zh")):
        return {":": "：", ".": "。"}[mark]
    if lang.startswith("fr") and mark == ":":
        return "\u00a0:"
    return mark


def lang_dir(lang):
    folder = next(l[1] for l in LANGUAGES if l[0] == lang)
    return f"{folder}/" if folder else ""


def html_lang(lang):
    return next(l[3] for l in LANGUAGES if l[0] == lang)


def flag(lang):
    """`lang`'s flag, two regional indicator letters, for the eyes only."""
    emoji = "".join(chr(0x1F1E6 + ord(c) - ord("A")) for c in FLAGS[lang])
    return f'<span class="flag" aria-hidden="true">{emoji}</span>'


def language_menu(t, lang, to_root, path):
    """The header's languages: a button showing the page's (its flag, its
    name, its code on a phone), and under it their menu, a popover as the
    menu of Boréale's sites, this page in each (site.css's and site.js's
    "languages" blocks; lang.js keeps the one chosen)."""
    name = next(n for c, _, n, _ in LANGUAGES if c == lang)
    current = ' aria-current="true"'
    options = "\n".join(
        f'        <li><a href="{to_root}{f"{d}/" if d else ""}{path}" data-lang="{c}" lang="{hl}" hreflang="{hl}"'
        f'{current if c == lang else ""}>{flag(c)}{html.escape(n)}</a></li>'
        for c, d, n, hl in LANGUAGES)
    return (f'<div class="settings">\n'
            f'      <button class="lang-button" type="button" popovertarget="languages" aria-label="{t.plain("language")} · {html.escape(name)}">'
            f'{flag(lang)}<span class="lang-name">{html.escape(name)}</span><span class="lang-code">{lang.split("-")[0].upper()}</span>'
            f'<svg class="chevron" viewBox="0 0 16 16" aria-hidden="true"><path d="m4 6 4 4 4-4" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/></svg></button>\n'
            f'      <ul id="languages" class="langs" popover aria-label="{t.plain("language")}">\n{options}\n      </ul>\n    </div>')


# ---- Code ------------------------------------------------------------------

CPP_KEYWORDS = {"auto", "bool", "break", "case", "char", "class", "const", "constexpr", "continue", "default",
                "delete", "do", "double", "else", "enum", "explicit", "false", "float", "for", "if", "inline", "int",
                "namespace", "new", "nullptr", "override", "private", "protected", "public", "return", "size_t",
                "static", "std", "struct", "switch", "template", "this", "true", "typename", "using", "virtual",
                "void", "while", "uint32_t", "int32_t", "uint8_t", "float"}
TOKENS = {
    "cpp": re.compile(r'(?P<c>//[^\n]*|/\*.*?\*/)|(?P<p>^[ \t]*#[^\n]*)|(?P<s>"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')'
                      r'|(?P<n>\b\d[\w.]*\b)|(?P<w>\b[A-Za-z_]\w*\b)', re.S | re.M),
    "xml": re.compile(r'(?P<c><!--.*?-->)|(?P<t></?[\w:.-]+|/?>)|(?P<a>\b[\w:-]+(?==))|(?P<s>"[^"]*")', re.S),
    "sh": re.compile(r'(?P<c>(?:^|(?<=\s))#[^\n]*)|(?P<s>"(?:\\.|[^"\\])*"|\'[^\']*\')|(?P<v>\$\w+|\$\{[^}]*\})'
                     r'|(?P<o>(?<=\s)--?[\w-]+(?:=\S*)?)', re.M),
}
CLASSES = {"c": "tok-comment", "p": "tok-pre", "s": "tok-string", "n": "tok-number", "t": "tok-tag",
           "a": "tok-attr", "v": "tok-var", "o": "tok-option", "w": "tok-keyword"}
ALIASES = {"c++": "cpp", "cxx": "cpp", "c": "cpp", "hpp": "cpp", "h": "cpp", "bash": "sh", "shell": "sh",
           "console": "sh", "cmake": "sh", "html": "xml", "svg": "xml"}


def highlight(code, lang):
    """`code` (raw text) as HTML, its comments, strings, tags and keywords in
    spans: a few colours, no library, nothing for a language not listed."""
    lang = ALIASES.get(lang, lang)
    pattern = TOKENS.get(lang)
    if not pattern:
        return html.escape(code, quote=False)
    out, at = [], 0
    for m in pattern.finditer(code):
        kind = m.lastgroup
        if kind == "w" and m.group(0) not in CPP_KEYWORDS:
            continue
        out.append(html.escape(code[at:m.start()], quote=False))
        out.append(f'<span class="{CLASSES[kind]}">{html.escape(m.group(0), quote=False)}</span>')
        at = m.end()
    out.append(html.escape(code[at:], quote=False))
    return "".join(out)


def highlight_blocks(body):
    """The fenced code of rendered Markdown, highlighted."""
    def block(m):
        lang = m.group(1) or ""
        return (f'<pre tabindex="0"><code class="language-{lang}">'
                f'{highlight(html.unescape(m.group(2)), lang.lower())}</code></pre>')
    return re.sub(r'<pre><code(?: class="language-([\w+-]+)")?>(.*?)</code></pre>', block, body, flags=re.S)


# ---- Markdown ----------------------------------------------------------------

def github_slug(value, separator="-"):
    """The anchor GitHub gives a heading, so links written for GitHub still work."""
    s = html.unescape(re.sub(r"<[^>]+>", "", value)).strip().lower()
    s = re.sub(r"[^\w\- ]", "", s)
    return s.replace(" ", separator)


ALERTS = {"NOTE": "note", "TIP": "tip", "IMPORTANT": "important", "WARNING": "warning", "CAUTION": "caution"}


def alerts(body):
    """GitHub's "> [!WARNING]" blockquotes as callouts."""
    body = re.sub(r"<blockquote>\s*<p>\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\s*(?:<br\s*/?>)?\s*",
                  lambda m: f'<div class="callout {ALERTS[m.group(1)]}">\n<p>', body)
    out, stack = [], []
    for token in re.split(r"(<blockquote>|</blockquote>|<div class=\"callout [a-z]+\">)", body):
        if token == "<blockquote>" or token.startswith('<div class="callout'):
            stack.append(token)
            out.append(token)
        elif token == "</blockquote>":
            opened = stack.pop() if stack else "<blockquote>"
            out.append("</div>" if opened.startswith("<div") else token)
        else:
            out.append(token)
    return "".join(out)


def repo_link(path, anchor=""):
    """A file or folder of the repository, on GitHub."""
    kind = "tree" if os.path.isdir(os.path.join(ROOT, path)) else "blob"
    return f"{REPO_URL}/{kind}/main/{path}{anchor}"


def rewrite_links(body, source, lang, to_root, assets):
    """Links of a document of the repository (`source`, its path) as the site's."""
    folder = os.path.dirname(source)
    pages = {p for p, _, _ in DOCS}

    def target(url):
        parts = urlsplit(url)
        if parts.scheme or url.startswith(("#", "mailto:", "//")):
            return url
        path = os.path.normpath(os.path.join(folder, parts.path)).replace(os.sep, "/")
        anchor = f"#{parts.fragment}" if parts.fragment else ""
        name = os.path.basename(path)
        if path.startswith("docs/") and name.endswith(".md") and name[:-3] in pages:
            return f"{to_root}{lang_dir(lang)}docs/{name[:-3]}/{anchor}"
        if path == "README.md":
            # The README's sections the home page has; the others on GitHub.
            if parts.fragment and parts.fragment not in README_SECTIONS:
                return f"{REPO_URL}#{parts.fragment}"
            return f"{to_root}{lang_dir(lang)}{'#' + README_SECTIONS[parts.fragment] if parts.fragment else ''}"
        if path == "CHANGELOG.md":
            return f"{to_root}{lang_dir(lang)}changelog/"
        if path.startswith(".github/readme/") and name.endswith((".png", ".gif")):
            assets.add(f"screenshots/{name}")
            return f"{to_root}assets/screenshots/{name}"
        return repo_link(path, anchor)

    return re.sub(r'(href|src)="([^"]+)"',
                  lambda m: f'{m.group(1)}="{html.escape(target(html.unescape(m.group(2))))}"', body)


def render_doc(page_name, lang, to_root, assets):
    """(title, html, toc) of docs/<page>.md."""
    source = f"docs/{page_name}.md"
    md = markdown.Markdown(extensions=["tables", "fenced_code", "sane_lists", "toc"],
                           extension_configs={"toc": {"slugify": github_slug, "toc_depth": "2-3"}})
    body = md.convert(read(source))
    m = re.search(r"<h1[^>]*>(.*?)</h1>", body, re.S)
    title = html.unescape(re.sub(r"<[^>]+>", "", m.group(1))) if m else page_name
    body = re.sub(r"<h1[^>]*>.*?</h1>", "", body, count=1, flags=re.S)
    body = alerts(rewrite_links(body, source, lang, to_root, assets))
    body = highlight_blocks(body)
    # A wide table scrolls on its own, not the page.
    body = body.replace("<table>", '<div class="table" tabindex="0"><table>').replace("</table>", "</table></div>")
    return title, body, md.toc_tokens


def plain(html_text):
    return re.sub(r"\s+", " ", html.unescape(re.sub(r"<[^>]+>", "", html_text))).strip()


# ---- The changelog -----------------------------------------------------------

# A release's heading in CHANGELOG.md (release-please's): "## [1.2.0](link) (2026-10-18)".
RELEASE = re.compile(r"^## \[?(\d+\.\d+\.\d+)\]?(?:\([^)]*\))? \((\d{4}-\d\d-\d\d)\)\s*$", re.M)


def releases():
    """[(version, date, html)] of CHANGELOG.md, the newest first."""
    text = read("CHANGELOG.md")
    heads = list(RELEASE.finditer(text))
    out = []
    for i, m in enumerate(heads):
        end = heads[i + 1].start() if i + 1 < len(heads) else len(text)
        body = markdown.markdown(text[m.end():end].replace("\n### ", "\n#### ").strip(), extensions=["sane_lists"])
        out.append((m.group(1), m.group(2), body.replace("<h4>", "<h3>").replace("</h4>", "</h3>")))
    return out


# ---- Pages -------------------------------------------------------------------

def json_ld(data):
    """schema.org data: a data block, not a script, which the CSP lets be."""
    text = json.dumps(data, ensure_ascii=False, indent=1).replace("</", "<\\/")
    return f'<script type="application/ld+json">\n{text}\n</script>\n'


def page(t, lang, to_root, path, title, body, description=None, home=False, structured=(), indexed=True,
         markdown_url=None, lost=False, night=False):
    """A whole page: `path` is its place under the language's folder ('' or 'docs/x/')."""
    code, folder, _, hl = next(l for l in LANGUAGES if l[0] == lang)
    description = description or t.plain("meta_description")
    full_title = title or f"Aurore — {t.plain('hero_title_plain')}"
    url = f"{SITE_URL}{lang_dir(lang)}{path}"
    head = []
    if indexed:
        head.append(f'<link rel="canonical" href="{url}">')
        head += [f'<link rel="alternate" hreflang="{h}" href="{SITE_URL}{f"{d}/" if d else ""}{path}">'
                 for _, d, _, h in LANGUAGES]
        head.append(f'<link rel="alternate" hreflang="x-default" href="{SITE_URL}{path}">')
    if markdown_url:
        head.append(f'<link rel="alternate" type="text/markdown" href="{markdown_url}">')
    if NOINDEX or not indexed:
        head.append('<meta name="robots" content="noindex">')
    first_visit = " data-first-visit" if home and lang == "en-US" else ""
    base = f"{to_root}{lang_dir(lang)}"
    api = f'<li><a href="{to_root}api/">{t("nav_api")}</a></li>' if API_DIR else ""
    api += f'<li><a class="nav-try" href="{to_root}demo/{demo_query(lang)}">{t("nav_demo")}</a></li>' if DEMO_DIR else ""
    # The aurora behind the home page's hero, and the page not found's (body
    # a pair, then: what stands under the sky, and the rest).
    # The page on borealis (night) stands under it too, the night going on
    # down the page as the home page's.
    aurora = home or lost or night
    sky = '<canvas id="sky" aria-hidden="true"></canvas>' if aurora else ""
    hero = body[0] if aurora else ""
    main = body[1] if aurora else body
    # A page of the documentation: once the header is scrolled away, a slim
    # bar at the top with the way home, the page's name, the search and the
    # light reading (site.js).
    doc_bar = ""
    doc_title = re.search(r'<div class="wrap doc">.*?<h1[^>]*>(.*?)</h1>', main, re.S)
    if doc_title:
        doc_bar = (f'<div class="doc-bar" hidden><div class="wrap doc-bar-inner">'
                   f'<a class="doc-bar-home" href="{base}" aria-label="Aurore"><img src="{to_root}assets/mark.svg" alt="" width="24" height="24"><span>Aurore</span></a>'
                   f'<span class="doc-bar-title">{doc_title.group(1)}</span>'
                   f'<button class="doc-bar-search" type="button" aria-label="{t.plain("search_placeholder")}">'
                   f'<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" aria-hidden="true">'
                   f'<circle cx="10.5" cy="10.5" r="6.5"/><path d="m15.5 15.5 5 5"/></svg></button>'
                   f'{reading_toggle(t)}</div></div>\n')
    sites_dir = lang_dir(lang)   # Boréale's other sites, in the page's language (the same folders)
    docs_links = "".join(f'<li><a href="{base}docs/{p}/">{html.escape(n)}</a></li>'
                         for p, g, n in DOCS if g == "start")
    umami = "" if NOINDEX else (f'<script src="{to_root}assets/umami.js" defer data-website-id="{UMAMI_WEBSITE}" '
                               f'data-domains="{UMAMI_DOMAIN}"></script>\n')
    return f"""<!doctype html>
<html lang="{hl}" data-lang="{code}"{first_visit}>
<head>
<meta charset="utf-8">
<meta name="referrer" content="no-referrer">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta http-equiv="Content-Security-Policy" content="{CSP}">
<title>{html.escape(full_title)}</title>
<meta name="description" content="{html.escape(description)}">
<meta name="theme-color" content="#0b1424">
{chr(10).join(head)}
<link rel="icon" href="{to_root}assets/mark.svg" type="image/svg+xml">
<link rel="icon" href="{to_root}assets/icon.png" type="image/png" sizes="256x256">
<link rel="apple-touch-icon" href="{to_root}assets/icon.png">
<link rel="manifest" href="{to_root}manifest.webmanifest">
<link rel="alternate" type="application/atom+xml" href="{SITE_URL}releases.atom" title="Aurore releases">
<link rel="preload" href="{to_root}assets/fonts/sora.woff2" as="font" type="font/woff2" crossorigin>
<link rel="preload" href="{to_root}assets/fonts/figtree.woff2" as="font" type="font/woff2" crossorigin>
<script src="{to_root}assets/reading.js"></script>
<link rel="stylesheet" href="{to_root}assets/site.css">
<meta property="og:type" content="website">
<meta property="og:site_name" content="Aurore">
<meta property="og:title" content="{html.escape(full_title)}">
<meta property="og:description" content="{html.escape(description)}">
<meta property="og:url" content="{url}">
<meta property="og:image" content="{SITE_URL}assets/social-preview.png">
<meta property="og:image:width" content="1280">
<meta property="og:image:height" content="640">
<meta property="og:image:alt" content="Aurore">
<meta property="og:locale" content="{OG_LOCALES[code]}">
<meta name="twitter:card" content="summary_large_image">
{"".join(json_ld(d) for d in structured)}<script src="{to_root}assets/lang.js"></script>
<script src="{to_root}assets/site.js" defer></script>
{umami}</head>
<body class="{'home' if home or night else 'inner'}" data-copy="{t.plain('copy')}" data-copied="{t.plain('copied')}">
<a class="skip" href="#main">{t("skip")}</a>
{doc_bar}<div class="sky{' sky-home' if aurora else ''}">
  {sky}
  <header class="wrap top">
    {switcher(t, lang, to_root, base, "aurore", f'<img src="{to_root}assets/mark.svg" alt="" width="34" height="34">Aurore', VERSIONS)}
    <nav aria-label="{t.plain('nav_label')}">
      <ul>
        <li><a href="{base}#features">{t("nav_features")}</a></li>
        <li><a href="{base}docs/getting-started/">{t("nav_docs")}</a></li>
        <li class="nav-vs"><a href="{base}borealis/"{' aria-current="page"' if path == "borealis/" else ""}>{t("nav_borealis")}</a></li>
        {api}
        <li class="nav-gh"><a href="{REPO_URL}">GitHub</a></li>
      </ul>
    </nav>
    {language_menu(t, lang, to_root, path)}
  </header>
{hero}
</div>
<main id="main">
{main}
</main>
<footer class="site">
  <div class="wrap footer-cols">
    <div>
      <p class="footer-brand"><img src="{to_root}assets/mark.svg" alt="" width="24" height="24"> <strong>Aurore</strong></p>
      <p>{t("footer_license")}</p>
      <p><a href="https://boreale.dev/{sites_dir}">{t("footer_boreale")}</a></p>
    </div>
    <div>
      <h2>{t("docs_title")}</h2>
      <ul>{docs_links}{f'<li><a href="{to_root}api/">{t("api_title")}</a></li>' if API_DIR else ""}</ul>
    </div>
    <div>
      <h2>Boréale</h2>
      <ul><li><a href="https://boreale.dev/{sites_dir}">boreale.dev</a></li>
        <li><a href="https://guard.boreale.dev/{sites_dir}">PlayGuard</a></li>
        <li><a href="https://shelf.boreale.dev/{sites_dir}">PlayShelf</a></li>
        <li><a href="https://boreale.dev/{sites_dir}legal/">{t("sites_legal")}</a></li>
        <li><a href="mailto:contact@boreale.dev">contact@boreale.dev</a></li></ul>
    </div>
    <div>
      <h2>{t("footer_project")}</h2>
      <ul><li><a href="{REPO_URL}">GitHub</a></li><li><a href="{base}changelog/">{t("changelog")}</a></li>
        <li><a href="{base}borealis/">{t("vs_footer")}</a></li>
        <li><a href="{REPO_URL}/blob/main/CONTRIBUTING.md">{t("footer_contributing")}</a></li>
        <li><a href="{REPO_URL}/issues/new">{t("footer_issues")}</a></li>
        <li><a href="https://github.com/xfangfang/borealis">{t("footer_upstream")}</a></li></ul>
    </div>
  </div>
  <p class="wrap fine">{t("footer_nintendo")}</p>
</footer>
<dialog class="lightbox" aria-label="{t.plain('screens_title')}">
  <figure><img alt=""><figcaption></figcaption></figure>
  <button type="button" class="lightbox-close" aria-label="{t.plain('close')}"></button>
  <button type="button" class="lightbox-prev" aria-label="{t.plain('previous')}"></button>
  <button type="button" class="lightbox-next" aria-label="{t.plain('next')}"></button>
</dialog>
</body>
</html>
"""


HELLO_XML = """<!-- resources/xml/activity/hello.xml -->
<aurore:AppletFrame>
    <aurore:Box
        title="Hello"
        axis="column"
        alignItems="center"
        justifyContent="center"
        gap="20">

        <aurore:Label
            text="@i18n/hello/welcome"
            fontSize="24" />

        <aurore:Button
            id="hello_button"
            style="primary"
            width="300"
            text="Say hello" />

    </aurore:Box>
</aurore:AppletFrame>"""

HELLO_CPP = """// hello_activity.hpp
#include <aurore.hpp>

class HelloActivity : public aurore::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/hello.xml");

    void onContentAvailable() override
    {
        button->registerClickAction([](aurore::View*) {
            aurore::Application::notify("Hello!");
            return true;
        });
    }

  private:
    AURORE_BIND(aurore::Button, button, "hello_button");
};"""

DEMO_SH = """git clone --recursive https://github.com/JigSawFr/aurore.git
cd aurore
cmake --preset desktop-glfw
cmake --build --preset desktop-glfw
./build/desktop-glfw/aurore_demo"""

SUBMODULE_SH = """git submodule add -b main https://github.com/JigSawFr/aurore.git library/aurore
git submodule update --init --recursive"""


def code_block(code, lang, label=None, t=None):
    caption = f'<figcaption>{label}</figcaption>' if label else ""
    return (f'<figure class="code">{caption}<pre tabindex="0"><code class="language-{lang}">{highlight(code, lang)}</code></pre>'
            f'</figure>')


def editor(t):
    """The home page's screen in XML and its activity in C++, in an editor's
    window (two tabs, site.js; both blocks one under the other without
    script), beside the screen they make, drawn in HTML in Aurore's look:
    the frame's header with the box's title, the label (its translation,
    @i18n/hello/welcome) and the "Say hello" button, which shows the
    activity's "Hello!" (site.js)."""
    panes = []
    for key, code, lang, name, path in (("xml", HELLO_XML, "xml", "XML", "resources/xml/activity/hello.xml"),
                                         ("cpp", HELLO_CPP, "cpp", "C++", "hello_activity.hpp")):
        panes.append(f'<figure class="code editor-pane" id="code-{key}" data-tab="{name}">'
                     f'<figcaption><span class="pane-name">{t(f"code_{key}")}</span> <span class="pane-file">{path.replace("/", "/<wbr>")}</span></figcaption>'
                     f'<pre tabindex="0"><code class="language-{lang}">{highlight(code, lang)}</code></pre></figure>')
    return f"""    <div class="code-pair">
      <div class="editor">
        <div class="editor-bar"><span class="editor-dots" aria-hidden="true"><i></i><i></i><i></i></span></div>
        <div class="editor-panes">
          {panes[0]}
          {panes[1]}
        </div>
      </div>
      <figure class="preview">
        <figcaption class="preview-bar">{t("code_result")}</figcaption>
        <div class="preview-stage">
          <div class="preview-screen">
            <p class="preview-head">Hello</p>
            <div class="preview-body">
              <p class="preview-label">{t("code_welcome")}</p>
              <button class="preview-button" type="button" data-notice="Hello!">Say hello</button>
            </div>
            <p class="preview-notice" role="status"></p>
            <p class="preview-foot"><span class="preview-key" aria-hidden="true">B</span> {t("code_back")}</p>
          </div>
        </div>
      </figure>
    </div>"""


def release_line(t, base):
    found = releases()
    if not found:
        return f'<p class="release">{t("release_none")}</p>'
    version, date, _ = found[0]
    return (f'<p class="release">{t("release_latest")}{punctuation(t.lang, ":")} <a href="{base}changelog/#v{version}">v{version}</a> '
            f'· <time datetime="{date}">{date}</time></p>')


def demo_audience(page):
    """The published demo counted as the site's pages are (UMAMI_WEBSITE): the
    site's copy of the tracker, and Umami's address in the page's connect-src.
    Not in a preview (NOINDEX), as the pages' tracker isn't."""
    with open(page, encoding="utf-8") as f:
        text = f.read()
    csp = "connect-src 'self';"
    if text.count(csp) != 1 or text.count("</head>") != 1:
        sys.exit(f"build_site: {page} has not the one connect-src and </head> expected")
    tag = (f'<script src="../assets/umami.js" defer data-website-id="{UMAMI_WEBSITE}" '
           f'data-domains="{UMAMI_DOMAIN}"></script>\n</head>')
    text = text.replace(csp, "connect-src 'self' https://gateway.umami.is;").replace("</head>", tag)
    with open(page, "w", encoding="utf-8") as f:
        f.write(text)


def demo_query(lang):
    """The demo in the page's language (the browser's for English)."""
    return "" if lang == "en-US" else f"?lang={lang}"


def hero_actions(t, lang, to_root, base):
    if DEMO_DIR:
        return (f'<a class="button primary" href="{to_root}demo/{demo_query(lang)}">{t("cta_demo")}</a>\n'
                f'        <a class="button ghost" href="{base}docs/getting-started/">{t("cta_start")}</a>')
    return (f'<a class="button primary" href="{base}docs/getting-started/">{t("cta_start")}</a>\n'
            f'        <a class="button ghost" href="{REPO_URL}">{t("cta_github")}</a>')


def demo_band(t, lang, to_root):
    """The home page's invitation to the demo in the browser."""
    if not DEMO_DIR:
        return ""
    return f"""
  <section class="wrap" id="demo">
    <div class="band">
      <div>
        <p class="eyebrow">{t("demo_eyebrow")}</p>
        <h2>{t("demo_band_title")}</h2>
        <p>{t("demo_band_text")}</p>
      </div>
      <a class="button primary" href="{to_root}demo/{demo_query(lang)}">{t("cta_demo")}</a>
    </div>
  </section>
"""


# The figures of "Under the hood" (tools/stack.py's), in this order
STACK_FIGURES = [("ci", "{ci_jobs}"), ("tests", "{tests}"), ("refs", "{references}"),
                 ("actions", "{actions_pinned}/{actions}")]
STACK_CHECKS = ["sanitizers", "fuzz", "deprecated", "format", "notices", "upstream"]
STACK = {}  # tools/stack.py's facts, read once (stack_facts())


def stack_facts():
    if not STACK:
        STACK.update(under_the_hood.facts())
    return STACK


def stack_section(t):
    """Under the hood: what Aurore is built with and how it is checked, read
    from the repository when the site is built (tools/stack.py)."""
    facts = stack_facts()
    figures = "\n".join(
        f'      <li class="stat"><p class="stat-figure"><span class="now">{value.format(**facts["figures"])}</span></p>'
        f'<p class="stat-text">{t(f"stack_fig_{k}")}</p></li>'
        for k, value in STACK_FIGURES)
    versions = "\n".join(
        f'          <li><span>{t("stack_language") if name == "Language" else html.escape(name)}</span>'
        f'<code>{html.escape(version)}</code></li>'
        for name, version in facts["versions"])
    checks = "\n".join(f"          <li>{t(f'stack_check_{k}')}</li>" for k in STACK_CHECKS)
    return f"""  <section class="wrap" id="stack">
    <div class="section-head">
      <p class="eyebrow">{t("stack_eyebrow")}</p>
      <h2>{t("stack_title")}</h2>
      <p>{t("stack_text")}</p>
    </div>
    <ul class="glance">
{figures}
    </ul>
    <div class="columns stack">
      <div>
        <h3>{t("stack_versions")}</h3>
        <ul class="versions">
{versions}
        </ul>
      </div>
      <div>
        <h3>{t("stack_checks")}</h3>
        <ul class="checks">
{checks}
        </ul>
        <p><a href="{REPO_URL}#-under-the-hood">{t("stack_link")} →</a></p>
      </div>
    </div>
  </section>"""


def png_size(path):
    """A PNG's width and height, read from its header"""
    with open(path, "rb") as f:
        head = f.read(24)
    return int.from_bytes(head[16:20], "big"), int.from_bytes(head[20:24], "big")


def consoles(t, to_root):
    items = []
    for key, name, screen in CONSOLES:
        label = name or t.plain(f"console_{key}")
        width, height = png_size(os.path.join(ROOT, ".github", "readme", f"console-{key}.png"))
        alt = html.escape(t.plain("console_alt").replace("{console}", label))
        src = f"{to_root}assets/screenshots/console-{key}.png"
        items.append(f'      <li><a href="{src}" data-caption="{alt}"><img src="{src}" alt="{alt}" width="{width}" height="{height}" loading="lazy"></a>'
                     f'<p><strong>{html.escape(label)}</strong><span>{screen}</span></p></li>')
    return "\n".join(items)


def home_body(t, lang, to_root):
    base = f"{to_root}{lang_dir(lang)}"
    platforms = "".join(f"<li>{html.escape(p)}</li>" for p in PLATFORMS)
    features = "\n".join(
        f'      <li class="card">{icon(k)}<h3>{t(f"f_{k}_title")}</h3><p>{t(f"f_{k}_text")}</p></li>'
        for k in FEATURES)
    shots = "\n".join(
        f'      <figure><a href="{to_root}assets/screenshots/{s}.png" data-caption="{t.plain(f"shot_{s}")}">'
        f'<img src="{to_root}assets/screenshots/{s}.png" alt="{html.escape(t.plain(f"shot_{s}"))}" width="1280" height="720" loading="lazy">'
        f'</a><figcaption>{t(f"shot_{s}")}</figcaption></figure>'
        for s in SHOTS)
    adds = "\n".join(f"        <li>{t(f'add_{k}')}</li>" for k in ADDS)
    made = "\n".join(f'      <li><a href="{url}">{html.escape(name)}</a><span>{t(f"made_{key}")}</span></li>'
                     for name, url, key in MADE_WITH)
    hero = f"""  <section class="wrap hero">
    <div class="hero-text">
      <p class="eyebrow">{t("hero_eyebrow")}</p>
      <h1>{t("hero_title")}</h1>
      <p class="lede">{t("hero_lede")}</p>
      <div class="actions">
        {hero_actions(t, lang, to_root, base)}
      </div>
      {release_line(t, base)}
    </div>
    <!-- The demo on a console's screen: a handheld drawn by site.css
         (.device), leaning towards the text on a wide screen. -->
    <figure class="hero-shot device">
      {DEVICE_GRIPS}
      <a href="{to_root}assets/screenshots/cadran.png" data-caption="{t.plain('shot_cadran')}"><img src="{to_root}assets/screenshots/cadran.png" alt="{html.escape(t.plain('hero_shot_alt'))}" width="1280" height="720"></a>
    </figure>
    <div class="platforms">
      <h2>{t("platforms_label")}</h2>
      <ul>{platforms}</ul>
    </div>
  </section>"""
    main = f"""  <section class="wrap code-section" id="code">
    <div class="section-head">
      <h2>{t("code_title")}</h2>
      <p>{t("code_text")}</p>
      <p><a href="{base}docs/getting-started/#3-a-screen-from-xml-3-minutes">{t("code_link")} →</a></p>
    </div>
{editor(t)}
  </section>

{demo_band(t, lang, to_root)}
  <section class="wrap" id="features">
    <div class="section-head">
      <p class="eyebrow">{t("features_eyebrow")}</p>
      <h2>{t("features_title")}</h2>
    </div>
    <ul class="cards">
{features}
    </ul>
  </section>

  <section class="wrap cadran" id="cadran">
    <div class="cadran-text">
      <p class="eyebrow">{t("cadran_eyebrow")}</p>
      <h2>{t("cadran_title")}</h2>
      <p>{t("cadran_text")}</p>
      <p><a href="{base}docs/cadran/">{t("cadran_link")} →</a></p>
    </div>
    <figure>
      <a href="{to_root}assets/screenshots/winter.png" data-caption="{t.plain('shot_winter')}"><img src="{to_root}assets/screenshots/winter.png" alt="{html.escape(t.plain('shot_winter'))}" width="1280" height="720" loading="lazy"></a>
    </figure>
  </section>

  <section class="wrap" id="screens">
    <div class="section-head">
      <p class="eyebrow">{t("screens_eyebrow")}</p>
      <h2>{t("screens_title")}</h2>
    </div>
    <div class="gallery">
{shots}
    </div>
  </section>

  <section class="wrap" id="consoles">
    <div class="section-head">
      <p class="eyebrow">{t("consoles_eyebrow")}</p>
      <h2>{t("consoles_title")}</h2>
      <p>{t("consoles_text")}</p>
      <p><a href="{base}docs/platforms/#simulated-platforms">{t("consoles_link")} →</a></p>
    </div>
    <ul class="consoles">
{consoles(t, to_root)}
    </ul>
  </section>

  <section class="wrap" id="adds">
    <div class="adds">
      <div class="section-head">
        <p class="eyebrow">{t("adds_eyebrow")}</p>
        <h2>{t("adds_title")}</h2>
        <p>{t("adds_text")}</p>
        <p><a href="{REPO_URL}#-what-aurore-adds">{t("adds_link")} →</a></p>
        <p><a href="{base}borealis/">{t("vs_home_link")} →</a></p>
      </div>
      <ul class="checks">
{adds}
      </ul>
    </div>
  </section>

{stack_section(t)}

  <section class="wrap" id="start">
    <div class="section-head">
      <p class="eyebrow">{t("start_eyebrow")}</p>
      <h2>{t("start_title")}</h2>
    </div>
    <div class="columns">
      <div>
        <ol class="steps">
          <li>{t("start_1")}</li>
          <li>{t("start_2")}{code_block(SUBMODULE_SH, "sh")}</li>
          <li>{t("start_3")}</li>
          <li>{t("start_4")}</li>
        </ol>
        <p class="callout tip">{t("start_template")}</p>
        <p class="callout note">{t("start_borealis")} <a href="{base}docs/migrating-from-borealis/">→</a></p>
      </div>
      <div>
        <h3>{t("demo_title")}</h3>
        <p class="muted">{t("demo_text")} <a href="{base}docs/platforms/">→</a></p>
        {code_block(DEMO_SH, "sh")}
      </div>
    </div>
  </section>

  <section class="wrap" id="made">
    <div class="section-head">
      <p class="eyebrow">{t("made_eyebrow")}</p>
      <h2>{t("made_title")}</h2>
    </div>
    <ul class="made">
{made}
    </ul>
  </section>

  <section class="wrap" id="docs">
    <div class="section-head">
      <p class="eyebrow">{t("docs_eyebrow")}</p>
      <h2>{t("docs_title")}</h2>
    </div>
{docs_list(t, lang, to_root, None)}
  </section>

  <section class="wrap closing">
    <div class="promise">
      <h2>{t("license_title")}</h2>
      <p>{t("license_text")}</p>
    </div>
  </section>"""
    return hero, main


# ---- Aurore and borealis -----------------------------------------------------
# What Aurore adds to or changes from xfangfang/borealis' wiliwili branch, the
# commit it starts from (vs_body()). Only what the code, its history and its
# documentation show; the speed figures are the README's, not here.

UPSTREAM_URL = "https://github.com/xfangfang/borealis/tree/wiliwili"
# The headline differences: (key, borealis', Aurore's).
VS_GLANCE = [("languages", "4", "15"), ("cpp", "C++17", "C++23"), ("yoga", "Yoga 2", "Yoga 3"), ("platforms", "7", "8")]
# What borealis has nothing of: (key, icon).
VS_NEW = [("cadran", "cadran"), ("themes", "themes"), ("styles", "styles"), ("a11y", "a11y"),
          ("text", "languages"), ("harness", "test"), ("inspector", "tools"), ("density", "density")]
# What both have, side by side, by theme: (key, borealis', Aurore's); None is
# the catalogue's vs_<key>_was / vs_<key>_now, a string the same in every language.
VS_SIDES = [
    ("look", [("look", None, None), ("colours", None, None), ("layout", "Yoga 2.0.0", None),
              ("corners", None, None), ("highlight", None, None), ("images", None, None)]),
    ("text", [("catalogues", None, None), ("switch", None, None), ("lookups", None, None),
              ("fonts", None, None), ("keyboard", None, None), ("focus", None, None)]),
    ("build", [("cpp", "C++17", None), ("target", "<code>borealis</code>", "<code>Aurore::aurore</code>, <code>Aurore::test</code>"),
               ("options", "<code>PLATFORM_*</code>, <code>USE_*</code>, <code>BRLS_*</code>", None),
               ("systems", None, None), ("platforms", None, None),
               ("libraries", "Yoga 2.0.0, SDL 2.28.5, tinyxml2 9.0.0, nlohmann/json 3.11, glad 0.1.27",
                "Yoga 3.2.1, SDL 2.32.10, tinyxml2 11.0.0, nlohmann/json 3.12.0, glad 2.0.8"),
               ("sounds", None, None), ("links", None, None), ("licence", "Apache 2.0", None)]),
]
VS_FIXES = ["counter", "destroyed", "hidden", "controllers", "recycler", "fonts", "logger", "disabled"]
# The authors of the commits picked from other forks, by their GitHub
# handle (those with an account linked to the commit).
VS_PICKERS = ["Nika0000", "XITRIX", "carlchina", "xlanor", "dragonflylee", "kawaii-flesh", "PalindromicBreadLoaf",
              "AMSNX", "eliasrosa", "youngzyl", "FayeSpica", "Jackiechen259", "Kiriox94"]
# The names an app changes: borealis' line, then Aurore's.
VS_RENAMES = [
    ("cpp", "#include <borealis.hpp>", "#include <aurore.hpp>"),
    ("cpp", "brls::Application::init();", "aurore::Application::init();"),
    ("cpp", 'BRLS_BIND(brls::Button, button, "go");', 'AURORE_BIND(aurore::Button, button, "go");'),
    ("xml", '<brls:Box axis="column">', '<aurore:Box axis="column">'),
    ("xml", 'textColor="@theme/brls/text"', 'textColor="@theme/aurore/text"'),
    ("sh", "target_link_libraries(app borealis)", "target_link_libraries(app PRIVATE Aurore::aurore)"),
    ("sh", "cmake -DPLATFORM_DESKTOP=ON -DUSE_SDL2=ON", "cmake -DAURORE_PLATFORM=DESKTOP -DAURORE_WINDOW=SDL2"),
    ("sh", "BRLS_LANG=fr ./app", "AURORE_LANG=fr ./app"),
]
VS_MOVE = ["names", "cpp", "licence"]
VS_PROJECT = ["docs", "ci", "releases", "security", "template", "notices"]


def vs_rename_editor(t):
    """The names an app changes, as a diff in the editor's window."""
    lines = []
    for lang, old, new in VS_RENAMES:
        lines.append(f'<span class="del"><span class="sign" aria-hidden="true">−</span>{highlight(old, lang)}</span>')
        lines.append(f'<span class="add"><span class="sign" aria-hidden="true">+</span>{highlight(new, lang)}</span>')
    return (f'<figure class="vs-diff">\n'
            f'        <div class="editor-bar"><span class="editor-dots" aria-hidden="true"><i></i><i></i><i></i></span>'
            f'<span class="vs-diff-name">{t("vs_move_file")}</span></div>\n'
            f'        <pre class="no-copy" tabindex="0"><code>{"".join(lines)}</code></pre>\n'
            f'      </figure>')


def vs_body(t, lang, to_root):
    """The page on what Aurore adds to borealis: (what stands under the sky, the rest)."""
    base = f"{to_root}{lang_dir(lang)}"
    migrate = f"{base}docs/migrating-from-borealis/"
    jumps = "".join(f'<li><a href="#{k}">{t(f"vs_jump_{k}")}</a></li>' for k in ("new", "sides", "fixes", "move", "project"))
    lineage = "\n".join(
        f'        <li class="{cls}"><strong>{name}</strong><span>{t(f"vs_line_{key}")}</span></li>'
        for cls, key, name in (("node", "origin", "natinusala/borealis"), ("node", "wiliwili", "xfangfang/borealis · wiliwili"),
                               ("pick", "forks", t("vs_line_forks_name")), ("pick", "playguard", t("vs_line_playguard_name")),
                               ("node now", "aurore", "JigSawFr/Aurore")))
    glance = "\n".join(
        f'      <li class="stat"><p class="stat-figure"><span class="sr-only">borealis: </span><span class="was">{was}</span>'
        f'<span class="to" aria-hidden="true">→</span><span class="sr-only">, Aurore: </span><span class="now">{now}</span></p>'
        f'<p class="stat-text">{t(f"vs_glance_{k}")}</p></li>'
        for k, was, now in VS_GLANCE)
    new = "\n".join(
        f'      <li class="card">{icon(i)}<h3>{t(f"vs_new_{k}_title")}</h3><p>{t(f"vs_new_{k}_text")}</p></li>'
        for k, i in VS_NEW)
    tables = []
    for group, rows in VS_SIDES:
        body = "\n".join(
            f'          <tr><th scope="row">{t(f"vs_{group}_{k}")}</th>'
            f'<td class="was" data-side="borealis">{was if was is not None else t(f"vs_{group}_{k}_was")}</td>'
            f'<td class="now" data-side="Aurore">{now if now is not None else t(f"vs_{group}_{k}_now")}</td></tr>'
            for k, was, now in rows)
        tables.append(f"""      <div class="sides">
        <h3 id="sides-{group}">{t(f"vs_group_{group}")}</h3>
        <table aria-labelledby="sides-{group}">
          <thead><tr><td></td><th scope="col">borealis <small>wiliwili</small></th><th scope="col"><span class="now-mark">Aurore</span></th></tr></thead>
          <tbody>
{body}
          </tbody>
        </table>
      </div>""")
    fixes = "\n".join(f"        <li>{t(f'vs_fix_{k}')}</li>" for k in VS_FIXES)
    pickers = "".join(f'<li><a href="https://github.com/{h}">{h}</a></li>' for h in VS_PICKERS)
    move = "\n".join(f"          <li>{t(f'vs_move_{k}')}</li>" for k in VS_MOVE)
    project = "\n".join(f'      <li><h3>{t(f"vs_project_{k}_title")}</h3><p>{t(f"vs_project_{k}_text")}</p></li>'
                        for k in VS_PROJECT)
    hero = f"""  <section class="wrap hero vs-hero">
    <div class="hero-text">
      <p class="eyebrow">{t("vs_eyebrow")}</p>
      <h1>{t("vs_title")}</h1>
      <p class="lede">{t("vs_lede")}</p>
      <div class="actions">
        <a class="button primary" href="{migrate}">{t("vs_cta_migrate")}</a>
        <a class="button ghost" href="{UPSTREAM_URL}">{t("vs_cta_upstream")}</a>
      </div>
      <p class="release">{t("vs_compared")}</p>
    </div>
    <figure class="lineage">
      <figcaption class="editor-bar"><span class="editor-dots" aria-hidden="true"><i></i><i></i><i></i></span><span>{t("vs_line_caption")}</span></figcaption>
      <ol>
{lineage}
      </ol>
    </figure>
    <nav class="platforms vs-jump" aria-label="{t.plain("vs_jump_label")}">
      <h2>{t("vs_jump_label")}</h2>
      <ul>{jumps}</ul>
    </nav>
  </section>"""
    main = f"""  <section class="wrap" id="glance">
    <h2 class="sr-only">{t("vs_glance_title")}</h2>
    <ul class="glance">
{glance}
    </ul>
  </section>

  <section class="wrap" id="new">
    <div class="section-head">
      <p class="eyebrow">{t("vs_new_eyebrow")}</p>
      <h2>{t("vs_new_title")}</h2>
      <p>{t("vs_new_text")}</p>
    </div>
    <ul class="cards">
{new}
    </ul>
  </section>

  <section class="wrap" id="sides">
    <div class="section-head">
      <p class="eyebrow">{t("vs_sides_eyebrow")}</p>
      <h2>{t("vs_sides_title")}</h2>
      <p>{t("vs_sides_text")}</p>
    </div>
{chr(10).join(tables)}
  </section>

  <section class="wrap" id="fixes">
    <div class="adds">
      <div class="section-head">
        <p class="eyebrow">{t("vs_fixes_eyebrow")}</p>
        <h2>{t("vs_fixes_title")}</h2>
        <p>{t("vs_fixes_text")}</p>
      </div>
      <div class="vs-fixes">
        <ul class="checks">
{fixes}
        </ul>
        <div class="pickers">
          <h3>{t("vs_pickers_title")}</h3>
          <p>{t("vs_pickers_text")}</p>
          <ul>{pickers}</ul>
        </div>
      </div>
    </div>
  </section>

  <section class="wrap" id="move">
    <div class="vs-move">
      <div class="section-head">
        <p class="eyebrow">{t("vs_move_eyebrow")}</p>
        <h2>{t("vs_move_title")}</h2>
        <p>{t("vs_move_text")}</p>
        <ul class="checks">
{move}
        </ul>
        <p><a href="{migrate}">{t("vs_move_link")} →</a></p>
      </div>
      {vs_rename_editor(t)}
    </div>
  </section>

  <section class="wrap" id="project">
    <div class="section-head">
      <p class="eyebrow">{t("vs_project_eyebrow")}</p>
      <h2>{t("vs_project_title")}</h2>
    </div>
    <ul class="facts">
{project}
    </ul>
  </section>

  <section class="wrap closing">
    <div class="band">
      <div>
        <h2>{t("vs_band_title")}</h2>
        <p>{t("vs_band_text")}</p>
      </div>
      <div class="actions">
        <a class="button primary" href="{migrate}">{t("vs_cta_migrate")}</a>
        <a class="button ghost" href="{REPO_URL}#-what-aurore-adds">{t("adds_link")}</a>
      </div>
    </div>
  </section>"""
    return hero, main


def docs_list(t, lang, to_root, current):
    base = f"{to_root}{lang_dir(lang)}"
    groups = []
    for group in GROUPS:
        items = "".join(
            f'<li><a href="{base}docs/{p}/"{" aria-current=\"page\"" if p == current else ""}>{html.escape(n)}</a></li>'
            for p, g, n in DOCS if g == group)
        groups.append(f'<div class="doc-group"><h3>{t(f"docs_{group}")}</h3><ul>{items}</ul></div>')
    if API_DIR:
        groups.append(f'<div class="doc-group"><h3>API</h3><ul><li><a href="{to_root}api/">{t("api_title")}</a></li></ul></div>')
    return '    <div class="doc-list">\n      ' + "\n      ".join(groups) + "\n    </div>"


def doc_body(t, lang, to_root, page_name, assets):
    title, body, toc = render_doc(page_name, lang, to_root, assets)
    note = f'<p class="callout note">{t("doc_untranslated")}</p>\n' if t("doc_untranslated") else ""
    toc_items = "".join(f'<li><a href="#{h["id"]}">{h["name"]}</a></li>' for h in toc)
    # A link to each section, to share it.
    body = re.sub(r'<(h[23]) id="([^"]+)">(.*?)</h[23]>',
                  lambda m: f'<{m.group(1)} id="{m.group(2)}">{m.group(3)}<a class="anchor" href="#{m.group(2)}" '
                            f'aria-label="{t.plain("anchor_link")}"></a></{m.group(1)}>', body, flags=re.S)
    names = [p for p, _, _ in DOCS]
    at = names.index(page_name)
    base = f"{to_root}{lang_dir(lang)}"
    pager = "".join(
        f'<a class="{cls}" href="{base}docs/{names[i]}/"><span>{t(label)}</span>{html.escape(DOCS[i][2])}</a>'
        for cls, label, i in (("prev", "previous", at - 1), ("next", "next", at + 1)) if 0 <= i < len(names))
    first = re.search(r"<p>(.*?)</p>", body, re.S)
    description = plain(first.group(1)) if first else title
    if len(description) > 180:
        description = description[:177].rsplit(" ", 1)[0] + "…"
    folded = (f'<details class="toc-mobile"><summary>{t("on_this_page")}</summary><ul>{toc_items}</ul></details>\n'
              if toc_items else "")
    search = (f'<div class="doc-search" hidden><input type="search" placeholder="{t.plain("search_placeholder")}" '
              f'aria-label="{t.plain("search_placeholder")}" aria-controls="doc-results" autocomplete="off" '
              f'spellcheck="false" enterkeyhint="search" data-index="{base}docs/search.json" '
              f'data-none="{t.plain("search_none")}"><kbd class="search-key" aria-hidden="true">/</kbd>'
              f'<ol id="doc-results" class="doc-results" hidden></ol></div>')
    return title, description, body, f"""  <div class="wrap doc">
    <nav class="doc-nav" aria-label="{t.plain('docs_title')}">
      {reading_toggle(t)}
      {search}
      <details class="doc-menu" open><summary>{t("docs_title")}</summary>
{docs_list(t, lang, to_root, page_name)}
      </details>
    </nav>
    <article class="prose" data-copy="{t.plain('copy')}" data-copied="{t.plain('copied')}" lang="en">
      <h1 id="{github_slug(title)}">{html.escape(title)}</h1>
      {note}{folded}{body}
      <nav class="pager" aria-label="{t.plain('docs_title')}">{pager}</nav>
      <p class="edit"><a href="{REPO_URL}/blob/main/docs/{page_name}.md">{t("doc_on_github")}</a></p>
    </article>
    {f'<nav class="toc" aria-label="{t.plain("on_this_page")}"><p>{t("on_this_page")}</p><ul>{toc_items}</ul></nav>' if toc_items else ""}
  </div>"""


def changelog_body(t, lang):
    found = releases()
    note = f'<p class="callout note">{t("changelog_english")}</p>\n' if t("changelog_english") and found else ""
    parts = []
    if not found:
        parts.append(f'<p>{t("changelog_none")} <a href="{REPO_URL}/commits/main">{t("changelog_commits")}</a>{punctuation(t.lang, ".")}</p>')
    for version, date, body in found:
        parts.append(f'<section class="release-entry"><h2 id="v{version}">{version} '
                     f'<small><time datetime="{date}">{date}</time></small></h2>\n{body}\n'
                     f'<p><a href="{REPO_URL}/releases/tag/v{version}">GitHub · v{version}</a></p></section>')
    return f"""  <div class="wrap narrow">
    <article class="prose">
      <h1>{t("changelog_title")}</h1>
      <p class="muted">{t("changelog_intro")}</p>
      {note}{chr(10).join(parts)}
      <p class="edit"><a href="{SITE_URL}releases.atom">Atom</a> · <a href="{REPO_URL}/blob/main/CHANGELOG.md">CHANGELOG.md</a></p>
    </article>
  </div>"""


def search_entries(page_name, title, body):
    """The search index' entries of a page: its introduction, then each section."""
    parts = re.split(r'<h2 id="([^"]*)">(.*?)</h2>', body, flags=re.S)
    entries = [{"p": title, "h": "", "u": f"docs/{page_name}/", "t": plain(parts[0])[:1500]}]
    for i in range(1, len(parts), 3):
        entries.append({"p": title, "h": plain(parts[i + 1]), "u": f"docs/{page_name}/#{parts[i]}",
                        "t": plain(parts[i + 2])[:3000]})
    return entries


# ---- For search engines and AI assistants --------------------------------------

def software(t, lang):
    found = releases()
    data = {
        "@context": "https://schema.org",
        "@type": "SoftwareSourceCode",
        "name": "Aurore",
        "url": f"{SITE_URL}{lang_dir(lang)}",
        "description": t.plain("meta_description"),
        "codeRepository": REPO_URL,
        "programmingLanguage": "C++",
        "runtimePlatform": PLATFORMS,
        "license": "https://www.apache.org/licenses/LICENSE-2.0",
        "image": f"{SITE_URL}assets/social-preview.png",
        "isBasedOn": "https://github.com/xfangfang/borealis",
        "author": {"@type": "Person", "name": "JigSawFr", "url": "https://github.com/JigSawFr"},
        "isPartOf": {"@type": "Organization", "name": "Boréale", "url": "https://boreale.dev/"},
    }
    if found:
        data["version"] = found[0][0]
    return data


def doc_data(t, lang, page_name, title, description):
    home = f"{SITE_URL}{lang_dir(lang)}"
    url = f"{home}docs/{page_name}/"
    return [
        {"@context": "https://schema.org", "@type": "TechArticle", "headline": title, "description": description,
         "url": url, "inLanguage": "en", "about": {"@type": "SoftwareSourceCode", "name": "Aurore"},
         "isPartOf": {"@type": "WebSite", "name": "Aurore", "url": home}},
        {"@context": "https://schema.org", "@type": "BreadcrumbList", "itemListElement": [
            {"@type": "ListItem", "position": 1, "name": "Aurore", "item": home},
            {"@type": "ListItem", "position": 2, "name": t.plain("docs_title"), "item": f"{home}docs/getting-started/"},
            {"@type": "ListItem", "position": 3, "name": title, "item": url}]},
    ]


def markdown_doc(page_name, assets):
    """docs/<page>.md as published next to its page: its links absolute."""
    source = f"docs/{page_name}.md"
    pages = {p for p, _, _ in DOCS}

    def target(m):
        url = m.group(2)
        parts = urlsplit(url)
        if parts.scheme or url.startswith(("#", "mailto:")):
            return m.group(0)
        path = os.path.normpath(os.path.join("docs", parts.path)).replace(os.sep, "/")
        anchor = f"#{parts.fragment}" if parts.fragment else ""
        name = os.path.basename(path)
        if path.startswith("docs/") and name.endswith(".md") and name[:-3] in pages:
            url = f"{SITE_URL}docs/{name[:-3]}.md{anchor}"
        elif path == "README.md":
            url = f"{SITE_URL}{anchor}"
        else:
            url = repo_link(path, anchor)
        return f"{m.group(1)}{url}{m.group(3)}"

    return re.sub(r"(\]\()([^)\s]+)(\))", target, read(source))


def llms_txt(t, descriptions, full=None):
    """llms.txt (llmstxt.org): what Aurore is and where its documentation is."""
    lines = ["# Aurore", "", f"> {t.plain('meta_description')}", "", t.plain("hero_lede"), "",
             f"- {t.plain('cadran_text')}",
             f"- {t.plain('start_borealis')}",
             "- Built with: " + ", ".join(v if n == "Language" else f"{n} {v}" for n, v in stack_facts()["versions"]),
             f"- {t.plain('license_text')}",
             f"- Source: {REPO_URL}", ""]
    for group in GROUPS:
        lines += [f"## {t.plain(f'docs_{group}')}", ""]
        lines += [f"- [{name}]({SITE_URL}docs/{p}.md): {descriptions[p]}" for p, g, name in DOCS if g == group]
        lines.append("")
    lines += ["## Optional", "", f"- [Changelog]({SITE_URL}changelog/)",
              f"- [{t.plain('vs_page_title')}]({SITE_URL}borealis/): {t.plain('vs_description')}",
              f"- [Contributing]({REPO_URL}/blob/main/CONTRIBUTING.md)"]
    if API_DIR:
        lines.append(f"- [API reference (Doxygen)]({SITE_URL}api/)")
    if DEMO_DIR:
        lines.append(f"- [The demo in a browser (Emscripten, WebGL 2)]({SITE_URL}demo/)")
    if not full:
        lines.append(f"- [The whole documentation in one file]({SITE_URL}llms-full.txt)")
        return "\n".join(lines) + "\n"
    return "\n".join(lines) + "\n\n" + "\n\n".join(full) + "\n"


def releases_atom():
    found = releases()
    entries = "".join(f"""  <entry>
    <title>Aurore {version}</title>
    <id>{REPO_URL}/releases/tag/v{version}</id>
    <link rel="alternate" type="text/html" href="{SITE_URL}changelog/#v{version}"/>
    <updated>{date}T00:00:00Z</updated>
    <content type="html">{html.escape(body)}</content>
  </entry>
""" for version, date, body in found)
    updated = found[0][1] if found else "2026-10-01"
    return f"""<?xml version="1.0" encoding="utf-8"?>
<feed xmlns="http://www.w3.org/2005/Atom">
  <title>Aurore releases</title>
  <subtitle>A Switch-like interface for your app, on every screen</subtitle>
  <id>{SITE_URL}releases.atom</id>
  <link rel="self" href="{SITE_URL}releases.atom"/>
  <link rel="alternate" type="text/html" href="{SITE_URL}changelog/"/>
  <updated>{updated}T00:00:00Z</updated>
  <author><name>JigSawFr</name><uri>{REPO_URL}</uri></author>
  <icon>{SITE_URL}assets/mark.svg</icon>
{entries}</feed>
"""


def sitemap(paths):
    entries = []
    for path in paths:
        alternates = "".join(f'\n    <xhtml:link rel="alternate" hreflang="{h}" href="{SITE_URL}{f"{d}/" if d else ""}{path}"/>'
                             for _, d, _, h in LANGUAGES)
        alternates += f'\n    <xhtml:link rel="alternate" hreflang="x-default" href="{SITE_URL}{path}"/>'
        for _, d, _, _ in LANGUAGES:
            entries.append(f'  <url>\n    <loc>{SITE_URL}{f"{d}/" if d else ""}{path}</loc>{alternates}\n  </url>')
    return ('<?xml version="1.0" encoding="UTF-8"?>\n'
            '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9" xmlns:xhtml="http://www.w3.org/1999/xhtml">\n'
            + "\n".join(entries) + "\n</urlset>\n")


def redirect_page(to, canonical, lang):
    """A page that sends its visitor on (GitHub Pages has no redirects)."""
    return (f'<!doctype html>\n<html lang="{lang}">\n<meta charset="utf-8">\n<meta name="referrer" content="no-referrer">\n<meta name="robots" content="noindex">\n'
            f'<meta http-equiv="refresh" content="0; url={to}">\n<link rel="canonical" href="{canonical}">\n'
            f'<title>Aurore</title>\n<a href="{to}">{canonical}</a>\n')


def favicon_ico(png):
    """An ICO holding one PNG, what a domain's root is asked for."""
    width, height = struct.unpack(">II", png[16:24])
    return (struct.pack("<HHH", 0, 1, 1)
            + struct.pack("<BBBBHHII", width % 256, height % 256, 0, 0, 1, 32, len(png), 22) + png)


def write(out, rel, text):
    path = os.path.join(out, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if rel.endswith(".html"):
        text = links_out(text)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def main():
    global ROOT
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    ROOT = sys.argv[2] if len(sys.argv) > 2 else "."
    global VERSIONS
    VERSIONS = boreale_versions()
    os.makedirs(out, exist_ok=True)
    assets = {f"screenshots/{s}.png" for s in SHOTS} | {f"screenshots/console-{c[0]}.png" for c in CONSOLES}
    count = 0
    descriptions, full = {}, []
    for lang, folder, _, _ in LANGUAGES:
        t = Texts(lang)
        base = f"{folder}/" if folder else ""
        to_root = "../" if folder else ""
        write(out, f"{base}index.html",
              page(t, lang, to_root, "", "", home_body(t, lang, to_root), home=True,
                   structured=[{"@context": "https://schema.org", "@type": "WebSite", "name": "Aurore",
                                "url": f"{SITE_URL}{base}", "inLanguage": html_lang(lang)}, software(t, lang)]))
        index = []
        for name, _, short in DOCS:
            doc_root = to_root + "../../"
            title, description, content, body = doc_body(t, lang, doc_root, name, assets)
            index += search_entries(name, short, content)
            descriptions[name] = description
            write(out, f"{base}docs/{name}/index.html",
                  page(t, lang, doc_root, f"docs/{name}/", f"{title} · Aurore", body, description=description,
                       structured=doc_data(t, lang, name, title, description),
                       markdown_url=f"{SITE_URL}docs/{name}.md"))
            if lang == "en-US":
                text = markdown_doc(name, assets)
                write(out, f"docs/{name}.md", text)
                full.append(text)
            count += 1
        write(out, f"{base}docs/index.html",
              redirect_page("getting-started/", f"{SITE_URL}{base}docs/getting-started/", lang))
        write(out, f"{base}docs/search.json", json.dumps(index, ensure_ascii=False, separators=(",", ":")))
        write(out, f"{base}changelog/index.html",
              page(t, lang, to_root + "../", "changelog/", f"{t.plain('changelog_title')} · Aurore",
                   changelog_body(t, lang)))
        write(out, f"{base}borealis/index.html",
              page(t, lang, to_root + "../", "borealis/", f"{t.plain('vs_page_title')} · Aurore",
                   vs_body(t, lang, to_root + "../"), description=t.plain("vs_description"), night=True))
        count += 3
    t = Texts("en-US")
    write(out, "llms.txt", llms_txt(t, descriptions))
    write(out, "llms-full.txt", llms_txt(t, descriptions, full))
    write(out, "sitemap.xml", sitemap([""] + ["borealis/"] + [f"docs/{p}/" for p, _, _ in DOCS] + ["changelog/"]))
    write(out, "releases.atom", releases_atom())
    at_root = urlsplit(SITE_URL).path == "/"
    if at_root:
        write(out, "robots.txt", f"User-agent: *\nAllow: /\n\nSitemap: {SITE_URL}sitemap.xml\n")
    # 404.html, served at any address: its links start from the site's root,
    # every language in it, site.js shows the address' or the visitor's.
    root_path = urlsplit(SITE_URL).path
    blocks = []
    for lang, folder, _, _ in LANGUAGES:
        tl = Texts(lang)
        blocks.append(f'<div class="not-found" lang="{html_lang(lang)}" data-lang="{lang}"{" hidden" if lang != "en-US" else ""}>'
                      f'<h1>{tl("not_found_title")}</h1><p>{tl("not_found_text")}</p>'
                      f'<p class="actions"><a class="button primary" href="{root_path}{f"{folder}/" if folder else ""}">{tl("home")}</a> '
                      f'<a class="button ghost" href="{root_path}{f"{folder}/" if folder else ""}docs/getting-started/">{tl("docs_title")}</a></p></div>')
    write(out, "404.html", page(t, "en-US", root_path, "", f"{t.plain('not_found_title')} · Aurore",
                                ('  <div class="wrap lost">\n' + "\n".join(blocks) + "\n  </div>", ""), indexed=False, lost=True))
    write(out, "manifest.webmanifest", json.dumps({
        "name": "Aurore", "short_name": "Aurore", "start_url": "./", "scope": "./", "display": "browser",
        "background_color": "#0b1424", "theme_color": "#0b1424",
        "icons": [{"src": "assets/mark.svg", "sizes": "any", "type": "image/svg+xml"},
                  {"src": "assets/icon.png", "sizes": "256x256", "type": "image/png"}]}, indent=1) + "\n")
    # The site's own files, then the README's screenshots and Aurore's icons.
    shutil.copytree(os.path.join(ROOT, "site", "assets"), os.path.join(out, "assets"), dirs_exist_ok=True)
    shutil.copy(os.path.join(ROOT, ".github", "branding", "icon.png"), os.path.join(out, "assets", "icon.png"))
    shutil.copy(os.path.join(ROOT, ".github", "branding", "social-preview.png"),
                os.path.join(out, "assets", "social-preview.png"))
    for path in sorted(assets):
        os.makedirs(os.path.join(out, "assets", os.path.dirname(path)), exist_ok=True)
        shutil.copy(os.path.join(ROOT, ".github", "readme", os.path.basename(path)), os.path.join(out, "assets", path))
    if at_root:
        with open(os.path.join(ROOT, ".github", "branding", "icon.png"), "rb") as f, \
                open(os.path.join(out, "favicon.ico"), "wb") as ico:
            ico.write(favicon_ico(f.read()))
    if API_DIR:
        shutil.copytree(API_DIR, os.path.join(out, "api"), dirs_exist_ok=True)
    if DEMO_DIR:
        shutil.copytree(DEMO_DIR, os.path.join(out, "demo"), dirs_exist_ok=True)
        if not NOINDEX:
            demo_audience(os.path.join(out, "demo", "index.html"))
    open(os.path.join(out, ".nojekyll"), "w").close()
    print(f"site: {count} pages in {len(LANGUAGES)} languages, {len(assets)} images"
          f"{', the API reference' if API_DIR else ''}{', the demo' if DEMO_DIR else ''}, in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
