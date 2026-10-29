# The website

Aurore's website, on GitHub Pages: <https://aurore.boreale.dev/>. English at the root, French in `/fr/`; one dark look, Boréale's (the night, the aurora: [boreale/branding](https://github.com/JigSawFr/boreale/tree/main/branding)).

- **The home page**: what Aurore is, a screen in XML and its activity, the features, Cadran, the screenshots, what Aurore adds to borealis, how to use it in a project and build the demo, the apps made with it, the documentation, the licence.
- **Reading the documentation**: once the header is scrolled away, a slim bar at the top (the way home, the page's name, the search, the light reading); *Light reading* puts the documentation's pages on a light ground (kept as `aurore-reading`, `reading.js` setting it before the first paint); the search's field shows its key, `/`.
- **The footer** has a Boréale column (boreale.dev, PlayGuard, PlayShelf, the privacy and legal notice, `contact@boreale.dev`), and the menu of Boréale's sites shows the projects' versions, read from boreale.dev's `versions.json` at the build. The page not found stands under the aurora, as the home page's hero.
- **The home page's showcase** (site.css's and site.js's "showcase" blocks, the same on Boréale's four sites, PlayGuard's the reference): the demo on a console of our own (`.device`, its sides `DEVICE_GRIPS`), leaning towards the text on a wide screen; the screen in XML and its activity in an editor's window, a tab for each file, beside the screen they make drawn in HTML in Aurore's look (its button shows the activity's "Hello!"); the night going on down the page (veils of aurora, faint stars, a horizon over the footer); the features on tiles, their edge lit under the pointer; the sections rising into view once; on a wide screen, the screenshots as one large over a strip of the others; the steps joined by a line; a short cross-fade from one page to the next where the browser can. Without script, or with less motion asked, everything stays in place, both files of the editor one under the other.
- **The documentation**: `docs/*.md`, converted at each build. It is written in English; the French site shows the same pages under its own header, saying so. Links between the docs and to the README become the site's pages, any other file of the repository links to GitHub, GitHub's `> [!NOTE]` blocks become callouts, headings keep GitHub's anchors. A search box above the pages searches `docs/search.json`; `/` goes to it.
- **The API reference**, Doxygen's (`docs/Doxyfile`), in `/api/`.
- **The demo in a browser** ([web/README.md](../web/README.md)) in `/demo/`, which the home page and the header offer to try.
- **The changelog** (`/changelog/`, from `CHANGELOG.md`) and `releases.atom`.
- **The page not found** (`404.html`, served at any address), in both languages.
- **For search engines and AI assistants**: absolute links, `sitemap.xml`, schema.org data, the preview image (`.github/branding/social-preview.png`), each page of the documentation in Markdown next to it, listed in [`llms.txt`](https://llmstxt.org) (`llms-full.txt`: all of it in one file).

## How it is made

`tools/build_site.py <out-dir>` (Python 3 and Python-Markdown) writes the pages; no framework.

- `site/i18n/<language>.json`: the texts around the content, one catalog per language (`en.json` the reference). They are HTML; `{{repo}}` is the repository's address. A new language is a catalog and its line in `LANGUAGES`.
- `site/assets/`: `site.css`, `site.js` (the aurora behind the home page, the screenshots' lightbox, the copy buttons, the search, the table of contents), `lang.js` (a first visit from a French browser goes to `/fr/`; the header's link changes it, remembered as `aurore-lang`), `mark.svg` (Boréale's drawing of Aurore's icon) and the fonts (Sora, Figtree, JetBrains Mono, SIL Open Font License), served with the site so that no visitor's browser loads anything from anyone else.
- **Boréale's sites**, in a menu the header's name opens, the site's own leading home (`switcher()` in `build_site.py`, a popover the browser closes on Esc or a click elsewhere): boreale.dev, the homebrew, the libraries, in French on a French page. Its CSS and script are the "site switcher" blocks of `site.css` and `site.js`, copied as they are from boreale.dev's (the reference), and the marks are boreale's (`site/assets/marks/`).
- The screenshots are the README's (`.github/readme/`), the icon and the preview image Aurore's (`.github/branding/`).
- The documentation's sidebar is `DOCS` in `build_site.py`: a new page of `docs/` is a line there.
- `SITE_URL` is the site's address; `SITE_NOINDEX=1` keeps a build out of search engines; `SITE_API=<folder>` publishes Doxygen's HTML in `/api/` and links to it; `SITE_DEMO=<folder>` the demo built for a browser (`build-web/web/`) in `/demo/`.
- **The audience**, counted by Umami Cloud (<https://cloud.umami.is>): `assets/umami.js` is its tracker, served with the site, that sends each page seen to `gateway.umami.is`, without cookie, and only from the site's own address (`SITE_URL`'s, not a local copy); a pull request's build has none. It is a copy: take it again from <https://cloud.umami.is/script.js> when Umami changes it. Its site in Umami (`UMAMI_WEBSITE` in `build_site.py`) is Boréale's, shared with the other projects' sites: Umami's *Hostname* filter tells them apart.
- A **Content-Security-Policy** on every page (a `<meta>`: GitHub Pages sends no headers): only the site's own files, no inline script or style; the one address it may send to is Umami's (`connect-src`).

To look at it locally:

```sh
python3 tools/build_site.py /tmp/site
python3 -m http.server -d /tmp/site 8000      # http://localhost:8000/
```

With the API reference: `doxygen docs/Doxyfile`, then `SITE_API=build_docs/html python3 tools/build_site.py /tmp/site`.

## Checks

- `tools/check_site.py <site> [site-url]`: the catalogs have the same keys, every link to the site and every `#anchor` leads somewhere, nothing the Content-Security-Policy blocks is on a page, the JSON reads.
- `tools/site_layout.py <site>` (Playwright for Python): every page on a phone, the widest also on a small phone, a phone in landscape, a tablet and a desktop: none wider than the screen, no script that throws, and nothing axe-core finds against WCAG 2.2 AA (`SITE_AXE=<axe.min.js>`).

## Publishing

`.github/workflows/site.yml` builds the demo for a browser, the API reference and the site, and runs both checks for each pull request (the `site` artifact) and for main, whose site it publishes on GitHub Pages.

In the repository's settings, *Pages › Build and deployment › Source: GitHub Actions*, with the custom domain `aurore.boreale.dev`; the workflow reads the address from Pages.
