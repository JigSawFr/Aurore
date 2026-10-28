# Aurore's build options. Include this file before project(), then
# toolchain.cmake (README.md, "Use it in your project").
#
# The options are prefixed (AURORE_, and BRLS_ for the older ones), so they
# do not clash with a parent project's own. The names borealis used
# (PLATFORM_DESKTOP, USE_GLFW, USE_LIBROMFS…) still work for now: given on
# the command line or set before this file, they are copied to the new
# options, with a deprecation warning. Either way this file then sets the
# old names as plain variables, which the build files read: an app can
# still test PLATFORM_SWITCH or USE_DEKO3D after including it.

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
set(AURORE_PLATFORM "" CACHE STRING "Platform to build for: DESKTOP (Linux, macOS, Windows), SWITCH, PSV, PS4, ANDROID, IOS or TVOS")
set_property(CACHE AURORE_PLATFORM PROPERTY STRINGS DESKTOP SWITCH PSV PS4 ANDROID IOS TVOS)

# Window and input, on desktop and Switch: GLFW (the default) or SDL2. The
# other platforms choose for themselves.
set(AURORE_WINDOW "" CACHE STRING "Window and input: GLFW or SDL2 (empty: the platform's default)")
set_property(CACHE AURORE_WINDOW PROPERTY STRINGS GLFW SDL2)

# Graphics: OpenGL 3.2+ unless one of these. DEKO3D on Switch only, D3D11 on
# Windows only, GXM on PS Vita only.
set(AURORE_GRAPHICS "" CACHE STRING "Graphics: GL3, GL2, GLES2, GLES3, DEKO3D, D3D11 or GXM (empty: the platform's default)")
set_property(CACHE AURORE_GRAPHICS PROPERTY STRINGS GL3 GL2 GLES2 GLES3 DEKO3D D3D11 GXM)

option(AURORE_EGL "Use EGL instead of the native GL context" OFF)

# Windows only
option(AURORE_WIN32_TERMINAL "Windows: show a terminal along with the window" ON)

# PS4 only
option(AURORE_LIBJBC "PS4: root access (libjbc)" OFF)

# PS Vita with GXM only
option(AURORE_VITA_SHARK "PS Vita GXM: compile the shaders at run time (vitashark)" OFF)

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
# Arabic letters, with GNU FriBidi (LGPL-2.1+): brls::bidiVisual().
option(AURORE_FRIBIDI "labels reorder right-to-left text with FriBidi" OFF)

# Unity build, CMAKE_UNITY_BUILD_BATCH_SIZE sets the batch size
# https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html
option(BRLS_UNITY_BUILD "Unity build" OFF)

option(BRLS_FONTSTASH_STREAM "fontstash use stream" OFF)

# The built-in inspector (F12, or L3 + R3) on from the start, for debug builds
# (Application::setInspectorEnabled does it at run time)
option(BRLS_INSPECTOR "Built-in inspector on" OFF)

# The debugging aids read from the environment on a desktop: BRLS_INSPECTOR,
# BRLS_HOT_RELOAD, BRLS_REDRAW_DEBUG, BRLS_PROFILE and Cadran's flags
# (ui::env_flag). AUTO builds them in except in Release and MinSizeRel: a
# distributed app does not read its screens from a folder an environment
# variable names. Their functions (Inspector::setEnabled,
# HotReload::setEnabled...) are in every build.
set(BRLS_DEV_TOOLS "AUTO" CACHE STRING "Debugging aids from the environment: AUTO (not in Release and MinSizeRel), ON or OFF")
set_property(CACHE BRLS_DEV_TOOLS PROPERTY STRINGS AUTO ON OFF)

# The test harness (headless platform, brls::test::Harness: target
# Aurore::test), and with this project the demo's tests, bench and
# aurore-preview. Desktop with OpenGL 3 (the default driver) only.
option(BRLS_BUILD_TESTS "Build the test harness" OFF)

# iOS only (if empty then not signed)
set(IOS_CODE_SIGN_IDENTITY "" CACHE STRING "The code sign identity to use when building the IPA.")
set(IOS_GUI_IDENTIFIER "" CACHE STRING "Package name.")

# --- The old names -----------------------------------------------------------

# Copies what the old names say (`value`, empty when none is set) to
# `option`, when it is new or changed since the last run: an option given
# later under its new name is not overwritten by an old name left in the
# cache. RESET: an empty value puts the option back to its default.
function(_aurore_legacy option value names)
    cmake_parse_arguments(ARG "RESET" "" "" ${ARGN})
    if (NOT "${value}" STREQUAL "${_AURORE_LEGACY_${option}}")
        if (NOT "${value}" STREQUAL "" OR ARG_RESET)
            set_property(CACHE ${option} PROPERTY VALUE "${value}")
        endif ()
        set(_AURORE_LEGACY_${option} "${value}" CACHE INTERNAL "")
    endif ()
    if (NOT "${value}" STREQUAL "" AND NOT "${value}" STREQUAL "OFF")
        message(DEPRECATION "${names} is deprecated: use -D${option}=${value} (and -U${names} to forget the old name)")
    endif ()
endfunction()

# A boolean option and its old name
function(_aurore_legacy_bool option old)
    set(value "")
    if (DEFINED ${old})
        if (${old})
            set(value ON)
        else ()
            set(value OFF)
        endif ()
    endif ()
    _aurore_legacy(${option} "${value}" ${old})
endfunction()

set(_aurore_value "")
set(_aurore_names "")
foreach (_aurore_p DESKTOP IOS TVOS ANDROID PSV PS4 SWITCH)
    if (PLATFORM_${_aurore_p})
        set(_aurore_value ${_aurore_p})
        set(_aurore_names PLATFORM_${_aurore_p})
        break()
    endif ()
endforeach ()
_aurore_legacy(AURORE_PLATFORM "${_aurore_value}" "${_aurore_names}" RESET)
string(TOUPPER "${AURORE_PLATFORM}" AURORE_PLATFORM)
set_property(CACHE AURORE_PLATFORM PROPERTY VALUE "${AURORE_PLATFORM}")
if (AURORE_PLATFORM AND NOT AURORE_PLATFORM MATCHES "^(DESKTOP|SWITCH|PSV|PS4|ANDROID|IOS|TVOS)$")
    message(FATAL_ERROR "AURORE_PLATFORM=${AURORE_PLATFORM}: DESKTOP, SWITCH, PSV, PS4, ANDROID, IOS or TVOS")
endif ()

set(_aurore_value "")
set(_aurore_names "")
if (USE_SDL2)
    set(_aurore_value SDL2)
    set(_aurore_names USE_SDL2)
elseif (USE_GLFW)
    set(_aurore_value GLFW)
    set(_aurore_names USE_GLFW)
endif ()
_aurore_legacy(AURORE_WINDOW "${_aurore_value}" "${_aurore_names}" RESET)
string(TOUPPER "${AURORE_WINDOW}" AURORE_WINDOW)
set_property(CACHE AURORE_WINDOW PROPERTY VALUE "${AURORE_WINDOW}")
if (AURORE_WINDOW AND NOT AURORE_WINDOW MATCHES "^(GLFW|SDL2)$")
    message(FATAL_ERROR "AURORE_WINDOW=${AURORE_WINDOW}: GLFW or SDL2")
endif ()

# The drivers of one platform count on that platform only, as before (an app
# may declare USE_DEKO3D=ON for its Switch build and build for desktop too)
set(_aurore_value "")
set(_aurore_names "")
if (USE_DEKO3D AND AURORE_PLATFORM STREQUAL "SWITCH")
    set(_aurore_value DEKO3D)
    set(_aurore_names USE_DEKO3D)
elseif (USE_D3D11 AND WIN32)
    set(_aurore_value D3D11)
    set(_aurore_names USE_D3D11)
elseif (USE_GXM AND AURORE_PLATFORM STREQUAL "PSV")
    set(_aurore_value GXM)
    set(_aurore_names USE_GXM)
elseif (USE_GL2)
    set(_aurore_value GL2)
    set(_aurore_names USE_GL2)
elseif (USE_GLES2)
    set(_aurore_value GLES2)
    set(_aurore_names USE_GLES2)
elseif (USE_GLES3)
    set(_aurore_value GLES3)
    set(_aurore_names USE_GLES3)
endif ()
_aurore_legacy(AURORE_GRAPHICS "${_aurore_value}" "${_aurore_names}" RESET)
string(TOUPPER "${AURORE_GRAPHICS}" AURORE_GRAPHICS)
set_property(CACHE AURORE_GRAPHICS PROPERTY VALUE "${AURORE_GRAPHICS}")
if (AURORE_GRAPHICS AND NOT AURORE_GRAPHICS MATCHES "^(GL3|GL2|GLES2|GLES3|DEKO3D|D3D11|GXM)$")
    message(FATAL_ERROR "AURORE_GRAPHICS=${AURORE_GRAPHICS}: GL3, GL2, GLES2, GLES3, DEKO3D, D3D11 or GXM")
endif ()
if (AURORE_GRAPHICS STREQUAL "DEKO3D" AND NOT AURORE_PLATFORM STREQUAL "SWITCH")
    message(FATAL_ERROR "AURORE_GRAPHICS=DEKO3D is for AURORE_PLATFORM=SWITCH")
elseif (AURORE_GRAPHICS STREQUAL "GXM" AND NOT AURORE_PLATFORM STREQUAL "PSV")
    message(FATAL_ERROR "AURORE_GRAPHICS=GXM is for AURORE_PLATFORM=PSV")
endif ()

_aurore_legacy_bool(AURORE_EGL USE_EGL)
_aurore_legacy_bool(AURORE_WIN32_TERMINAL WIN32_TERMINAL)
_aurore_legacy_bool(AURORE_LIBJBC LIBJBC)
_aurore_legacy_bool(AURORE_VITA_SHARK USE_VITA_SHARK)
_aurore_legacy_bool(AURORE_BOOST_FILESYSTEM USE_BOOST_FILESYSTEM)
_aurore_legacy_bool(AURORE_STD_THREAD USE_STD_THREAD)
_aurore_legacy_bool(AURORE_LIBROMFS USE_LIBROMFS)
_aurore_legacy_bool(AURORE_SIMPLE_HIGHLIGHT SIMPLE_HIGHLIGHT)
_aurore_legacy_bool(AURORE_FREETYPE USE_FREETYPE)
_aurore_legacy_bool(AURORE_FRIBIDI USE_FRIBIDI)

# The plain variables the build files read. toolchain.cmake changes some of
# them for the platforms that impose a choice (iOS and Android: SDL2 and
# libromfs).
foreach (_aurore_p DESKTOP IOS TVOS ANDROID PSV PS4 SWITCH)
    if (AURORE_PLATFORM STREQUAL _aurore_p)
        set(PLATFORM_${_aurore_p} ON)
    else ()
        set(PLATFORM_${_aurore_p} OFF)
    endif ()
endforeach ()
foreach (_aurore_p GLFW SDL2)
    if (AURORE_WINDOW STREQUAL _aurore_p)
        set(USE_${_aurore_p} ON)
    else ()
        set(USE_${_aurore_p} OFF)
    endif ()
endforeach ()
foreach (_aurore_p DEKO3D D3D11 GXM GL2 GLES2 GLES3)
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
unset(_aurore_value)
unset(_aurore_names)
