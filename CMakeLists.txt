cmake_minimum_required(VERSION 3.16...3.31)

set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/library)

# build options
include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)

# Dependencies
option(USE_SHARED_LIB "Whether to use shared libs provided by system" OFF)
cmake_dependent_option(USE_SYSTEM_FMT "" OFF "NOT USE_SHARED_LIB" ON)
cmake_dependent_option(USE_SYSTEM_TINYXML2 "" OFF "NOT USE_SHARED_LIB" ON)
cmake_dependent_option(USE_SYSTEM_TWEENY "" OFF "NOT USE_SHARED_LIB" ON)

if (APPLE AND PLATFORM_DESKTOP)
    option(BUNDLE_MACOS_APP "Bundle a app for macOS" OFF)
    # Defaults: -DCMAKE_OSX_ARCHITECTURES=arm64 builds for one only
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=arm64] "11.0" CACHE STRING "")
    message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif ()

# toolchain
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

# project info
project(aurore_demo)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_ALTER "0")
set(VERSION_BUILD "0")
set(PACKAGE_NAME "com.aurore.demo")
set(PSN_TITLE_ID  "BRLS00000")
set(PSN_VERSION  "01.00")
set(PROJECT_AUTHOR "Aurore")
# Switch: the .nro's title id, none by default
set(PROJECT_TITLEID "")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/img/demo_icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)

if (USE_LIBROMFS)
    add_libromfs(${PROJECT_NAME} ${PROJECT_RESOURCES})
endif ()

add_subdirectory(library)

find_package(Threads REQUIRED)

file(GLOB_RECURSE MAIN_SRC CONFIGURE_DEPENDS demo/*.cpp)

set(PLATFORM_OPTION)
if (PLATFORM_DESKTOP)
    if (WIN32)
        configure_file("${PROJECT_SOURCE_DIR}/demo/resource.rc.in" "${CMAKE_BINARY_DIR}/resource.rc")
        list(APPEND MAIN_SRC ${CMAKE_BINARY_DIR}/resource.rc)
    elseif (BUNDLE_MACOS_APP)
        list(APPEND MAIN_SRC ${PROJECT_SOURCE_DIR}/demo/aurore.icns)
    endif ()
elseif (PLATFORM_SWITCH)
    list(APPEND MAIN_SRC ${PROJECT_SOURCE_DIR}/library/lib/platforms/switch/switch_wrapper.c)
elseif (PLATFORM_PS4)
    # Show backtrace when app crash
    list(APPEND MAIN_SRC ${PROJECT_SOURCE_DIR}/library/lib/platforms/ps4/crashlog.c)
endif ()


# building target
program_target(${PROJECT_NAME} "${MAIN_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_SCAN_FOR_MODULES OFF)
if (PLATFORM_DESKTOP)
    aurore_app_link_options(${PROJECT_NAME})
endif ()
if (PLATFORM_DESKTOP AND MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static)
endif ()


# building release file
if (PLATFORM_DESKTOP)
    if (BUNDLE_MACOS_APP)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}"
            MACOSX_BUNDLE_ICON_FILE "aurore.icns"
            MACOSX_BUNDLE_COPYRIGHT "Copyright 2026 ${PROJECT_AUTHOR}"
            RESOURCE "${PROJECT_SOURCE_DIR}/demo/aurore.icns"
        )
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${PROJECT_NAME}.app/Contents/Resources/resources
        )
    else ()
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources
        )
    endif ()
    if (NOT USE_LIBROMFS)
        add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)
    endif ()
elseif (PLATFORM_PSV)
    set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12") # max heap size mode
    vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
    set(PSV_ASSETS_FILES ${PROJECT_SOURCE_DIR}/psv/sce_sys sce_sys)
    if (NOT USE_LIBROMFS)
        list(APPEND PSV_ASSETS_FILES ${PROJECT_SOURCE_DIR}/resources resources)
    endif ()
    if (NOT USE_GXM)
        list(APPEND PSV_ASSETS_FILES ${PROJECT_SOURCE_DIR}/psv/module/ module)
    endif ()
    if (USE_VITA_SHARK)
        list(APPEND PSV_ASSETS_FILES "${CMAKE_BINARY_DIR}/vendor/SceShaccCg" module)
    endif ()
    vita_create_vpk(${PROJECT_NAME}.vpk ${PSN_TITLE_ID} ${PROJECT_NAME}.self
            VERSION ${PSN_VERSION}
            NAME ${PROJECT_NAME}
            FILE ${PSV_ASSETS_FILES}
    )
elseif (PLATFORM_PS4)
    # There is no `add_self` function without pacbrew toolchain.
    # We can install the OpenOrbits SDK so that the IDE can provide some basic code prompts,
    # but compilation is done through the pacbrew Docker container as pacbrew only supports Linux.
    if (PS4)
        # Defined by pacbrew toolchain
        add_self(${PROJECT_NAME})
        # ps4_pkg: the toolchain's add_pkg, fixed for directories like i18n/es and i18n/es-419
        ps4_pkg(${PROJECT_NAME}
                # Put all the resources inside `<build dir>/ps4` directory, and access through `/app0/...`
                # eg: `cmake-build/ps4/resources/icon.png` -> `/app0/resources/icon.png`
                ${CMAKE_BINARY_DIR}/ps4
                ${PSN_TITLE_ID}
                "Aurore demo"
                ${PSN_VERSION}
                # https://psdevwiki.com/ps4/param.sfo#ATTRIBUTE
                # Support ps4 pro
                8388608
        )
    endif()
    add_custom_target(${PROJECT_NAME}.data
        COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_SOURCE_DIR}/ps4 ${CMAKE_BINARY_DIR}/ps4
        COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/ps4/resources
    )
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)
elseif (PLATFORM_SWITCH)
    # deko3d's shaders are built into the library (library/CMakeLists.txt)
    set(NACP_TITLEID)
    if (PROJECT_TITLEID)
        set(NACP_TITLEID --titleid=${PROJECT_TITLEID})
    endif ()
    add_custom_target(${PROJECT_NAME}.nro DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${PROJECT_NAME}.nacp ${NACP_TITLEID}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        # The system's shared fonts take the place of the text and button
        # fonts; Cadran's Sora stays
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/resources/font/aurore_text.ttf
                ${CMAKE_BINARY_DIR}/resources/font/aurore_buttons.ttf
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
    )
elseif (PLATFORM_IOS)
    ios_bundle(
            "${CMAKE_CURRENT_SOURCE_DIR}/demo/ios/iphoneos/Splash.storyboard"
            "${CMAKE_CURRENT_SOURCE_DIR}/demo/ios/Images.xcassets"
            "${CMAKE_CURRENT_SOURCE_DIR}/demo/ios/iOSBundleInfo.plist.in"
            "Aurore"
            "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}")
elseif (PLATFORM_TVOS)
    ios_bundle(
            "${CMAKE_CURRENT_SOURCE_DIR}/demo/ios/tvos/Splash.storyboard"
            "${CMAKE_CURRENT_SOURCE_DIR}/demo/ios/Images.xcassets"
            "${CMAKE_CURRENT_SOURCE_DIR}/demo/ios/iOSBundleInfo.plist.in"
            "Aurore"
            "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}")
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE demo demo/include)
target_link_libraries(${PROJECT_NAME} PRIVATE Aurore::aurore Threads::Threads)

# The demo's tests, bench and aurore-preview, on the test harness
# (-DBRLS_BUILD_TESTS=ON, then ctest)
if (BRLS_BUILD_TESTS)
    # Their reference screenshots are drawn with the resources built in: from
    # files the desktop build picks other fonts
    if (USE_LIBROMFS)
        enable_testing()
        add_subdirectory(tests)
    elseif (AURORE_INSTALL)
        message(STATUS "aurore: the test harness is installed; the demo's tests need AURORE_LIBROMFS=ON")
    else ()
        message(FATAL_ERROR "The demo's tests need AURORE_LIBROMFS=ON (their reference screenshots are drawn with the resources built in)")
    endif ()
endif ()
