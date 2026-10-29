# SPDX-FileCopyrightText: 2026 JigSawFr
# SPDX-License-Identifier: Apache-2.0

# An app built on Aurore: a desktop binary (SDL3, or GLFW or SDL2) and a Switch .nro.
#
#   cmake -B build -DAURORE_PLATFORM=DESKTOP && cmake --build build
#   cmake -B build-switch -DAURORE_PLATFORM=SWITCH && cmake --build build-switch --target my_app.nro
#
# See README.md.
cmake_minimum_required(VERSION 3.16...3.31)

# Aurore: a submodule at library/aurore (git submodule add -b main
# https://github.com/JigSawFr/aurore.git library/aurore), or, inside Aurore's
# own repository, the checkout around this template.
set(AURORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/aurore" CACHE PATH "Aurore's checkout")
if (NOT EXISTS "${AURORE_DIR}/library/CMakeLists.txt" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../library/CMakeLists.txt")
    set(AURORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif ()
if (NOT EXISTS "${AURORE_DIR}/library/CMakeLists.txt")
    message(FATAL_ERROR "Aurore not found in ${AURORE_DIR}: git submodule update --init --recursive")
endif ()
get_filename_component(AURORE_DIR "${AURORE_DIR}" ABSOLUTE)
set(AURORE_LIBRARY ${AURORE_DIR}/library)

# Aurore's options (AURORE_PLATFORM, AURORE_WINDOW, AURORE_GRAPHICS…) and toolchain;
# they set PLATFORM_DESKTOP, PLATFORM_SWITCH, USE_DEKO3D… for the tests below
include(${AURORE_LIBRARY}/cmake/commonOption.cmake)
include(${AURORE_LIBRARY}/cmake/toolchain.cmake)

project(my_app)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_ALTER "0")
set(PROJECT_AUTHOR "Me")
set(PROJECT_TITLEID "")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# The app's resources go on top of Aurore's (fonts, icons, its strings and
# Cadran's) in one directory: next to the binary on desktop, in the .nro's
# romfs on the Switch.
set(MERGED_RESOURCES ${CMAKE_BINARY_DIR}/resources)
if (PLATFORM_DESKTOP)
    set(AURORE_RESOURCES_ROOT ${CMAKE_BINARY_DIR})
endif ()

add_subdirectory(${AURORE_LIBRARY} ${CMAKE_BINARY_DIR}/aurore)

find_package(Threads REQUIRED)

file(GLOB_RECURSE APP_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
if (PLATFORM_SWITCH)
    list(APPEND APP_SRC ${AURORE_LIBRARY}/lib/platforms/switch/switch_wrapper.c)
endif ()

program_target(${PROJECT_NAME} "${APP_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE Aurore::aurore ${CMAKE_THREAD_LIBS_INIT})
if (PLATFORM_DESKTOP)
    aurore_app_link_options(${PROJECT_NAME})
endif ()

# Aurore's demo assets stay out (its screens, pictures, strings and example
# themes: the built-in presets are in the library); on the Switch, so do the
# fonts the system already has (Cadran's own, Sora, stays).
set(ASSEMBLE_RESOURCES
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${MERGED_RESOURCES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${AURORE_DIR}/resources ${MERGED_RESOURCES}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${MERGED_RESOURCES}/xml ${MERGED_RESOURCES}/img/gallery ${MERGED_RESOURCES}/themes
    COMMAND ${CMAKE_COMMAND} -E remove -f ${MERGED_RESOURCES}/img/tiles.png ${MERGED_RESOURCES}/img/demo_icon.jpg
            ${MERGED_RESOURCES}/i18n/en-US/demo.json ${MERGED_RESOURCES}/i18n/en-US/scroll.json
            ${MERGED_RESOURCES}/i18n/fr/demo.json ${MERGED_RESOURCES}/i18n/zh-Hans/demo.json
            ${MERGED_RESOURCES}/i18n/zh-Hans/scroll.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${MERGED_RESOURCES})

if (PLATFORM_SWITCH)
    list(APPEND ASSEMBLE_RESOURCES
        COMMAND ${CMAKE_COMMAND} -E remove -f ${MERGED_RESOURCES}/font/aurore_text.ttf ${MERGED_RESOURCES}/font/aurore_buttons.ttf)
endif ()

add_custom_target(${PROJECT_NAME}_resources ${ASSEMBLE_RESOURCES} COMMENT "Assembling the resources")
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_resources)

if (PLATFORM_SWITCH)
    set(NACP_TITLEID)
    if (PROJECT_TITLEID)
        set(NACP_TITLEID --titleid=${PROJECT_TITLEID})
    endif ()
    add_custom_target(${PROJECT_NAME}.nro ALL DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}"
                "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${PROJECT_NAME}.nacp ${NACP_TITLEID}
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON}
                --nacp=${PROJECT_NAME}.nacp --romfsdir=${MERGED_RESOURCES})
endif ()
