#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
#
# The demo in a browser (web/README.md), started in headless Chromium: fails
# when the page throws, says the demo cannot start or has stopped, or when
# the demo does not draw its first screen or answer the keyboard.
#
#   - served with the headers threads need (as `python3 -m http.server`
#     cannot): the demo starts at once;
#   - served without them, as GitHub Pages does: the service worker (sw.js)
#     adds them and the page loads once more under it;
#   - ?lang=fr&theme=winter: the address' language and a theme folder,
#     the demo drawn otherwise.
#
# In each, the first screen is not blank, and a press of the down arrow moves
# the focus (the screen changes). The canvas of each is written to
# <out-dir>/<name>.png, and what the demo logged to <out-dir>/<name>.log.
#
#     python3 tools/web_smoke.py build-web/web build-web/smoke
#
# Needs Playwright for Python with Chromium (python -m playwright install
# chromium); PLAYWRIGHT_CHROMIUM=<path> runs a browser of one's own.

import functools
import http.server
import io
import os
import sys
import threading
import time

from playwright.sync_api import sync_playwright

START_S = 120  # SwiftShader draws WebGL on the CPU: a start takes 10 to 30 s


class Handler(http.server.SimpleHTTPRequestHandler):
    isolate = True

    def end_headers(self):
        if self.isolate:
            self.send_header("Cross-Origin-Opener-Policy", "same-origin")
            self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, *args):
        pass


def serve(site, isolate):
    handler = type("Handler", (Handler,), {"isolate": isolate})
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(handler, directory=site))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def spread(png):
    """How far apart the darkest and lightest pixels of a screenshot are, 0 to 255"""
    from PIL import Image
    gray = Image.open(io.BytesIO(png)).convert("L")
    low, high = gray.getextrema()
    return high - low


def differs(a, b):
    """The share of pixels that changed between two screenshots"""
    from PIL import Image, ImageChops
    one, two = (Image.open(io.BytesIO(x)).convert("RGB") for x in (a, b))
    diff = ImageChops.difference(one, two).convert("L").point(lambda v: 255 if v > 24 else 0)
    return diff.histogram()[255] / (one.width * one.height)


def scenario(browser, site, out, name, isolate, query=""):
    server = serve(site, isolate)
    url = f"http://127.0.0.1:{server.server_address[1]}/{query}"
    context = browser.new_context(viewport={"width": 1400, "height": 1000}, locale="en-US")
    page = context.new_page()
    errors, log = [], []
    page.on("pageerror", lambda e: errors.append(str(e)))
    page.on("console", lambda m: log.append(f"[{m.type}] {m.text}"))
    started = time.monotonic()
    try:
        page.goto(url)
        # The page hides #loading on the demo's first frame, and shows #error
        # when it cannot start or has stopped
        page.wait_for_function(
            "() => document.getElementById('loading').hidden || !document.getElementById('error').hidden",
            timeout=START_S * 1000)
        if page.is_visible("#error"):
            raise AssertionError(f"the page says: {page.inner_text('#error-text')}")
        took = time.monotonic() - started
        if not page.evaluate("window.crossOriginIsolated"):
            raise AssertionError("the page is not cross-origin isolated")
        page.wait_for_timeout(1500)  # the screen's opening animations
        canvas = page.locator("#canvas")
        first = canvas.screenshot()
        with open(os.path.join(out, f"{name}.png"), "wb") as f:
            f.write(first)
        if spread(first) < 40:
            raise AssertionError("the first screen is blank")
        canvas.focus()
        page.keyboard.press("ArrowDown")
        page.wait_for_timeout(1000)
        moved = differs(first, canvas.screenshot())
        if moved < 0.002:
            raise AssertionError(f"the down arrow changed nothing on screen ({moved:.2%} of the pixels)")
        if errors:
            raise AssertionError("the page threw: " + "; ".join(errors))
        print(f"{name}: started in {took:.1f} s, the down arrow changed {moved:.1%} of the screen")
        return True
    except Exception as e:  # each scenario says what failed, the others still run
        print(f"{name}: FAILED: {e}", file=sys.stderr)
        for line in log[-30:]:
            print(f"    {line}", file=sys.stderr)
        return False
    finally:
        with open(os.path.join(out, f"{name}.log"), "w", encoding="utf-8") as f:
            f.write("\n".join(log) + "\n")
        context.close()
        server.shutdown()


def main(args):
    if len(args) != 2:
        sys.exit("usage: tools/web_smoke.py <the built page, build-web/web> <out-dir>")
    site, out = args
    if not os.path.isfile(os.path.join(site, "aurore_demo.wasm")):
        sys.exit(f"web_smoke: no aurore_demo.wasm in {site}: build the demo for a browser first (web/README.md)")
    os.makedirs(out, exist_ok=True)
    with sync_playwright() as p:
        browser = p.chromium.launch(executable_path=os.environ.get("PLAYWRIGHT_CHROMIUM") or None,
                                    args=["--enable-unsafe-swiftshader", "--use-angle=swiftshader"])
        results = [
            scenario(browser, site, out, "isolated", isolate=True),
            scenario(browser, site, out, "service_worker", isolate=False),
            scenario(browser, site, out, "french_winter", isolate=True, query="?lang=fr&theme=winter"),
        ]
        browser.close()
    if not all(results):
        sys.exit(1)


if __name__ == "__main__":
    main(sys.argv[1:])
