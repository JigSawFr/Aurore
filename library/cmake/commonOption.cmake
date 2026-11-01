# SPDX-FileCopyrightText: borealis contributors
# SPDX-License-Identifier: Apache-2.0

# Aurore's build options. Include this file before project(), then
# toolchain.cmake (README.md, "Use it in your project").
#
# The options are prefixed (AURORE_), so they do not clash with a parent
# project's own. This file then sets plain variables from them, which the
# build files read: an app can test PLATFORM_SWITCH or USE_DEKO3D after
# including it.

# disable in-source build
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed")
endif ()

# Once per directory scope
if (DEFINED _AURORE_OPTIONS_READ)
    return()
endif ()
set(_AURORE_OPTIONS_READ ON)

include(CMakeDependentOption)

# Target platform
set(AURORE_PLATFORM "" CACHE STRING "Platform to build for: DESKTOP (Linux, macOS, Windows), SWITCH, PSV, PS4, 3DS (New 3DS, citro3d), ANDROID, IOS or TVOS")
set_property(CACHE AURORE_PLATFORM PROPERTY STRINGS DESKTOP SWITCH PSV PS4 3DS ANDROID IOS TVOS)

# Window and input. SDL3 by default on desktop, in a browser, on Android,
# iOS and tvOS; SDL2 or GLFW (desktop) when asked for. The consoles have no
# SDL3 yet: GLFW (the default) or SDL2 on the Switch, SDL2 on the PS Vita and
# the PS4, their own elsewhere.
set(AURORE_WINDOW "" CACHE STRING "Window and input: GLFW, SDL2 or SDL3 (empty: the platform's default)")
set_property(CACHE AURORE_WINDOW PROPERTY STRINGS GLFW SDL2 SDL3)

# Graphics: OpenGL 3.2+ unless one of these. DEKO3D on Switch only, GXM on
# PS Vita only, SDLGPU (SDL3's GPU API: Direct3D 12, Metal, Vulkan; a
# prototype) on the desktop with SDL3.
set(AURORE_GRAPHICS "" CACHE STRING "Graphics: GL3, GL2, GLES2, GLES3, DEKO3D, GXM or SDLGPU (empty: the platform's default)")
set_property(CACHE AURORE_GRAPHICS PROPERTY STRINGS GL3 GL2 GLES2 GLES3 DEKO3D GXM SDLGPU)

option(AURORE_EGL "Use EGL instead of the native GL context" OFF)

# Windows only
option(AURORE_WIN32_TERMINAL "Windows: show a terminal along with the window" ON)

# PS4 only
option(AURORE_LIBJBC "PS4: root access (libjbc)" OFF)

# PS Vita with GXM only
option(AURORE_VITA_SHARK "PS Vita GXM: compile the shaders at run time (vitashark)" OFF)

# Switch with deko3d only: deko3d's debug library, for a console that aborts
# with 2359-xxxx. It checks every call and writes the reason (a GPU page fault
# and its address, a bad call) to stderr before showing it on the screen.
option(AURORE_DEKO3D_DEBUG "Switch deko3d: link libdeko3dd, deko3d's checks and messages on stderr" OFF)

option(AURORE_BOOST_FILESYSTEM "Use boost::filesystem instead of std::filesystem" OFF)

option(AURORE_STD_THREAD "Use std::thread instead of pthread" OFF)

# Built in on platforms where files are not convenient to load (Android and
# iOS, always). Elsewhere, an app may build them in so it travels as one
# file, or so its resources are not changed.
option(AURORE_LIBROMFS "Build the resources into the binary (libromfs)" OFF)

# Disable the highlight border animation (useful for low-end devices like PS Vita)
option(AURORE_SIMPLE_HIGHLIGHT "Simple highlight" OFF)

# Rasterize glyphs with FreeType instead of stb_truetype: hinted text and
# colour emoji (CBDT/CBLC, sbix and COLR fonts). The system FreeType on
# desktop, devkitPro's switch-freetype on Switch.
option(AURORE_FREETYPE "fontstash renders glyphs with FreeType" OFF)

# Labels put right-to-left text (Hebrew, Arabic) in reading order and join
# Arabic letters, with GNU FriBidi (LGPL-2.1+): aurore::bidiVisual().
option(AURORE_FRIBIDI "labels reorder right-to-left text with FriBidi" OFF)

# Unity build, CMAKE_UNITY_BUILD_BATCH_SIZE sets the batch size
# https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html
option(AURORE_UNITY_BUILD "Unity build" OFF)

option(AURORE_FONTSTASH_STREAM "fontstash use stream" OFF)

# The built-in inspector (F12, or L3 + R3) on from the start, for debug builds
# (Application::setInspectorEnabled does it at run time)
option(AURORE_INSPECTOR "Built-in inspector on" OFF)

# The debugging aids read from the environment on a desktop: AURORE_INSPECTOR,
# AURORE_HOT_RELOAD, AURORE_REDRAW_DEBUG, AURORE_PROFILE and Cadran's flags
# (ui::env_flag). AUTO builds them in except in Release and MinSizeRel: a
# distributed app does not read its screens from a folder an environment
# variable names. Their functions (Inspector::setEnabled,
# HotReload::setEnabled...) are in every build.
set(AURORE_DEV_TOOLS "AUTO" CACHE STRING "Debugging aids from the environment: AUTO (not in Release and MinSizeRel), ON or OFF")
set_property(CACHE AURORE_DEV_TOOLS PROPERTY STRINGS AUTO ON OFF)

# The test harness (headless platform, aurore::test::Harness: target
# Aurore::test), and with this project the demo's tests, bench and
# aurore-preview. Desktop with OpenGL 3 (the default driver) only.
option(AURORE_BUILD_TESTS "Build the test harness" OFF)

# iOS only (if empty then not signed)
set(IOS_CODE_SIGN_IDENTITY "" CACHE STRING "The code sign identity to use when building the IPA.")
set(IOS_GUI_IDENTIFIER "" CACHE STRING "Package name.")

string(TOUPPER "${AURORE_PLATFORM}" AURORE_PLATFORM)
set_property(CACHE AURORE_PLATFORM PROPERTY VALUE "${AURORE_PLATFORM}")
if (AURORE_PLATFORM AND NOT AURORE_PLATFORM MATCHES "^(DESKTOP|SWITCH|PSV|PS4|3DS|ANDROID|IOS|TVOS)$")
    message(FATAL_ERROR "AURORE_PLATFORM=${AURORE_PLATFORM}: DESKTOP, SWITCH, PSV, PS4, 3DS, ANDROID, IOS or TVOS")
endif ()

string(TOUPPER "${AURORE_WINDOW}" AURORE_WINDOW)
set_property(CACHE AURORE_WINDOW PROPERTY VALUE "${AURORE_WINDOW}")
if (AURORE_WINDOW AND NOT AURORE_WINDOW MATCHES "^(GLFW|SDL2|SDL3)$")
    message(FATAL_ERROR "AURORE_WINDOW=${AURORE_WINDOW}: GLFW, SDL2 or SDL3")
endif ()
# The 3DS has its own screens and input (libctru) and a single renderer,
# citro3d (docs/platforms.md)
if (AURORE_PLATFORM STREQUAL "3DS" AND (AURORE_WINDOW OR AURORE_GRAPHICS))
    message(FATAL_ERROR "AURORE_PLATFORM=3DS takes no AURORE_WINDOW nor AURORE_GRAPHICS: its window is libctru's, and its renderer citro3d")
endif ()
if (AURORE_WINDOW STREQUAL "SDL3" AND AURORE_PLATFORM MATCHES "^(SWITCH|PSV|PS4|3DS)$")
    message(FATAL_ERROR "AURORE_WINDOW=SDL3 is for DESKTOP, ANDROID, IOS and TVOS: ${AURORE_PLATFORM} has no SDL3 yet")
endif ()

string(TOUPPER "${AURORE_GRAPHICS}" AURORE_GRAPHICS)
set_property(CACHE AURORE_GRAPHICS PROPERTY VALUE "${AURORE_GRAPHICS}")
if (AURORE_GRAPHICS STREQUAL "D3D11")
    message(FATAL_ERROR "AURORE_GRAPHICS=D3D11 is gone: AURORE_GRAPHICS=SDLGPU draws with Direct3D 12 on Windows")
elseif (AURORE_GRAPHICS AND NOT AURORE_GRAPHICS MATCHES "^(GL3|GL2|GLES2|GLES3|DEKO3D|GXM|SDLGPU)$")
    message(FATAL_ERROR "AURORE_GRAPHICS=${AURORE_GRAPHICS}: GL3, GL2, GLES2, GLES3, DEKO3D, GXM or SDLGPU")
endif ()
if (AURORE_GRAPHICS STREQUAL "DEKO3D" AND NOT AURORE_PLATFORM STREQUAL "SWITCH")
    message(FATAL_ERROR "AURORE_GRAPHICS=DEKO3D is for AURORE_PLATFORM=SWITCH")
elseif (AURORE_GRAPHICS STREQUAL "GXM" AND NOT AURORE_PLATFORM STREQUAL "PSV")
    message(FATAL_ERROR "AURORE_GRAPHICS=GXM is for AURORE_PLATFORM=PSV")
elseif (AURORE_GRAPHICS STREQUAL "SDLGPU" AND (NOT AURORE_PLATFORM STREQUAL "DESKTOP" OR AURORE_WINDOW MATCHES "^(GLFW|SDL2)$"))
    message(FATAL_ERROR "AURORE_GRAPHICS=SDLGPU is for AURORE_PLATFORM=DESKTOP with AURORE_WINDOW=SDL3 (the default)")
endif ()

# The plain variables the build files read. toolchain.cmake changes some of
# them for the platforms that impose a choice (iOS and Android: SDL, and
# libromfs).
foreach (_aurore_p DESKTOP IOS TVOS ANDROID PSV PS4 SWITCH 3DS)
    if (AURORE_PLATFORM STREQUAL _aurore_p)
        set(PLATFORM_${_aurore_p} ON)
    else ()
        set(PLATFORM_${_aurore_p} OFF)
    endif ()
endforeach ()
foreach (_aurore_p GLFW SDL2 SDL3)
    if (AURORE_WINDOW STREQUAL _aurore_p)
        set(USE_${_aurore_p} ON)
    else ()
        set(USE_${_aurore_p} OFF)
    endif ()
endforeach ()
foreach (_aurore_p DEKO3D GXM SDLGPU GL2 GLES2 GLES3)
    if (AURORE_GRAPHICS STREQUAL _aurore_p)
        set(USE_${_aurore_p} ON)
    else ()
        set(USE_${_aurore_p} OFF)
    endif ()
endforeach ()
set(USE_EGL ${AURORE_EGL})
if (WIN32 AND AURORE_WIN32_TERMINAL)
    set(WIN32_TERMINAL ON)
else ()
    set(WIN32_TERMINAL OFF)
endif ()
if (PLATFORM_PS4 AND AURORE_LIBJBC)
    set(LIBJBC ON)
else ()
    set(LIBJBC OFF)
endif ()
if (USE_GXM AND AURORE_VITA_SHARK)
    set(USE_VITA_SHARK ON)
else ()
    set(USE_VITA_SHARK OFF)
endif ()
set(USE_BOOST_FILESYSTEM ${AURORE_BOOST_FILESYSTEM})
set(USE_STD_THREAD ${AURORE_STD_THREAD})
set(USE_LIBROMFS ${AURORE_LIBROMFS})
set(SIMPLE_HIGHLIGHT ${AURORE_SIMPLE_HIGHLIGHT})
set(USE_FREETYPE ${AURORE_FREETYPE})
set(USE_FRIBIDI ${AURORE_FRIBIDI})
unset(_aurore_p)
