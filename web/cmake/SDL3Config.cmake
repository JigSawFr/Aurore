# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

# The demo in a browser with SDL3 (web/README.md): SDL3 is Emscripten's port
# (-sUSE_SDL=3, set by the CMakeLists.txt), so the target links nothing.
if (NOT TARGET SDL3::SDL3)
    add_library(SDL3::SDL3 INTERFACE IMPORTED)
endif ()
set(SDL3_FOUND TRUE)
