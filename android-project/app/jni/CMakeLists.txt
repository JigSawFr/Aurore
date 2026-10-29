cmake_minimum_required(VERSION 3.16...4.1)

project(AURORE_DEMO)

# armeabi-v7a requires cpufeatures library
# include(AndroidNdkModules)
# android_ndk_import_module_cpufeatures()

set(BUILD_SHARED_LIBS ON CACHE BOOL "")

# SDL2's sources are in a subfolder named "SDL", SDL3's in "SDL3" (the
# Gradle project gives AURORE_WINDOW: SDL2 or SDL3)
if (AURORE_WINDOW STREQUAL "SDL3")
    set(SDL_TEST_LIBRARY OFF)
    add_subdirectory(SDL3)
else ()
    add_subdirectory(SDL)
endif ()

# Compilation of companion libraries
#add_subdirectory(SDL_image)
#add_subdirectory(SDL_mixer)
#add_subdirectory(SDL_ttf)


set(AURORE_PLATFORM ANDROID CACHE STRING "")
SET(LIBROMFS_PREBUILT_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/aurore/libromfs-generator" CACHE STRING "")

if (NOT EXISTS "${LIBROMFS_PREBUILT_GENERATOR}")
    message(FATAL_ERROR "libromfs-generator has not been built, please refer to aurore/build_libromfs_generator.sh for more information")
endif()

add_subdirectory(aurore)
