# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

# The New 3DS: a test card drawn by citro3d, built by the root
# CMakeLists.txt with -DAURORE_PLATFORM=3DS instead of the demo
# (docs/platforms.md). The library's resources (fonts, pictures) go in its
# romfs.

add_executable(aurore_3ds_smoke smoke.cpp)
set_target_properties(aurore_3ds_smoke PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_SCAN_FOR_MODULES OFF)
target_link_libraries(aurore_3ds_smoke PRIVATE aurore)

ctr_generate_smdh(aurore_3ds_smoke.smdh
        NAME "Aurore test card"
        DESCRIPTION "Aurore on the New 3DS, drawn by citro3d"
        AUTHOR "JigSawFr")
ctr_create_3dsx(aurore_3ds_smoke
        SMDH aurore_3ds_smoke.smdh
        ROMFS ${PROJECT_RESOURCES})
