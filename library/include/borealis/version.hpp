/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

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
