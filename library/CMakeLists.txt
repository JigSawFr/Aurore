# SPDX-FileCopyrightText: borealis contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20...3.31)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(GNUInstallDirs)
set(AURORE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(AURORE_SOURCE)
set(AURORE_SRC)
# The usage requirements of the aurore target, filled in below. Headers of
# other projects are system headers: their warnings are not the library's.
set(AURORE_INCLUDE)
set(AURORE_PLATFORM_LIBS)
set(AURORE_PRIVATE_LIBS)
set(AURORE_PUBLIC_DEFINITIONS ${AURORE_TOOLCHAIN_DEFINITIONS})
set(AURORE_PRIVATE_DEFINITIONS)
set(AURORE_PUBLIC_OPTIONS)
# The vendored projects install their packages along with the library
set(AURORE_EXTERN_EXCLUDE EXCLUDE_FROM_ALL)
if (AURORE_INSTALL)
    set(AURORE_EXTERN_EXCLUDE)
endif ()
# Desktop: the directory that holds "resources/" at run time (default: the
# working directory). An app sets it before add_subdirectory(); it can be
# given with -DAURORE_RESOURCES_ROOT=... too (for an AppImage, say).
if (NOT AURORE_RESOURCES_ROOT)
    set(AURORE_RESOURCES_ROOT ".")
endif ()

# The version of library/include/aurore/version.hpp, which release-please updates
file(STRINGS ${AURORE_PATH}/include/aurore/version.hpp AURORE_VERSION REGEX "^#define AURORE_VERSION \"")
string(REGEX REPLACE ".*\"([0-9.]+)\".*" "\\1" AURORE_VERSION "${AURORE_VERSION}")
set(AURORE_VERSION ${AURORE_VERSION} PARENT_SCOPE)

option(AURORE_INSTALL "Install the library, its headers and its CMake package (AuroreConfig.cmake); desktop only" OFF)

# tinyxml2, GLFW and SDL (2 or 3) from the system when find_package()
# finds them, downloaded at configure time otherwise, instead of the copies in
# lib/extern (for packagers). nanovg, yoga and the others stay vendored.
option(AURORE_FETCH_DEPS "Use tinyxml2, GLFW and SDL from the system or downloaded (CMake 3.24+)" OFF)
if (AURORE_FETCH_DEPS)
    if (CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "AURORE_FETCH_DEPS needs CMake 3.24 or later (FIND_PACKAGE_ARGS)")
    endif ()
    include(FetchContent)
    # The versions of the vendored copies: tools/check_extern_versions.py
    # (CI) checks they match.
    FetchContent_Declare(tinyxml2
            GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
            GIT_TAG 11.0.0
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS)
    # The vendored GLFW is GLFW 3.5.1 with xfangfang's fixes (IME, touch, soft
    # fullscreen, dark title bar), carried in the aurore branch of JigSawFr/glfw
    FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/JigSawFr/glfw.git
            GIT_TAG fd162259625651250feb3f382130bd0af4a6337e
            FIND_PACKAGE_ARGS NAMES glfw3)
    FetchContent_Declare(SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.32.10
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS)
    FetchContent_Declare(SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.18
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS)
endif ()

# tweeny 4: its easings, in animation.cpp only (header-only, not in the
# library's headers, so not a dependency of an app or of the installed
# package). Its own CMakeLists.txt asks for CMake 3.23: its headers here.
if (USE_SYSTEM_TWEENY)
  find_package(Tweeny 4 REQUIRED)
else()
  add_library(tweeny INTERFACE)
  target_include_directories(tweeny SYSTEM INTERFACE ${AURORE_PATH}/lib/extern/tweeny/include)
  add_library(tweeny::tweeny ALIAS tweeny)
endif()

# yoga 3.2.1 (facebook/yoga tag v3.2.1, 042f501, MIT), library sources only
add_subdirectory(${AURORE_PATH}/lib/extern/yoga/yoga EXCLUDE_FROM_ALL)
# Its headers' installed place, relative to the prefix (yoga's own is absolute)
set_property(TARGET yogacore PROPERTY INTERFACE_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${AURORE_PATH}/lib/extern/yoga>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/yoga>)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(yogacore PRIVATE -fvisibility=default -Wno-error)
    # C++20 library parts Yoga 3 needs that some toolchains' libraries lack
    target_compile_options(yogacore PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/yoga_std_compat.h)
endif ()

# opencc
if (AURORE_USE_OPENCC)
    list(APPEND AURORE_PLATFORM_LIBS libopencc)
    list(APPEND AURORE_INCLUDE ${AURORE_OPENCC_INCLUDE})
    list(APPEND AURORE_PUBLIC_DEFINITIONS OPENCC)
endif ()

# boost::filesystem
if (USE_BOOST_FILESYSTEM)
    find_package(Boost 1.44 REQUIRED COMPONENTS filesystem)
    if(Boost_FOUND)
        list(APPEND AURORE_PRIVATE_LIBS Boost::filesystem)
        list(APPEND AURORE_PRIVATE_DEFINITIONS USE_BOOST_FILESYSTEM)
    endif()
elseif ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1) OR
    (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT WIN32 AND NOT PLATFORM_ANDROID AND NOT PLATFORM_PS4))
    list(APPEND AURORE_PLATFORM_LIBS stdc++fs)
endif ()

# libromfs
if(USE_LIBROMFS)
    add_subdirectory(${AURORE_PATH}/lib/extern/libromfs EXCLUDE_FROM_ALL)
    list(APPEND AURORE_PLATFORM_LIBS ${LIBROMFS_LIBRARY})
    list(APPEND AURORE_PUBLIC_DEFINITIONS USE_LIBROMFS)
endif()

# tinyxml2
if (USE_SYSTEM_TINYXML2)
    find_package(tinyxml2 REQUIRED COMPONENTS shared)

    list(APPEND AURORE_INCLUDE ${tinyxml2_INCLUDE_DIRS})
    list(APPEND AURORE_PLATFORM_LIBS tinyxml2::tinyxml2)
elseif (AURORE_FETCH_DEPS)
    set(tinyxml2_BUILD_TESTING OFF)
    FetchContent_MakeAvailable(tinyxml2)
    list(APPEND AURORE_PLATFORM_LIBS tinyxml2::tinyxml2)
else()
    add_library(tinyxml2 STATIC ${AURORE_PATH}/lib/extern/tinyxml2/tinyxml2.cpp)
    target_include_directories(tinyxml2 SYSTEM PUBLIC
            $<BUILD_INTERFACE:${AURORE_PATH}/include/aurore/extern/tinyxml2>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/aurore/extern/tinyxml2>)
    list(APPEND AURORE_PLATFORM_LIBS tinyxml2)
endif()

# dbus (Linux with GLFW: keeps the screen saver off). An imported target,
# so the installed package names no path of this machine.
if (PLATFORM_DESKTOP AND UNIX AND NOT APPLE AND NOT USE_SDL2 AND NOT USE_SDL3)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
    list(APPEND AURORE_PLATFORM_LIBS PkgConfig::DBUS)
endif ()

if (PLATFORM_DESKTOP)
    find_package(Threads REQUIRED)
    list(APPEND AURORE_PLATFORM_LIBS Threads::Threads)

    if (USE_SDL3)
        if (USE_SYSTEM_SDL3)
            find_package(SDL3 REQUIRED CONFIG)
            list(APPEND AURORE_PLATFORM_LIBS SDL3::SDL3)
        elseif (AURORE_FETCH_DEPS)
            set(BUILD_SHARED_LIBS OFF)
            set(SDL_TEST_LIBRARY OFF)
            FetchContent_MakeAvailable(SDL3)
            list(APPEND AURORE_PLATFORM_LIBS SDL3::SDL3)
        else ()
            set(BUILD_SHARED_LIBS OFF)
            set(SDL_TEST_LIBRARY OFF)
            if (AURORE_INSTALL)
                set(SDL_INSTALL ON CACHE BOOL "" FORCE)
            endif ()
            add_subdirectory(${AURORE_PATH}/lib/extern/SDL3 ${AURORE_EXTERN_EXCLUDE})
            list(APPEND AURORE_PLATFORM_LIBS SDL3::SDL3-static)
        endif ()
        list(APPEND AURORE_SOURCE ${AURORE_PATH}/lib/platforms/sdl)
    elseif (USE_SDL2)
        if (USE_SYSTEM_SDL2)
            find_package(SDL2 REQUIRED)
            message(STATUS "Find SDL2: ${SDL2_INCLUDE_DIR}, ${SDL2_LIBRARIES}")
            list(APPEND AURORE_PLATFORM_LIBS ${SDL2_LIBRARIES})
            list(APPEND AURORE_INCLUDE ${SDL2_INCLUDE_DIRS})
        elseif (AURORE_FETCH_DEPS)
            set(BUILD_SHARED_LIBS OFF)
            set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
            FetchContent_MakeAvailable(SDL2)
            if (sdl2_SOURCE_DIR)
                list(APPEND AURORE_PLATFORM_LIBS SDL2::SDL2-static SDL2::SDL2main)
            else ()
                list(APPEND AURORE_PLATFORM_LIBS SDL2::SDL2 SDL2::SDL2main)
            endif ()
        else ()
            set(BUILD_SHARED_LIBS OFF)
            set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
            if (AURORE_INSTALL)
                set(SDL2_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
            endif ()
            add_subdirectory(${AURORE_PATH}/lib/extern/SDL ${AURORE_EXTERN_EXCLUDE})
            list(APPEND AURORE_PLATFORM_LIBS SDL2::SDL2-static SDL2::SDL2main)
        endif ()
        list(APPEND AURORE_SOURCE ${AURORE_PATH}/lib/platforms/sdl)
    else ()
        if (USE_SYSTEM_GLFW)
            find_package(glfw3 REQUIRED)
        elseif (AURORE_FETCH_DEPS)
            set(GLFW_BUILD_DOCS OFF)
            set(GLFW_BUILD_EXAMPLES OFF)
            set(GLFW_BUILD_TESTS OFF)
            FetchContent_MakeAvailable(glfw)
        else ()
            set(GLFW_BUILD_DOCS OFF)
            add_subdirectory(${AURORE_PATH}/lib/extern/glfw ${AURORE_EXTERN_EXCLUDE})
        endif ()
        list(APPEND AURORE_PLATFORM_LIBS glfw)
        list(APPEND AURORE_PUBLIC_DEFINITIONS __GLFW__)
        list(APPEND AURORE_SOURCE ${AURORE_PATH}/lib/platforms/glfw)
    endif ()
    if (AURORE_USE_SDLGPU)
        list(APPEND AURORE_SRC ${AURORE_PATH}/lib/platforms/driver/sdlgpu.cpp)
    endif ()
    if (WIN32)
        if (MINGW)
            list(APPEND AURORE_PUBLIC_DEFINITIONS WINVER=0x0605)
        endif ()
        list(APPEND AURORE_PLATFORM_LIBS wlanapi iphlpapi ws2_32)
    elseif (APPLE)
        list(APPEND AURORE_PLATFORM_LIBS "-framework CoreWLAN" "-framework CoreGraphics")
        list(APPEND AURORE_PLATFORM_LIBS "-framework SystemConfiguration" "-framework Cocoa")
        list(APPEND AURORE_PLATFORM_LIBS "-framework IOKit" "-framework CoreFoundation")
        list(APPEND AURORE_PLATFORM_LIBS "-framework AppKit" "-framework Foundation" "-framework CoreServices" "-lobjc")
        # The system's fonts for Chinese, Japanese and Korean (desktop_font.cpp)
        list(APPEND AURORE_PLATFORM_LIBS "-framework CoreText")
        list(APPEND AURORE_SRC ${AURORE_PATH}/lib/platforms/desktop/desktop_darwin.mm)
    elseif (UNIX)
        # fontconfig, opened at run time when there (desktop_font.cpp)
        list(APPEND AURORE_PLATFORM_LIBS ${CMAKE_DL_LIBS})
    endif ()
    list(APPEND AURORE_SOURCE ${AURORE_PATH}/lib/platforms/desktop)
    set(AURORE_PLATFORM_RESOURCES_PATH "\"${AURORE_RESOURCES_ROOT}/resources/\"")
elseif (PLATFORM_PSV)
    list(APPEND AURORE_SOURCE ${AURORE_PATH}/lib/platforms/desktop)
    list(APPEND AURORE_PLATFORM_LIBS
            SceGxm_stub
            SceDisplay_stub
            SceCtrl_stub
            SceCommonDialog_stub
            SceTouch_stub
            SceAppUtil_stub
            SceIme_stub
            ScePower_stub
            SceAVConfig_stub
            SceRegistryMgr_stub
            -Wl,--whole-archive -lpthread -Wl,--no-whole-archive)
    if (AURORE_USE_GXM)
        if (USE_VITA_SHARK)
                # libshacccg.suprx, which the app ships in its module folder
                include(FetchContent)
                FetchContent_Declare(
                        libshacccg
                        # A fixed commit, not the moving main branch
                        URL "https://codeload.github.com/AnimMouse/SceShaccCg/zip/f9511fc20ce905704f384b3a99a17c5e97f9a92c"
                        SOURCE_DIR "${CMAKE_BINARY_DIR}/vendor/SceShaccCg"
                        SUBBUILD_DIR "${CMAKE_BINARY_DIR}/CMakeFiles/SceShaccCg"
                        BINARY_DIR "${CMAKE_BINARY_DIR}/CMakeFiles/SceShaccCg"
                )
                # No CMakeLists.txt in it: downloaded only
                FetchContent_MakeAvailable(libshacccg)
                list(APPEND AURORE_PLATFORM_LIBS
                        vitashark
                        SceShaccCgExt
                        taihen_stub
                        SceShaccCg_stub
                )
        endif ()
        file(GLOB_RECURSE PSV_PLATFORM_SRC CONFIGURE_DEPENDS "${AURORE_PATH}/lib/platforms/psv/*.cpp")
        list(APPEND AURORE_SRC ${PSV_PLATFORM_SRC})
    else()
        if (USE_SYSTEM_SDL2)
            find_package(SDL2 REQUIRED)
        else ()
            set(BUILD_SHARED_LIBS OFF)
            set(VIDEO_VITA_PVR ${AURORE_USE_GXM} CACHE BOOL "")
            add_subdirectory(${AURORE_PATH}/lib/extern/SDL EXCLUDE_FROM_ALL)
        endif ()
        list(APPEND AURORE_PLATFORM_LIBS
                SDL2::SDL2-static
                libgpu_es4_ext_stub_weak
                libIMGEGL_stub_weak
                libGLESv2_stub_weak)
        # Using SDL implementation instead
        file(GLOB_RECURSE SDL_PLATFORM_SRC CONFIGURE_DEPENDS "${AURORE_PATH}/lib/platforms/sdl/*.cpp")
        list(APPEND AURORE_SRC ${SDL_PLATFORM_SRC})
        list(APPEND AURORE_SRC ${AURORE_PATH}/lib/platforms/psv/psv_platform.cpp)
    endif ()
    list(APPEND AURORE_PUBLIC_DEFINITIONS __PSV__ STBI_NEON)
    set(AURORE_PLATFORM_RESOURCES_PATH "\"app0:resources/\"")
    set_target_properties(yogacore PROPERTIES POSITION_INDEPENDENT_CODE OFF)
elseif(PLATFORM_PS4)
    find_package(SDL2 REQUIRED)
    list(APPEND AURORE_SOURCE
            ${AURORE_PATH}/lib/platforms/sdl
            ${AURORE_PATH}/lib/platforms/desktop
            ${AURORE_PATH}/lib/extern/nanovg/ps4)
    file(GLOB_RECURSE PS4_PLATFORM_SRC CONFIGURE_DEPENDS "${AURORE_PATH}/lib/platforms/ps4/*.cpp")
    list(APPEND AURORE_SRC ${PS4_PLATFORM_SRC})
    list(APPEND AURORE_INCLUDE /opt/OpenOrbis-PS4-Toolchain/include)
    list(APPEND AURORE_PLATFORM_LIBS SDL2::SDL2-static SceNetCtl SceImeDialog SceCommonDialog)
    list(APPEND AURORE_PUBLIC_DEFINITIONS __SDL2__ PS4 USE_GLES2)
    if (LIBJBC)
        list(APPEND AURORE_PLATFORM_LIBS jbc)
        list(APPEND AURORE_PUBLIC_DEFINITIONS USE_JBC)
        set(AURORE_PLATFORM_RESOURCES_PATH "\"/mnt/sandbox/${PSN_TITLE_ID}_000/app0/resources/\"")
    else ()
        set(AURORE_PLATFORM_RESOURCES_PATH "\"/app0/resources/\"")
    endif ()
elseif(PLATFORM_ANDROID)
    # The app's CMakeLists.txt adds SDL's project (android-project/app/jni):
    # SDL's Java activity loads the shared library it builds
    list(APPEND AURORE_SOURCE
            ${AURORE_PATH}/lib/platforms/sdl
            ${AURORE_PATH}/lib/platforms/desktop
            ${AURORE_PATH}/lib/platforms/android)
    if (USE_SDL3)
        list(APPEND AURORE_PLATFORM_LIBS SDL3::SDL3)
    else ()
        find_library(SDL2 NAMES SDL2 SDL2main)
        list(APPEND AURORE_PLATFORM_LIBS SDL2 SDL2main)
    endif ()
    list(APPEND AURORE_PUBLIC_DEFINITIONS ANDROID)
    # The NDK's libc++ returns std::string from path::u8string() in C++20,
    # which nlohmann json's path conversion does not build with
    list(APPEND AURORE_PUBLIC_DEFINITIONS JSON_HAS_FILESYSTEM=0 JSON_HAS_EXPERIMENTAL_FILESYSTEM=0)
elseif(PLATFORM_IOS OR PLATFORM_TVOS)
    set(BUILD_SHARED_LIBS OFF)
    if (USE_SDL3)
        set(SDL_TEST_LIBRARY OFF)
        add_subdirectory(${AURORE_PATH}/lib/extern/SDL3)
        list(APPEND AURORE_PLATFORM_LIBS SDL3::SDL3-static)
    else ()
        add_subdirectory(${AURORE_PATH}/lib/extern/SDL)
        find_library(SDL2 NAMES SDL2-static SDL2main)
        list(APPEND AURORE_PLATFORM_LIBS SDL2-static SDL2main)
    endif ()
    list(APPEND AURORE_SOURCE
            ${AURORE_PATH}/lib/platforms/sdl
            ${AURORE_PATH}/lib/platforms/desktop)
    list(APPEND AURORE_PLATFORM_LIBS "-framework CoreText" "-framework CoreFoundation")
    list(APPEND AURORE_SRC ${AURORE_PATH}/lib/platforms/desktop/ios_darwin.mm)

    if(PLATFORM_IOS)
        list(APPEND AURORE_PUBLIC_DEFINITIONS IOS)
    elseif(PLATFORM_TVOS)
        list(APPEND AURORE_PUBLIC_DEFINITIONS TVOS)
    endif()
elseif(PLATFORM_SWITCH)
    file(GLOB_RECURSE SWITCH_PLATFORM_SRC CONFIGURE_DEPENDS "${AURORE_PATH}/lib/platforms/switch/*.cpp")
    list(APPEND AURORE_SRC ${SWITCH_PLATFORM_SRC})
    list(APPEND AURORE_PUBLIC_OPTIONS $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>)
    list(APPEND AURORE_PUBLIC_DEFINITIONS SWITCH __SWITCH__ STBI_NO_THREAD_LOCALS)
    list(APPEND AURORE_PRIVATE_DEFINITIONS HAVE_LIBNX)
    find_package(PkgConfig REQUIRED)

    if (AURORE_USE_DEKO3D)
        list(APPEND AURORE_PLATFORM_LIBS deko3d nx m)
    elseif (USE_SDL2)
        find_package(SDL2 REQUIRED)
        list(REMOVE_ITEM AURORE_SRC ${AURORE_PATH}/lib/platforms/switch/switch_video.cpp)
        list(APPEND AURORE_SRC ${AURORE_PATH}/lib/platforms/sdl/sdl_video.cpp)
        list(APPEND AURORE_PLATFORM_LIBS ${SDL2_LIBRARIES} EGL glapi drm_nouveau nx m)
    else ()
        list(REMOVE_ITEM AURORE_SRC ${AURORE_PATH}/lib/platforms/switch/switch_video.cpp)
        list(APPEND AURORE_SRC ${AURORE_PATH}/lib/platforms/glfw/glfw_video.cpp)
        list(APPEND AURORE_PLATFORM_LIBS glfw3 EGL glapi drm_nouveau nx m)
    endif ()
    set(AURORE_PLATFORM_RESOURCES_PATH "\"romfs:/\"")
    # devkitPro's portlibs (GLFW, SDL2, EGL, curl…) last: it also has a
    # tinyxml2.h and a glad/gl.h, which must not hide the library's own
    list(APPEND AURORE_INCLUDE ${DEVKITPRO}/portlibs/switch/include)
elseif(PLATFORM_3DS)
    # The New 3DS through libctru (lib/platforms/ctr), drawn with citro3d by
    # nanovg's citro3d renderer (nanovg_citro3d.h)
    file(GLOB_RECURSE CTR_PLATFORM_SRC CONFIGURE_DEPENDS "${AURORE_PATH}/lib/platforms/ctr/*.cpp")
    list(APPEND AURORE_SRC ${CTR_PLATFORM_SRC})
    list(APPEND AURORE_PUBLIC_DEFINITIONS STBI_NO_THREAD_LOCALS)
    # The renderer's vertex shader, assembled by picasso and built into the
    # library (bin2s writes it as assembly): nanovg_citro3d_shbin.h
    enable_language(ASM)
    ctr_add_shader_library(nanovg_citro3d "${AURORE_PATH}/lib/extern/nanovg/citro3d/nanovg_citro3d.v.pica")
    dkp_add_embedded_binary_library(nanovg_citro3d_shaders nanovg_citro3d)
    list(APPEND AURORE_PLATFORM_LIBS nanovg_citro3d_shaders citro3d ctru m)
    set(AURORE_PLATFORM_RESOURCES_PATH "\"romfs:/\"")
else ()
    message(FATAL_ERROR "Please set the platform to build for. Example: -DAURORE_PLATFORM=DESKTOP or -DAURORE_PLATFORM=SWITCH")
endif ()

# aurore c/cpp source
list(APPEND AURORE_SOURCE
        ${AURORE_PATH}/lib/core
        ${AURORE_PATH}/lib/views
        ${AURORE_PATH}/lib/extern/glad
        ${AURORE_PATH}/lib/extern/libretro-common/compat
        ${AURORE_PATH}/lib/extern/libretro-common/encodings
        ${AURORE_PATH}/lib/extern/libretro-common/features
        ${AURORE_PATH}/lib/extern/qrcodegen
        # Cadran, Aurore's look (GPLv3, LICENSE.GPL)
        ${AURORE_PATH}/lib/cadran
        )
if (PLATFORM_PSV OR PLATFORM_PS4 OR PLATFORM_3DS OR AURORE_USE_SDLGPU)
        list(REMOVE_ITEM AURORE_SOURCE ${AURORE_PATH}/lib/extern/glad)
endif ()

# nanovg
list(APPEND AURORE_SRC ${AURORE_PATH}/lib/extern/nanovg/nanovg.c)
if (AURORE_FONTSTASH_STREAM)
    set_source_files_properties(
        ${AURORE_PATH}/lib/extern/nanovg/nanovg.c
        PROPERTIES
        COMPILE_DEFINITIONS "FONTSTASH_STREAM_IMPLEMENTATION"
    )
    message(STATUS "aurore: fontstash use stream")
endif ()
if (USE_FREETYPE)
    if (AURORE_FONTSTASH_STREAM)
        message(FATAL_ERROR "USE_FREETYPE cannot be combined with AURORE_FONTSTASH_STREAM")
    endif ()
    if (PLATFORM_SWITCH)
        # switch-freetype links bz2, png, zlib and harfbuzz: pkg-config knows them
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(AURORE_PC_FREETYPE REQUIRED IMPORTED_TARGET freetype2)
        list(APPEND AURORE_PLATFORM_LIBS PkgConfig::AURORE_PC_FREETYPE)
    else ()
        find_package(Freetype REQUIRED)
        list(APPEND AURORE_PLATFORM_LIBS Freetype::Freetype)
    endif ()
    set_property(SOURCE ${AURORE_PATH}/lib/extern/nanovg/nanovg.c APPEND PROPERTY COMPILE_DEFINITIONS FONS_USE_FREETYPE)
    list(APPEND AURORE_PUBLIC_DEFINITIONS AURORE_USE_FREETYPE)
    message(STATUS "aurore: fontstash uses FreeType")
endif ()
if (USE_FRIBIDI)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AURORE_PC_FRIBIDI REQUIRED IMPORTED_TARGET fribidi)
    list(APPEND AURORE_PLATFORM_LIBS PkgConfig::AURORE_PC_FRIBIDI)
    list(APPEND AURORE_PUBLIC_DEFINITIONS AURORE_USE_FRIBIDI)
    message(STATUS "aurore: labels reorder right-to-left text with FriBidi")
endif ()
if (PLATFORM_SWITCH AND AURORE_USE_DEKO3D)
    file(GLOB_RECURSE NANOVG_PLATFORM_SRC CONFIGURE_DEPENDS ${AURORE_PATH}/lib/extern/nanovg/*.cpp)
    list(APPEND AURORE_SRC ${NANOVG_PLATFORM_SRC})

    # nanovg's deko3d shaders, compiled and built into the library: an app
    # no longer copies .dksh files into its romfs (those it has still load
    # if these are missing). uam does not always give the same bytes for the
    # same GLSL (one machine-code byte of fill_vsh differs from one run to the
    # next), which made every build of an app unreproducible: each shader is
    # compiled once and kept in shaders/prebuilt/ with the hash of the GLSL it
    # came from, and uam runs only when the GLSL no longer matches it
    # (tools/dksh.py --update compiles them again).
    find_program(NX_UAM_EXE NAMES uam HINTS "${DEVKITPRO}/tools/bin")
    set(DKSH_GLSL "${AURORE_PATH}/lib/extern/nanovg/deko3d/shaders")
    set(DKSH_PREBUILT "${DKSH_GLSL}/prebuilt")
    set(DKSH_OUT "${CMAKE_CURRENT_BINARY_DIR}/dksh")
    file(MAKE_DIRECTORY "${DKSH_OUT}")
    set(DKSH_HEADER "/* Generated by library/CMakeLists.txt from nanovg's deko3d shaders. */\n#pragma once\n#include <cstddef>\n")
    foreach (shader fill_vsh:vert fill_aa_fsh:frag fill_fsh:frag)
        string(REPLACE ":" ";" parts ${shader})
        list(GET parts 0 name)
        list(GET parts 1 stage)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DKSH_GLSL}/${name}.glsl")
        set(dksh_hash "")
        set(glsl_hash "")
        if (EXISTS "${DKSH_PREBUILT}/${name}.dksh" AND EXISTS "${DKSH_PREBUILT}/${name}.glsl.sha256")
            file(SHA256 "${DKSH_GLSL}/${name}.glsl" glsl_hash)
            file(STRINGS "${DKSH_PREBUILT}/${name}.glsl.sha256" dksh_hash LIMIT_COUNT 1)
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DKSH_PREBUILT}/${name}.dksh" "${DKSH_PREBUILT}/${name}.glsl.sha256")
        endif ()
        if (dksh_hash STREQUAL glsl_hash AND NOT dksh_hash STREQUAL "")
            configure_file("${DKSH_PREBUILT}/${name}.dksh" "${DKSH_OUT}/${name}.dksh" COPYONLY)
            set(uam_result 0)
        else ()
            if (EXISTS "${DKSH_PREBUILT}/${name}.dksh")
                message(WARNING "deko3d shader ${name}: the prebuilt copy is not of this GLSL, compiled with uam instead (its bytes can differ from one build to the next); run tools/dksh.py --update")
            endif ()
            if (NX_UAM_EXE)
                execute_process(COMMAND ${NX_UAM_EXE} -s ${stage} -o "${DKSH_OUT}/${name}.dksh" "${DKSH_GLSL}/${name}.glsl"
                    RESULT_VARIABLE uam_result TIMEOUT 30)
            else ()
                set(uam_result 1)
            endif ()
        endif ()
        if (uam_result EQUAL 0 AND EXISTS "${DKSH_OUT}/${name}.dksh")
            file(READ "${DKSH_OUT}/${name}.dksh" bytes HEX)
            string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${bytes}")
            string(APPEND DKSH_HEADER "alignas(8) static const unsigned char dksh_${name}[] = {${bytes}};\n")
        else ()
            message(WARNING "deko3d shader ${name} not built in (uam missing or failed): the app's romfs must have it")
            string(APPEND DKSH_HEADER "static const unsigned char* const dksh_${name} = nullptr;\n")
        endif ()
    endforeach ()
    file(WRITE "${DKSH_OUT}/dk_shaders.h" "${DKSH_HEADER}")
    set(AURORE_DKSH_INCLUDE "${DKSH_OUT}")
endif ()

# Every .c and .cpp under these directories: CONFIGURE_DEPENDS has the build
# run CMake again when one is added or removed
foreach (lib ${AURORE_SOURCE})
    file(GLOB_RECURSE TEMP_SRC CONFIGURE_DEPENDS "${lib}/*.cpp" "${lib}/*.c")
    list(APPEND AURORE_SRC ${TEMP_SRC})
endforeach (lib)

# The graphics driver
set(AURORE_DRIVER AURORE_USE_OPENGL)
if (AURORE_USE_CITRO3D)
    set(AURORE_DRIVER AURORE_USE_CITRO3D)
elseif (AURORE_USE_METAL)
    set(AURORE_DRIVER AURORE_USE_METAL)
elseif (AURORE_USE_SDLGPU)
    set(AURORE_DRIVER AURORE_USE_SDLGPU)
elseif (AURORE_USE_DEKO3D)
    set(AURORE_DRIVER AURORE_USE_DEKO3D)
elseif (AURORE_USE_GXM)
    set(AURORE_DRIVER AURORE_USE_GXM)
    if (USE_VITA_SHARK)
        list(APPEND AURORE_PUBLIC_DEFINITIONS USE_VITA_SHARK)
    endif ()
endif ()
list(APPEND AURORE_PUBLIC_DEFINITIONS ${AURORE_DRIVER})
if (AURORE_INSPECTOR)
    list(APPEND AURORE_PRIVATE_DEFINITIONS AURORE_INSPECTOR)
endif ()
# The tests read it too (tests/dev_tools_tests.cpp)
if (AURORE_DEV_TOOLS STREQUAL "AUTO")
    set(AURORE_DEV_TOOLS_DEFINITION "$<$<NOT:$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:AURORE_DEV_TOOLS>")
elseif (AURORE_DEV_TOOLS)
    set(AURORE_DEV_TOOLS_DEFINITION AURORE_DEV_TOOLS)
else ()
    set(AURORE_DEV_TOOLS_DEFINITION "")
endif ()
set(AURORE_DEV_TOOLS_DEFINITION "${AURORE_DEV_TOOLS_DEFINITION}" CACHE INTERNAL "")
list(APPEND AURORE_PRIVATE_DEFINITIONS ${AURORE_DEV_TOOLS_DEFINITION})
message(STATUS "aurore: driver ${AURORE_DRIVER}")

# The library: target aurore, Aurore::aurore for the apps
add_library(aurore STATIC ${AURORE_SRC})
add_library(Aurore::aurore ALIAS aurore)
# C++23, which every toolchain has, the PS4's and the Vita's included
set_target_properties(aurore PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        UNITY_BUILD ${AURORE_UNITY_BUILD}
        # No C++ modules: no dependency scanning step (CMake >= 3.28)
        CXX_SCAN_FOR_MODULES OFF
)
set_source_files_properties(${AURORE_PATH}/lib/platforms/desktop/desktop_darwin.mm PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON)
target_include_directories(aurore PUBLIC
        $<BUILD_INTERFACE:${AURORE_PATH}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
# The headers of other projects shipped in include/aurore/extern, before
# the ones found on the system
set(AURORE_EXTERN_INCLUDE extern extern/nanovg extern/qrcodegen)
if (NOT USE_SYSTEM_TINYXML2 AND NOT AURORE_FETCH_DEPS)
    list(APPEND AURORE_EXTERN_INCLUDE extern/tinyxml2)
endif ()
foreach (dir ${AURORE_EXTERN_INCLUDE})
    target_include_directories(aurore SYSTEM PUBLIC
            $<BUILD_INTERFACE:${AURORE_PATH}/include/aurore/${dir}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/aurore/${dir}>)
endforeach ()
if (AURORE_INCLUDE)
    target_include_directories(aurore SYSTEM PUBLIC ${AURORE_INCLUDE})
endif ()
if (AURORE_DKSH_INCLUDE)
    target_include_directories(aurore PRIVATE ${AURORE_DKSH_INCLUDE})
endif ()
# The vendored projects' headers are system headers too, for the library and the apps
foreach (dep yogacore tinyxml2 glfw SDL2-static SDL2main SDL3-static SDL3-shared ${LIBROMFS_LIBRARY})
    if (TARGET ${dep})
        get_target_property(dep_imported ${dep} IMPORTED)
        get_target_property(dep_include ${dep} INTERFACE_INCLUDE_DIRECTORIES)
        if (NOT dep_imported AND dep_include)
            set_property(TARGET ${dep} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${dep_include})
        endif ()
    endif ()
endforeach ()
# The projects that link the library build with C++23 at least, so its headers
# can use it
target_compile_features(aurore PUBLIC cxx_std_23)
target_link_libraries(aurore
        PUBLIC yogacore ${AURORE_PLATFORM_LIBS}
        PRIVATE ${AURORE_PRIVATE_LIBS} $<BUILD_INTERFACE:tweeny::tweeny>)
if (PLATFORM_PSV)
    # vita-elf-create writes the module's SCE data (module info, exports,
    # imports: about 3 KB for the demo) right after the text segment, and
    # fails when the data segment starts there. VitaSDK's linker aligns the
    # data segment to 64 KiB, so a text segment ending less than that before
    # a boundary fails at random as the code grows; __sce_headroom (VitaSDK
    # 2026.08's linker script) keeps room between the two segments. An
    # older linker ignores the symbol.
    target_link_options(aurore INTERFACE "LINKER:--defsym=__sce_headroom=0x4000")
endif ()
list(REMOVE_DUPLICATES AURORE_PUBLIC_DEFINITIONS)
target_compile_definitions(aurore
        PUBLIC
        AURORE_RESOURCES=${AURORE_PLATFORM_RESOURCES_PATH}
        ${AURORE_PUBLIC_DEFINITIONS}
        PRIVATE
        ${AURORE_PRIVATE_DEFINITIONS})
target_compile_options(aurore PUBLIC ${AURORE_PUBLIC_OPTIONS})
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # A section per function and per variable: an app linked with
    # aurore_app_link_options() (toolchain.cmake) drops the ones it does not use
    target_compile_options(aurore PRIVATE -ffunction-sections -fdata-sections)
    # The same warnings on every platform; never errors
    target_compile_options(aurore PRIVATE -Wall)
endif ()

# Test harness: a headless platform and aurore::test::Harness (include/aurore/test/,
# docs/testing.md), for an app's tests to link: target Aurore::test
if (AURORE_BUILD_TESTS)
    if (NOT PLATFORM_DESKTOP OR NOT AURORE_DRIVER STREQUAL "AURORE_USE_OPENGL" OR USE_GL2 OR USE_GLES2 OR USE_GLES3)
        message(FATAL_ERROR "AURORE_BUILD_TESTS needs the desktop platform with the OpenGL 3 driver (the default)")
    endif ()
    add_library(aurore_test STATIC
            ${AURORE_PATH}/lib/test/headless_platform.cpp
            ${AURORE_PATH}/lib/test/harness.cpp)
    add_library(Aurore::test ALIAS aurore_test)
    set_target_properties(aurore_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
    target_link_libraries(aurore_test PUBLIC aurore)
    # stb_image_write, for the screenshots: the file GLFW ships, not its
    # folder (its glad/gl.h would hide the library's)
    target_compile_definitions(aurore_test PRIVATE
            AURORE_STB_IMAGE_WRITE="${AURORE_PATH}/lib/extern/glfw/deps/stb_image_write.h")
    if (UNIX AND NOT APPLE)
        # A context without any window system (Mesa's surfaceless EGL)
        find_package(OpenGL COMPONENTS EGL)
        if (OpenGL_EGL_FOUND)
            # The library's path here, Aurore::egl for an installed copy (made
            # by AuroreConfig.cmake): the imported target is only visible in
            # this directory, a static library hands its link dependencies to
            # its users, and OpenGL::EGL would also ask for libOpenGL
            target_include_directories(aurore_test SYSTEM PRIVATE ${OPENGL_EGL_INCLUDE_DIRS})
            target_link_libraries(aurore_test PRIVATE
                    $<BUILD_INTERFACE:${OPENGL_egl_LIBRARY}> $<INSTALL_INTERFACE:Aurore::egl>)
            set(AURORE_TEST_EGL ON)
            target_compile_definitions(aurore_test PRIVATE AURORE_HEADLESS_EGL)
        else ()
            message(WARNING "EGL not found (libegl-dev): the test harness will need an X server (a hidden GLFW window)")
        endif ()
    endif ()

    include(${AURORE_PATH}/cmake/AuroreTesting.cmake)
endif ()

if (AURORE_INSTALL)
    include(${AURORE_PATH}/cmake/install.cmake)
endif ()
