// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Aurore's version, kept up to date by release-please (CHANGELOG.md says what
// changed). Semantic versioning: before 1.0, a minor version may break the API.
#define AURORE_VERSION_MAJOR 0 // x-release-please-major
#define AURORE_VERSION_MINOR 1 // x-release-please-minor
#define AURORE_VERSION_PATCH 0 // x-release-please-patch
#define AURORE_VERSION "0.1.0" // x-release-please-version

// For a test on the version: #if AURORE_VERSION_AT_LEAST(0, 2, 0)
#define AURORE_VERSION_AT_LEAST(major, minor, patch)                                                       \
    (AURORE_VERSION_MAJOR > (major) || (AURORE_VERSION_MAJOR == (major) && AURORE_VERSION_MINOR > (minor)) \
        || (AURORE_VERSION_MAJOR == (major) && AURORE_VERSION_MINOR == (minor) && AURORE_VERSION_PATCH >= (patch)))
