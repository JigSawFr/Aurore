#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0
"""tools/build_site.py: Aurore's own version in the menu of Boréale's sites."""
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "tools"))
import build_site  # noqa: E402

CHANGELOG = """# Changelog

## [1.1.0](https://example.org/compare/v1.0.0...v1.1.0) (2026-11-20)

### Features

* **view:** a thing

## 1.0.0 (2026-10-31)

### Features

* **view:** the first
"""


class OwnVersion(unittest.TestCase):
    def setUp(self):
        self.root = build_site.ROOT
        self.dir = tempfile.TemporaryDirectory()
        build_site.ROOT = self.dir.name

    def tearDown(self):
        build_site.ROOT = self.root
        self.dir.cleanup()

    def write(self, text):
        with open(os.path.join(self.dir.name, "CHANGELOG.md"), "w", encoding="utf-8") as f:
            f.write(text)

    def test_the_changelog_beats_a_stale_versions_json(self):
        self.write(CHANGELOG)
        got = build_site.with_own_version({"aurore": "1.0.0", "playguard": "1.3.0"})
        self.assertEqual(got, {"aurore": "1.1.0", "playguard": "1.3.0"})

    def test_offline_the_own_version_is_still_there(self):
        self.write(CHANGELOG)
        self.assertEqual(build_site.with_own_version({}), {"aurore": "1.1.0"})

    def test_before_the_first_release_the_menu_is_left_as_it_is(self):
        self.write("# Changelog\n")
        versions = {"playguard": "1.3.0"}
        self.assertEqual(build_site.with_own_version(versions), versions)
        self.assertIsNot(build_site.with_own_version(versions), versions)


if __name__ == "__main__":
    unittest.main()
