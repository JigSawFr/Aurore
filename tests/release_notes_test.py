#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
"""tools/release_notes.py: the page of a release, laid out."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
import release_notes as rn  # noqa: E402

REPO = "https://github.com/JigSawFr/Aurore"
NOTES = """## 1.1.0 (2026-11-20)

### ⚠ BREAKING CHANGES

* **api:** a view's `setFoo()` is gone

### Features

* **view:** a new thing ([#1](https://github.com/JigSawFr/Aurore/issues/1)) ([abc1234](https://github.com/JigSawFr/Aurore/commit/abc1234))
* **layout:** another ([#2](https://github.com/JigSawFr/Aurore/issues/2))

### Bug Fixes

* **core:** a fix ([#3](https://github.com/JigSawFr/Aurore/issues/3))

### Documentation

* **guides:** words ([#4](https://github.com/JigSawFr/Aurore/issues/4))
"""
SUMMARY = """### 🌟 Highlights

A line.

* 🎛️ **Cadran**: the only look.
* 📱 **Small screens**: compact layouts.

Read the breaking changes first.

### 🤝 Credits

Thanks to all.

* 🍒 **Forks**: [someone](https://github.com/someone) and others.
"""


class ReleaseNotes(unittest.TestCase):
    def page(self, **kw):
        kw.setdefault("summary", SUMMARY)
        return rn.layout(NOTES, "1.1.0", REPO, **kw)

    def test_heading_and_figures(self):
        page = self.page(compare={"total_commits": 12, "commits": []})
        self.assertTrue(page.startswith("## 1.1.0 · 2026-11-20\n"))
        for fig in ("<b>1</b> breaking change", "<b>2</b> features", "<b>1</b> fix"):
            self.assertIn(fig, page)
        self.assertNotIn("security", page)

    def test_lists_folded_users_then_contributors(self):
        page = self.page()
        self.assertIn("<summary><b>Every change in 1.1.0</b> (4 changes)</summary>", page)
        self.assertIn("<summary><b>🛠️ For contributors: docs</b> (1 change)</summary>", page)
        self.assertLess(page.index("a new thing"), page.index("words"))

    def test_summary_bullets_and_important_box_per_part(self):
        page = self.page()
        self.assertIn("* 🎛️ **Cadran**:  \n  The only look.", page)
        self.assertIn("> [!IMPORTANT]\n> Read the breaking changes first.", page)
        # The second part has no bullet after its last bullet: no second box.
        self.assertEqual(page.count("[!IMPORTANT]"), 1)
        self.assertIn("Thanks to all.", page)
        self.assertNotIn("> Thanks to all.", page)

    def test_idempotent(self):
        once = self.page()
        self.assertEqual(rn.layout(once, "1.1.0", REPO), once)
        self.assertEqual(rn.layout(once, "1.1.0", REPO, summary=SUMMARY), once)

    def test_without_summary(self):
        page = rn.layout(NOTES, "1.1.0", REPO)
        self.assertNotIn("Highlights", page)
        self.assertIn("## 📦 Use it", page)

    def test_contributors_are_links_not_mentions(self):
        compare = {"total_commits": 1, "commits": [{"author": {"login": "someone"}}, {"author": {"login": "bot[bot]"}}, {}]}
        page = self.page(compare=compare)
        self.assertIn("**Contributors**: [someone](https://github.com/someone)", page)
        self.assertNotIn("@", page.replace("](mailto", ""))
        self.assertNotIn("bot[bot]", page)

    def test_first_release_has_no_compare(self):
        page = rn.layout(NOTES.replace("## 1.1.0", "## [1.1.0](x/compare/v1.0.0...v1.1.0)"), "1.1.0", REPO)
        self.assertIn("/compare/v1.0.0...v1.1.0", page)
        first = self.page(contributors=["b", "a"])
        self.assertIn("every commit", first)
        self.assertLess(first.index("[a]("), first.index("[b]("))

    def test_install_names_the_series(self):
        self.assertIn("find_package(Aurore 1.1 REQUIRED)", self.page())
        self.assertIn("AURORE_VERSION_AT_LEAST(1, 1, 0)", self.page())

    def test_site_links_carry_the_utm_of_the_release(self):
        page = self.page()
        self.assertIn("https://aurore.boreale.dev/?utm_source=release&utm_medium=release-notes&utm_campaign=v1.1.0", page)

    def test_no_empty_section_in_the_figures(self):
        page = rn.layout("## 1.1.1 (2026-11-21)\n\n### Bug Fixes\n\n* **core:** x\n", "1.1.1", REPO)
        self.assertIn("<b>1</b> fix", page)
        self.assertNotIn("feature", page)


if __name__ == "__main__":
    unittest.main()
