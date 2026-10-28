cmake_minimum_required(VERSION 3.16...3.31)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(GNUInstallDirs)
set(BOREALIS_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(BOREALIS_SOURCE)
set(BOREALIS_SRC)
# The usage requirements of the aurore target, filled in below. Headers of
# other projects are system headers: their warnings are not the library's.
set(BOREALIS_INCLUDE)
set(BRLS_PLATFORM_LIBS)
set(BRLS_PRIVATE_LIBS)
set(BRLS_PUBLIC_DEFINITIONS ${BRLS_TOOLCHAIN_DEFINITIONS})
set(BRLS_PRIVATE_DEFINITIONS)
set(BRLS_PUBLIC_OPTIONS)
# The vendored projects install their packages along with the library
set(BRLS_EXTERN_EXCLUDE EXCLUDE_FROM_ALL)
if (AURORE_INSTALL)
    set(BRLS_EXTERN_EXCLUDE)
endif ()
# Desktop: the directory that holds "resources/" at run time (default: the
# working directory). An app sets it before add_subdirectory(); it can be
# given with -DBRLS_RESOURCES_DIR=... too (for an AppImage, say).
if (NOT BRLS_RESOURCES_DIR)
    if (CUSTOM_RESOURCES_DIR)
        message(DEPRECATION "CUSTOM_RESOURCES_DIR is deprecated: use -DBRLS_RESOURCES_DIR=${CUSTOM_RESOURCES_DIR}")
        set(BRLS_RESOURCES_DIR "${CUSTOM_RESOURCES_DIR}")
    else ()
        set(BRLS_RESOURCES_DIR ".")
    endif ()
endif ()

# The version of library/include/borealis/version.hpp, which release-please updates
file(STRINGS ${BOREALIS_PATH}/include/borealis/version.hpp AURORE_VERSION REGEX "^#define AURORE_VERSION \"")
string(REGEX REPLACE ".*\"([0-9.]+)\".*" "\\1" AURORE_VERSION "${AURORE_VERSION}")
set(AURORE_VERSION ${AURORE_VERSION} PARENT_SCOPE)

option(AURORE_INSTALL "Install the library, its headers and its CMake package (AuroreConfig.cmake); desktop only" OFF)

# fmt, tinyxml2, GLFW and SDL2 from the system when find_package() finds
# them, downloaded at configure time otherwise, instead of the copies in
# lib/extern (for packagers). nanovg, yoga and the others stay vendored.
option(AURORE_FETCH_DEPS "Use fmt, tinyxml2, GLFW and SDL2 from the system or downloaded (CMake 3.24+)" OFF)
if (AURORE_FETCH_DEPS)
    if (CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "AURORE_FETCH_DEPS needs CMake 3.24 or later (FIND_PACKAGE_ARGS)")
    endif ()
    include(FetchContent)
    # The versions of the vendored copies: tools/check_extern_versions.py
    # (CI) checks they match. A system fmt 12 or later will do.
    FetchContent_Declare(fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 12.2.0
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS 12)
    FetchContent_Declare(tinyxml2
            GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
            GIT_TAG 11.0.0
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS)
    # The vendored GLFW is xfangfang's fork, for its fixes
    FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/xfangfang/glfw.git
            GIT_TAG 892256c3f630739fb02552544b8d83240883ec8a
            FIND_PACKAGE_ARGS NAMES glfw3)
    FetchContent_Declare(SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.28.5
            GIT_SHALLOW TRUE
            FIND_PACKAGE_ARGS)
endif ()

# fmt
if (USE_SYSTEM_FMT)
  find_package(fmt REQUIRED)
elseif (AURORE_FETCH_DEPS)
  FetchContent_MakeAvailable(fmt)
else()
  if (AURORE_INSTALL)
    set(FMT_INSTALL ON CACHE BOOL "" FORCE)
  endif ()
  add_subdirectory(${BOREALIS_PATH}/lib/extern/fmt ${BRLS_EXTERN_EXCLUDE})
endif()

# tweeny
if (USE_SYSTEM_TWEENY)
  find_package(Tweeny REQUIRED)
else()
  add_subdirectory(${BOREALIS_PATH}/lib/extern/tweeny ${BRLS_EXTERN_EXCLUDE})
endif()

# yoga 3.2.1 (facebook/yoga tag v3.2.1, 042f501, MIT), library sources only
add_subdirectory(${BOREALIS_PATH}/lib/extern/yoga/yoga EXCLUDE_FROM_ALL)
# Its headers' installed place, relative to the prefix (yoga's own is absolute)
set_property(TARGET yogacore PROPERTY INTERFACE_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${BOREALIS_PATH}/lib/extern/yoga>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/yoga>)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(yogacore PRIVATE -fvisibility=default -Wno-error)
    # C++20 library parts Yoga 3 needs that some toolchains' libraries lack
    target_compile_options(yogacore PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/yoga_std_compat.h)
endif ()

# opencc
if (BRLS_USE_OPENCC)
    list(APPEND BRLS_PLATFORM_LIBS libopencc)
    list(APPEND BOREALIS_INCLUDE ${BRLS_OPENCC_INCLUDE})
    list(APPEND BRLS_PUBLIC_DEFINITIONS OPENCC)
endif ()

# boost::filesystem
if (USE_BOOST_FILESYSTEM)
    find_package(Boost 1.44 REQUIRED COMPONENTS filesystem)
    if(Boost_FOUND)
        list(APPEND BRLS_PRIVATE_LIBS Boost::filesystem)
        list(APPEND BRLS_PRIVATE_DEFINITIONS USE_BOOST_FILESYSTEM)
    endif()
elseif ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1) OR
    (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT WIN32 AND NOT PLATFORM_ANDROID AND NOT PLATFORM_PS4))
    list(APPEND BRLS_PLATFORM_LIBS stdc++fs)
endif ()

# libromfs
if(USE_LIBROMFS)
    add_subdirectory(${BOREALIS_PATH}/lib/extern/libromfs EXCLUDE_FROM_ALL)
    list(APPEND BRLS_PLATFORM_LIBS ${LIBROMFS_LIBRARY})
    list(APPEND BRLS_PUBLIC_DEFINITIONS USE_LIBROMFS)
endif()

# tinyxml2
if (USE_SYSTEM_TINYXML2)
    find_package(tinyxml2 REQUIRED COMPONENTS shared)

    list(APPEND BOREALIS_INCLUDE ${tinyxml2_INCLUDE_DIRS})
    list(APPEND BRLS_PLATFORM_LIBS tinyxml2::tinyxml2)
elseif (AURORE_FETCH_DEPS)
    set(tinyxml2_BUILD_TESTING OFF)
    FetchContent_MakeAvailable(tinyxml2)
    list(APPEND BRLS_PLATFORM_LIBS tinyxml2::tinyxml2)
else()
    add_library(tinyxml2 STATIC ${BOREALIS_PATH}/lib/extern/tinyxml2/tinyxml2.cpp)
    target_include_directories(tinyxml2 SYSTEM PUBLIC
            $<BUILD_INTERFACE:${BOREALIS_PATH}/include/borealis/extern/tinyxml2>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/borealis/extern/tinyxml2>)
    list(APPEND BRLS_PLATFORM_LIBS tinyxml2)
endif()

# dbus (Linux with GLFW: keeps the screen saver off). An imported target,
# so the installed package names no path of this machine.
if (PLATFORM_DESKTOP AND UNIX AND NOT APPLE AND NOT USE_SDL2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
    list(APPEND BRLS_PLATFORM_LIBS PkgConfig::DBUS)
endif ()

if (PLATFORM_DESKTOP)
    find_package(Threads REQUIRED)
    list(APPEND BRLS_PLATFORM_LIBS Threads::Threads)

    if (USE_SDL2)
        if (USE_SYSTEM_SDL2)
            find_package(SDL2 REQUIRED)
            message(STATUS "Find SDL2: ${SDL2_INCLUDE_DIR}, ${SDL2_LIBRARIES}")
            list(APPEND BRLS_PLATFORM_LIBS ${SDL2_LIBRARIES})
            list(APPEND BOREALIS_INCLUDE ${SDL2_INCLUDE_DIRS})
        elseif (AURORE_FETCH_DEPS)
            set(BUILD_SHARED_LIBS OFF)
            set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
            FetchContent_MakeAvailable(SDL2)
            if (sdl2_SOURCE_DIR)
                list(APPEND BRLS_PLATFORM_LIBS SDL2::SDL2-static SDL2::SDL2main)
            else ()
                list(APPEND BRLS_PLATFORM_LIBS SDL2::SDL2 SDL2::SDL2main)
            endif ()
        else ()
            set(BUILD_SHARED_LIBS OFF)
            set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
            if (AURORE_INSTALL)
                set(SDL2_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
            endif ()
            add_subdirectory(${BOREALIS_PATH}/lib/extern/SDL ${BRLS_EXTERN_EXCLUDE})
            list(APPEND BRLS_PLATFORM_LIBS SDL2::SDL2-static SDL2::SDL2main)
        endif ()
        list(APPEND BRLS_PUBLIC_DEFINITIONS __SDL2__)
        list(APPEND BOREALIS_SOURCE ${BOREALIS_PATH}/lib/platforms/sdl)
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
            add_subdirectory(${BOREALIS_PATH}/lib/extern/glfw ${BRLS_EXTERN_EXCLUDE})
        endif ()
        list(APPEND BRLS_PLATFORM_LIBS glfw)
        list(APPEND BRLS_PUBLIC_DEFINITIONS __GLFW__)
        list(APPEND BOREALIS_SOURCE ${BOREALIS_PATH}/lib/platforms/glfw)
    endif ()
    if (BOREALIS_USE_D3D11 AND NOT WIN32)
        message(FATAL_ERROR "AURORE_GRAPHICS=D3D11 is for Windows")
    endif ()
    if (WIN32)
        if (BOREALIS_USE_D3D11)
            list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/driver/d3d11.cpp)
            list(APPEND BRLS_PLATFORM_LIBS d3d11)
        endif ()
        if (MINGW)
            list(APPEND BRLS_PUBLIC_DEFINITIONS WINVER=0x0605)
        endif ()
        list(APPEND BRLS_PLATFORM_LIBS wlanapi iphlpapi ws2_32)
    elseif (APPLE)
        # C++20 needs macOS 10.15, which has them all
        list(APPEND BRLS_PLATFORM_LIBS "-framework CoreWLAN" "-framework CoreGraphics")
        list(APPEND BRLS_PLATFORM_LIBS "-framework SystemConfiguration" "-framework Cocoa")
        list(APPEND BRLS_PLATFORM_LIBS "-framework IOKit" "-framework CoreFoundation")
        list(APPEND BRLS_PLATFORM_LIBS "-framework AppKit" "-framework Foundation" "-framework CoreServices" "-lobjc")
        # The system's fonts for Chinese, Japanese and Korean (desktop_font.cpp)
        list(APPEND BRLS_PLATFORM_LIBS "-framework CoreText")
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/desktop/desktop_darwin.mm)
    elseif (UNIX)
        # fontconfig, opened at run time when there (desktop_font.cpp)
        list(APPEND BRLS_PLATFORM_LIBS ${CMAKE_DL_LIBS})
    endif ()
    list(APPEND BOREALIS_SOURCE ${BOREALIS_PATH}/lib/platforms/desktop)
    set(BRLS_PLATFORM_RESOURCES_PATH "\"${BRLS_RESOURCES_DIR}/resources/\"")
elseif (PLATFORM_PSV)
    list(APPEND BOREALIS_SOURCE ${BOREALIS_PATH}/lib/platforms/desktop)
    list(APPEND BRLS_PLATFORM_LIBS
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
    if (BOREALIS_USE_GXM)
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
                list(APPEND BRLS_PLATFORM_LIBS
                        vitashark
                        SceShaccCgExt
                        taihen_stub
                        SceShaccCg_stub
                )
        endif ()
        file(GLOB_RECURSE PSV_PLATFORM_SRC CONFIGURE_DEPENDS "${BOREALIS_PATH}/lib/platforms/psv/*.cpp")
        list(APPEND BOREALIS_SRC ${PSV_PLATFORM_SRC})
    else()
        if (USE_SYSTEM_SDL2)
            find_package(SDL2 REQUIRED)
        else ()
            set(BUILD_SHARED_LIBS OFF)
            set(VIDEO_VITA_PVR ${BOREALIS_USE_GXM} CACHE BOOL "")
            add_subdirectory(${BOREALIS_PATH}/lib/extern/SDL EXCLUDE_FROM_ALL)
        endif ()
        list(APPEND BRLS_PLATFORM_LIBS
                SDL2::SDL2-static
                libgpu_es4_ext_stub_weak
                libIMGEGL_stub_weak
                libGLESv2_stub_weak)
        # Using SDL implementation instead
        file(GLOB_RECURSE SDL_PLATFORM_SRC CONFIGURE_DEPENDS "${BOREALIS_PATH}/lib/platforms/sdl/*.cpp")
        list(APPEND BOREALIS_SRC ${SDL_PLATFORM_SRC})
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/psv/psv_platform.cpp)
    endif ()
    list(APPEND BRLS_PUBLIC_DEFINITIONS __PSV__ STBI_NEON)
    set(BRLS_PLATFORM_RESOURCES_PATH "\"app0:resources/\"")
    set_target_properties(yogacore PROPERTIES POSITION_INDEPENDENT_CODE OFF)
elseif(PLATFORM_PS4)
    find_package(SDL2 REQUIRED)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/sdl
            ${BOREALIS_PATH}/lib/platforms/desktop
            ${BOREALIS_PATH}/lib/extern/nanovg/ps4)
    file(GLOB_RECURSE PS4_PLATFORM_SRC CONFIGURE_DEPENDS "${BOREALIS_PATH}/lib/platforms/ps4/*.cpp")
    list(APPEND BOREALIS_SRC ${PS4_PLATFORM_SRC})
    list(APPEND BOREALIS_INCLUDE /opt/OpenOrbis-PS4-Toolchain/include)
    list(APPEND BRLS_PLATFORM_LIBS SDL2::SDL2-static SceNetCtl SceImeDialog SceCommonDialog)
    list(APPEND BRLS_PUBLIC_DEFINITIONS __SDL2__ PS4 USE_GLES2)
    if (LIBJBC)
        list(APPEND BRLS_PLATFORM_LIBS jbc)
        list(APPEND BRLS_PUBLIC_DEFINITIONS USE_JBC)
        set(BRLS_PLATFORM_RESOURCES_PATH "\"/mnt/sandbox/${PSN_TITLE_ID}_000/app0/resources/\"")
    else ()
        set(BRLS_PLATFORM_RESOURCES_PATH "\"/app0/resources/\"")
    endif ()
elseif(PLATFORM_ANDROID)
    find_library(SDL2 NAMES SDL2 SDL2main)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/sdl
            ${BOREALIS_PATH}/lib/platforms/desktop
            ${BOREALIS_PATH}/lib/platforms/android)
    list(APPEND BRLS_PLATFORM_LIBS SDL2 SDL2main)
    list(APPEND BRLS_PUBLIC_DEFINITIONS __SDL2__ ANDROID)
    # The NDK's libc++ returns std::string from path::u8string() in C++20,
    # which nlohmann json's path conversion does not build with
    list(APPEND BRLS_PUBLIC_DEFINITIONS JSON_HAS_FILESYSTEM=0 JSON_HAS_EXPERIMENTAL_FILESYSTEM=0)
elseif(PLATFORM_IOS OR PLATFORM_TVOS)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(${BOREALIS_PATH}/lib/extern/SDL)
    find_library(SDL2 NAMES SDL2-static SDL2main)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/sdl
            ${BOREALIS_PATH}/lib/platforms/desktop)
    list(APPEND BRLS_PLATFORM_LIBS SDL2-static SDL2main "-framework CoreText" "-framework CoreFoundation")
    list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/desktop/ios_darwin.mm)

    if(PLATFORM_IOS)
        list(APPEND BRLS_PUBLIC_DEFINITIONS __SDL2__ IOS)
    elseif(PLATFORM_TVOS)
        list(APPEND BRLS_PUBLIC_DEFINITIONS __SDL2__ TVOS)
    endif()
elseif(PLATFORM_SWITCH)
    file(GLOB_RECURSE SWITCH_PLATFORM_SRC CONFIGURE_DEPENDS "${BOREALIS_PATH}/lib/platforms/switch/*.cpp")
    list(APPEND BOREALIS_SRC ${SWITCH_PLATFORM_SRC})
    list(APPEND BRLS_PUBLIC_OPTIONS $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>)
    list(APPEND BRLS_PUBLIC_DEFINITIONS SWITCH __SWITCH__ STBI_NO_THREAD_LOCALS)
    list(APPEND BRLS_PRIVATE_DEFINITIONS HAVE_LIBNX)
    find_package(PkgConfig REQUIRED)

    if (BOREALIS_USE_DEKO3D)
        list(APPEND BRLS_PLATFORM_LIBS deko3d nx m)
    elseif (USE_SDL2)
        find_package(SDL2 REQUIRED)
        list(REMOVE_ITEM BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/switch/switch_video.cpp)
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/sdl/sdl_video.cpp)
        list(APPEND BRLS_PLATFORM_LIBS ${SDL2_LIBRARIES} EGL glapi drm_nouveau nx m)
    else ()
        list(REMOVE_ITEM BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/switch/switch_video.cpp)
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/glfw/glfw_video.cpp)
        list(APPEND BRLS_PLATFORM_LIBS glfw3 EGL glapi drm_nouveau nx m)
    endif ()
    set(BRLS_PLATFORM_RESOURCES_PATH "\"romfs:/\"")
    # devkitPro's portlibs (GLFW, SDL2, EGL, curl…) last: it also has a
    # tinyxml2.h and a glad/glad.h, which must not hide the library's own
    list(APPEND BOREALIS_INCLUDE ${DEVKITPRO}/portlibs/switch/include)
else ()
    message(FATAL_ERROR "Please set the platform to build for. Example: -DAURORE_PLATFORM=DESKTOP or -DAURORE_PLATFORM=SWITCH")
endif ()

# aurore c/cpp source
list(APPEND BOREALIS_SOURCE
        ${BOREALIS_PATH}/lib/core
        ${BOREALIS_PATH}/lib/views
        ${BOREALIS_PATH}/lib/extern/glad
        ${BOREALIS_PATH}/lib/extern/libretro-common/compat
        ${BOREALIS_PATH}/lib/extern/libretro-common/encodings
        ${BOREALIS_PATH}/lib/extern/libretro-common/features
        ${BOREALIS_PATH}/lib/extern/qrcodegen
        # Cadran, Aurore's look (GPLv3, LICENSE.GPL)
        ${BOREALIS_PATH}/lib/cadran
        )
if (PLATFORM_PSV OR PLATFORM_PS4 OR BOREALIS_USE_D3D11)
        list(REMOVE_ITEM BOREALIS_SOURCE ${BOREALIS_PATH}/lib/extern/glad)
endif ()

# nanovg
list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/extern/nanovg/nanovg.c)
if (BRLS_FONTSTASH_STREAM)
    set_source_files_properties(
        ${BOREALIS_PATH}/lib/extern/nanovg/nanovg.c
        PROPERTIES
        COMPILE_DEFINITIONS "FONTSTASH_STREAM_IMPLEMENTATION"
    )
    message(STATUS "aurore: fontstash use stream")
endif ()
if (USE_FREETYPE)
    if (BRLS_FONTSTASH_STREAM)
        message(FATAL_ERROR "USE_FREETYPE cannot be combined with BRLS_FONTSTASH_STREAM")
    endif ()
    if (PLATFORM_SWITCH)
        # switch-freetype links bz2, png, zlib and harfbuzz: pkg-config knows them
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(BRLS_FREETYPE REQUIRED IMPORTED_TARGET freetype2)
        list(APPEND BRLS_PLATFORM_LIBS PkgConfig::BRLS_FREETYPE)
    else ()
        find_package(Freetype REQUIRED)
        list(APPEND BRLS_PLATFORM_LIBS Freetype::Freetype)
    endif ()
    set_property(SOURCE ${BOREALIS_PATH}/lib/extern/nanovg/nanovg.c APPEND PROPERTY COMPILE_DEFINITIONS FONS_USE_FREETYPE)
    list(APPEND BRLS_PUBLIC_DEFINITIONS BRLS_USE_FREETYPE)
    message(STATUS "aurore: fontstash uses FreeType")
endif ()
if (USE_FRIBIDI)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(BRLS_FRIBIDI REQUIRED IMPORTED_TARGET fribidi)
    list(APPEND BRLS_PLATFORM_LIBS PkgConfig::BRLS_FRIBIDI)
    list(APPEND BRLS_PUBLIC_DEFINITIONS BRLS_USE_FRIBIDI)
    message(STATUS "aurore: labels reorder right-to-left text with FriBidi")
endif ()
if (PLATFORM_SWITCH AND BOREALIS_USE_DEKO3D)
    file(GLOB_RECURSE NANOVG_PLATFORM_SRC CONFIGURE_DEPENDS ${BOREALIS_PATH}/lib/extern/nanovg/*.cpp)
    list(APPEND BOREALIS_SRC ${NANOVG_PLATFORM_SRC})

    # nanovg's deko3d shaders, compiled and built into the library: an app
    # no longer copies .dksh files into its romfs (those it has still load
    # if these are missing)
    find_program(NX_UAM_EXE NAMES uam HINTS "${DEVKITPRO}/tools/bin")
    set(DKSH_GLSL "${BOREALIS_PATH}/lib/extern/nanovg/deko3d/shaders")
    set(DKSH_OUT "${CMAKE_CURRENT_BINARY_DIR}/dksh")
    file(MAKE_DIRECTORY "${DKSH_OUT}")
    set(DKSH_HEADER "/* Generated by library/CMakeLists.txt from nanovg's deko3d shaders. */\n#pragma once\n#include <cstddef>\n")
    foreach (shader fill_vsh:vert fill_aa_fsh:frag fill_fsh:frag)
        string(REPLACE ":" ";" parts ${shader})
        list(GET parts 0 name)
        list(GET parts 1 stage)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DKSH_GLSL}/${name}.glsl")
        if (NX_UAM_EXE)
            execute_process(COMMAND ${NX_UAM_EXE} -s ${stage} -o "${DKSH_OUT}/${name}.dksh" "${DKSH_GLSL}/${name}.glsl"
                RESULT_VARIABLE uam_result TIMEOUT 30)
        else ()
            set(uam_result 1)
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
    set(BOREALIS_DKSH_INCLUDE "${DKSH_OUT}")
endif ()

# Every .c and .cpp under these directories: CONFIGURE_DEPENDS has the build
# run CMake again when one is added or removed
foreach (lib ${BOREALIS_SOURCE})
    file(GLOB_RECURSE TEMP_SRC CONFIGURE_DEPENDS "${lib}/*.cpp" "${lib}/*.c")
    list(APPEND BOREALIS_SRC ${TEMP_SRC})
endforeach (lib)

# The graphics driver
set(BOREALIS_DRIVER BOREALIS_USE_OPENGL)
if (BOREALIS_USE_METAL)
    set(BOREALIS_DRIVER BOREALIS_USE_METAL)
elseif (BOREALIS_USE_D3D11)
    set(BOREALIS_DRIVER BOREALIS_USE_D3D11)
elseif (BOREALIS_USE_DEKO3D)
    set(BOREALIS_DRIVER BOREALIS_USE_DEKO3D)
elseif (BOREALIS_USE_GXM)
    set(BOREALIS_DRIVER BOREALIS_USE_GXM)
    if (USE_VITA_SHARK)
        list(APPEND BRLS_PUBLIC_DEFINITIONS USE_VITA_SHARK)
    endif ()
endif ()
list(APPEND BRLS_PUBLIC_DEFINITIONS ${BOREALIS_DRIVER})
if (BRLS_INSPECTOR)
    list(APPEND BRLS_PRIVATE_DEFINITIONS BRLS_INSPECTOR)
endif ()
message(STATUS "aurore: driver ${BOREALIS_DRIVER}")

# The library: target aurore, Aurore::aurore for the apps
add_library(aurore STATIC ${BOREALIS_SRC})
add_library(Aurore::aurore ALIAS aurore)
set_target_properties(aurore PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        UNITY_BUILD ${BRLS_UNITY_BUILD}
        # No C++20 modules: no dependency scanning step (CMake >= 3.28)
        CXX_SCAN_FOR_MODULES OFF
)
set_source_files_properties(${BOREALIS_PATH}/lib/platforms/desktop/desktop_darwin.mm PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON)
target_include_directories(aurore PUBLIC
        $<BUILD_INTERFACE:${BOREALIS_PATH}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
# The headers of other projects shipped in include/borealis/extern, before
# the ones found on the system
set(BRLS_EXTERN_INCLUDE extern extern/nanovg extern/qrcodegen)
if (NOT USE_SYSTEM_TINYXML2 AND NOT AURORE_FETCH_DEPS)
    list(APPEND BRLS_EXTERN_INCLUDE extern/tinyxml2)
endif ()
foreach (dir ${BRLS_EXTERN_INCLUDE})
    target_include_directories(aurore SYSTEM PUBLIC
            $<BUILD_INTERFACE:${BOREALIS_PATH}/include/borealis/${dir}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/borealis/${dir}>)
endforeach ()
if (BOREALIS_INCLUDE)
    target_include_directories(aurore SYSTEM PUBLIC ${BOREALIS_INCLUDE})
endif ()
if (BOREALIS_DKSH_INCLUDE)
    target_include_directories(aurore PRIVATE ${BOREALIS_DKSH_INCLUDE})
endif ()
# The vendored projects' headers are system headers too, for the library and the apps
foreach (dep fmt tweeny yogacore tinyxml2 glfw SDL2-static SDL2main ${LIBROMFS_LIBRARY})
    if (TARGET ${dep})
        get_target_property(dep_imported ${dep} IMPORTED)
        get_target_property(dep_include ${dep} INTERFACE_INCLUDE_DIRECTORIES)
        if (NOT dep_imported AND dep_include)
            set_property(TARGET ${dep} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${dep_include})
        endif ()
    endif ()
endforeach ()
# The projects that link the library build with C++20 too, so its headers can use it
target_compile_features(aurore PUBLIC cxx_std_20)
target_link_libraries(aurore
        PUBLIC fmt::fmt tweeny yogacore ${BRLS_PLATFORM_LIBS}
        PRIVATE ${BRLS_PRIVATE_LIBS})
list(REMOVE_DUPLICATES BRLS_PUBLIC_DEFINITIONS)
target_compile_definitions(aurore
        PUBLIC
        BRLS_RESOURCES=${BRLS_PLATFORM_RESOURCES_PATH}
        ${BRLS_PUBLIC_DEFINITIONS}
        PRIVATE
        ${BRLS_PRIVATE_DEFINITIONS})
target_compile_options(aurore PUBLIC ${BRLS_PUBLIC_OPTIONS})
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # A section per function and per variable: an app linked with
    # aurore_app_link_options() (toolchain.cmake) drops the ones it does not use
    target_compile_options(aurore PRIVATE -ffunction-sections -fdata-sections)
    # The same warnings on every platform; never errors
    target_compile_options(aurore PRIVATE -Wall)
endif ()

# Test harness: a headless platform and brls::test::Harness (include/borealis/test/,
# docs/testing.md), for an app's tests to link: target Aurore::test
if (BRLS_BUILD_TESTS)
    if (NOT PLATFORM_DESKTOP OR NOT BOREALIS_DRIVER STREQUAL "BOREALIS_USE_OPENGL" OR USE_GL2 OR USE_GLES2 OR USE_GLES3)
        message(FATAL_ERROR "BRLS_BUILD_TESTS needs the desktop platform with the OpenGL 3 driver (the default)")
    endif ()
    add_library(aurore_test STATIC
            ${BOREALIS_PATH}/lib/test/headless_platform.cpp
            ${BOREALIS_PATH}/lib/test/harness.cpp)
    add_library(Aurore::test ALIAS aurore_test)
    set_target_properties(aurore_test PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    target_link_libraries(aurore_test PUBLIC aurore)
    # stb_image_write, for the screenshots
    target_include_directories(aurore_test PRIVATE ${BOREALIS_PATH}/lib/extern/glfw/deps)
    if (UNIX AND NOT APPLE)
        # A context without any window system (Mesa's surfaceless EGL)
        find_package(OpenGL COMPONENTS EGL)
        if (OpenGL_EGL_FOUND)
            # The library's path here, OpenGL::EGL for an installed copy:
            # the imported target is only visible in this directory, and a
            # static library hands its link dependencies to its users
            target_include_directories(aurore_test SYSTEM PRIVATE ${OPENGL_EGL_INCLUDE_DIRS})
            target_link_libraries(aurore_test PRIVATE
                    $<BUILD_INTERFACE:${OPENGL_egl_LIBRARY}> $<INSTALL_INTERFACE:OpenGL::EGL>)
            set(AURORE_TEST_EGL ON)
            target_compile_definitions(aurore_test PRIVATE BRLS_HEADLESS_EGL)
        else ()
            message(WARNING "EGL not found (libegl-dev): the test harness will need an X server (a hidden GLFW window)")
        endif ()
    endif ()

    include(${BOREALIS_PATH}/cmake/AuroreTesting.cmake)
endif ()

if (AURORE_INSTALL)
    include(${BOREALIS_PATH}/cmake/install.cmake)
endif ()
