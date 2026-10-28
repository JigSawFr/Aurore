message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

function(check_libromfs_generator)
    if (NOT DEFINED LIBROMFS_PREBUILT_GENERATOR OR NOT EXISTS "${LIBROMFS_PREBUILT_GENERATOR}")
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libromfs-generator")
            set(LIBROMFS_PREBUILT_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/libromfs-generator" PARENT_SCOPE)
        elseif (DEFINED AURORE_LIBRARY AND EXISTS "${AURORE_LIBRARY}/../libromfs-generator")
            # A project that only includes this file keeps it at its root, so
            # the helper sits beside the checkout rather than beside the source
            # directory of the caller.
            set(LIBROMFS_PREBUILT_GENERATOR "${AURORE_LIBRARY}/../libromfs-generator" PARENT_SCOPE)
        else ()
            message(FATAL_ERROR "libromfs-generator has not been built: run build_libromfs_generator.sh at the root of Aurore's checkout")
        endif ()
    endif()
endfunction()

# Definitions the choices below make: the library gives them to the targets
# that link it (target_compile_definitions(aurore PUBLIC ...))
set(AURORE_TOOLCHAIN_DEFINITIONS)

if (PLATFORM_DESKTOP)
    message(STATUS "building for Desktop")
    if (NOT USE_GLFW AND NOT USE_SDL2)
        set(USE_GLFW ON)
    endif ()
elseif (PLATFORM_IOS OR PLATFORM_TVOS)
    if(PLATFORM_IOS)
        message(STATUS "building for iOS")
    elseif (PLATFORM_TVOS)
        message(STATUS "building for tvOS")
    endif ()
    set(ENABLE_STRICT_TRY_COMPILE ON)
    if (NOT DEFINED AURORE_LIBRARY)
        message(FATAL_ERROR AURORE_LIBRARY is not defined)
    endif ()
    set(CMAKE_TOOLCHAIN_FILE ${AURORE_LIBRARY}/cmake/ios.toolchain.cmake CACHE STRING "")
    set(USE_SDL2 ON)
    set(USE_GLFW OFF)
    if (NOT USE_GLES2)
        set(USE_GLES3 ON)
    endif()
    set(USE_LIBROMFS ON)
    if(PLATFORM_IOS)
        set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2") # iphone, ipad
    elseif(PLATFORM_TVOS)
        set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "3") # tvos
    endif()
    check_libromfs_generator()
elseif(PLATFORM_ANDROID)
    message(STATUS "building for Android")
    set(USE_SDL2 ON)
    set(USE_GLFW OFF)
    if (NOT USE_GLES3)
        set(USE_GLES2 ON)
    endif()
    set(USE_LIBROMFS ON)
    check_libromfs_generator()
elseif(PLATFORM_PSV)
    message(STATUS "building for PSVita")
    set(USE_SDL2 OFF)
    set(USE_GLFW OFF)
    if (NOT USE_GXM)
        set(USE_GLES2 ON)
        set(USE_SDL2 ON)
    endif ()
    if (NOT DEFINED ENV{VITASDK})
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif ()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    set(VITASDK $ENV{VITASDK} CACHE BOOL "VITASDK")
    include("${VITASDK}/share/vita.cmake" REQUIRED)
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS __psp2__ __PSV__)
    # Global on purpose: the vendored projects (SDL, yoga…) need them too
    add_compile_options(-Wno-error=pedantic -Wno-psabi)
elseif(PLATFORM_PS4)
    message(STATUS "building for PS4")
    set(USE_SDL2 ON)
    set(USE_GLFW OFF)
    set(USE_GLES2 ON)
    if (NOT DEFINED ENV{OPENORBIS})
        message(FATAL_ERROR "Please set OPENORBIS in your environment. export OPENORBIS=<path to>/openorbis")
    endif ()
    set(OPENORBIS $ENV{OPENORBIS} CACHE BOOL "OPENORBIS")
    set(CMAKE_TOOLCHAIN_FILE "${OPENORBIS}/cmake/ps4.cmake" CACHE PATH "toolchain file")
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS PS4 __PS4__)
elseif (PLATFORM_SWITCH)
    message(STATUS "building for SWITCH")
    if (NOT DEFINED ENV{DEVKITPRO})
        message(FATAL_ERROR "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
    endif ()
    set(DEVKITPRO $ENV{DEVKITPRO} CACHE BOOL "DEVKITPRO")
    # devkitPro's platform file gives the compilers their flags through
    # CMAKE_<LANG>_FLAGS_INIT (Cortex-A57 tuning, crc and crypto, the TLS
    # model libnx is built with, -D__SWITCH__) and libnx's include directory.
    # Setting CMAKE_C_FLAGS here, as before, hid them all. The portlibs'
    # include directory comes with the library target.
    include(${DEVKITPRO}/cmake/Switch.cmake REQUIRED)
    if (NOT USE_GLFW AND NOT USE_SDL2)
        set(USE_GLFW ON)
    endif ()
else()
    message(FATAL_ERROR "Please set the platform to build for. Example: -DAURORE_PLATFORM=DESKTOP or -DAURORE_PLATFORM=SWITCH")
endif ()

# graphics drivers
if (USE_DEKO3D)
    message(STATUS "USE_DEKO3D")
    set(AURORE_USE_DEKO3D ON)
elseif (USE_D3D11)
    message(STATUS "USE_D3D11")
    set(AURORE_USE_D3D11 ON)
elseif (USE_GXM)
    message(STATUS "USE_GXM")
    set(AURORE_USE_GXM ON)
elseif (USE_METAL)
    message(STATUS "USE_METAL")
    set(AURORE_USE_METAL ON)
elseif (USE_GL2)
    message(STATUS "USE_GL2")
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS USE_GL2)
elseif (USE_GLES2)
    message(STATUS "USE_GLES2")
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS USE_GLES2)
elseif (USE_GLES3)
    message(STATUS "USE_GLES3")
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS USE_GLES3)
else ()
    message(STATUS "USE_GL3/4")
endif ()

# SDL or GLFW
if (USE_SDL2)
    message(STATUS "SDL2")
    set(USE_SDL2 ON)
    set(USE_GLFW OFF)
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS __SDL2__)
elseif (USE_GLFW)
    message(STATUS "GLFW")
    set(USE_SDL2 OFF)
    set(USE_GLFW ON)
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS __GLFW__)
else ()
    message(STATUS "Neither SDL2 nor GLFW: the platform has its own window and input")
endif ()

if (USE_EGL)
    message(STATUS "Using EGL")
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS USE_EGL)
elseif (NOT USE_D3D11)
    message(STATUS "Using native GL")
endif ()

if (SIMPLE_HIGHLIGHT)
    message(STATUS "Enable SIMPLE_HIGHLIGHT")
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS SIMPLE_HIGHLIGHT)
endif ()

if (USE_STD_THREAD)
    message(STATUS "Enable std thread")
    list(APPEND AURORE_TOOLCHAIN_DEFINITIONS AURORE_USE_STD_THREAD)
endif ()

function(program_target target source)
    if (WIN32 AND NOT WIN32_TERMINAL)
        add_executable(${target} WIN32 ${source})
    elseif (PLATFORM_ANDROID)
        add_library(${target} SHARED)
        target_sources(${target} PRIVATE ${source})
    else ()
        add_executable(${target} ${source})
    endif ()
endfunction()

# Links an app without the functions and data of the library it does not use
# (the library is built with -ffunction-sections -fdata-sections). Linux and
# macOS; the consoles' toolchains do it themselves.
function(aurore_app_link_options target)
    if (APPLE)
        target_link_options(${target} PRIVATE -Wl,-dead_strip)
    elseif (NOT WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(${target} PRIVATE -Wl,--gc-sections)
    endif ()
endfunction()

function(ios_bundle iosStoryBoard assets plist name version)
    set(IOS_CODE_SIGN_IDENTITY "" CACHE STRING "The code sign identity to use when building the IPA.")
    set(IOS_GUI_IDENTIFIER "" CACHE STRING "The package name")
    if(IOS_CODE_SIGN_IDENTITY STREQUAL "")
        set(IOS_CODE_SIGNING_ENABLED NO)
    else()
        set(IOS_CODE_SIGNING_ENABLED YES)
    endif()
    if (IOS_GUI_IDENTIFIER STREQUAL "")
        set(IOS_GUI_IDENTIFIER "${PACKAGE_NAME}")
        message(WARNING "Using default package name: ${PACKAGE_NAME}")
    endif ()
    set(IOS_SPLASH_STORYBOARD "${iosStoryBoard}")
    set(IOS_ASSETS ${assets})
    set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${IOS_CODE_SIGN_IDENTITY}"
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "${IOS_CODE_SIGNING_ENABLED}"
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "${IOS_CODE_SIGNING_ENABLED}"
            BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${plist}
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER ${IOS_GUI_IDENTIFIER}
            MACOSX_BUNDLE_BUNDLE_NAME ${name}
            MACOSX_BUNDLE_BUNDLE_VERSION "${version}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${version}"
            XCODE_ATTRIBUTE_ENABLE_BITCODE NO
            XCODE_ATTRIBUTE_SKIP_INSTALL NO
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
            )
    set_property(
            SOURCE ${IOS_ASSETS}
            PROPERTY MACOSX_PACKAGE_LOCATION "Resources"
    )
    set_property(
            SOURCE ${IOS_SPLASH_STORYBOARD}
            PROPERTY MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE
            ${IOS_ASSETS}
            ${IOS_SPLASH_STORYBOARD}
            )
endfunction()

function(linux_install res res_install svg_icon icon_res_dir icon_res_list appdata desktop)
    message("install prefix: ${CMAKE_INSTALL_PREFIX}")
    # bin
    install(TARGETS ${PROJECT_NAME}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
    if (NOT DEFINED CMAKE_INSTALL_SHARE_DIR)
        set(CMAKE_INSTALL_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share)
    endif ()
    # resources
    install(DIRECTORY ${res}
            DESTINATION ${res_install}
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    # svg icon
    install(FILES ${svg_icon}
            DESTINATION ${CMAKE_INSTALL_SHARE_DIR}/icons/hicolor/scalable/apps
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    # png icon
    foreach (icon_dir ${icon_res_list})
        install(FILES ${icon_res_dir}/${icon_dir}/${PACKAGE_NAME}.png
                DESTINATION ${CMAKE_INSTALL_SHARE_DIR}/icons/hicolor/${icon_dir}/apps
                PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    endforeach ()
    # appdata
    install(FILES ${appdata}
            DESTINATION ${CMAKE_INSTALL_SHARE_DIR}/metainfo
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    # desktop
    configure_file("${desktop}" "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.desktop")
    install(FILES "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.desktop" DESTINATION ${CMAKE_INSTALL_SHARE_DIR}/applications)
endfunction()

function(add_libromfs app res)
    message(STATUS "USE LIBROMFS")
    set(LIBROMFS_PROJECT_NAME ${app} PARENT_SCOPE)
    set(LIBROMFS_RESOURCE_LOCATION "${res}" PARENT_SCOPE)
endfunction()

function(git_info tag short)
    # Add git info
    find_package(Git)
    if(GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --tags
                TIMEOUT 5
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_TAG_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE
                )
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                TIMEOUT 5
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_TAG_SHORT
                OUTPUT_STRIP_TRAILING_WHITESPACE
                )
        message(STATUS "building from git tag: ${GIT_TAG_VERSION}")
        message(STATUS "building from git commit: ${GIT_TAG_SHORT}")
    else ()
        set(GIT_TAG_VERSION "")
        set(GIT_TAG_SHORT "")
    endif ()
    set(${tag} ${GIT_TAG_VERSION} PARENT_SCOPE)
    set(${short} ${GIT_TAG_SHORT} PARENT_SCOPE)
endfunction()

# switch platform
# glsl -> deko3d dksh, in romfs_dir. The library builds these shaders in
# (library/CMakeLists.txt): an app needs this only to ship its own copies.
function(gen_dksh romfs_dir)
    find_program(NX_UAM_EXE NAMES uam HINTS "${DEVKITPRO}/tools/bin")
    if (NOT NX_UAM_EXE)
        message(FATAL_ERROR "Could not find uam: try installing uam")
    endif()
    if (NOT DEFINED AURORE_LIBRARY)
        message(FATAL_ERROR AURORE_LIBRARY is not defined)
    endif ()
    set(SHADER_BASE "${AURORE_LIBRARY}/lib/extern/nanovg/deko3d/shaders")

    file(MAKE_DIRECTORY "${romfs_dir}")
    execute_process(COMMAND ${NX_UAM_EXE} -s vert -o "${romfs_dir}/fill_vsh.dksh" "${SHADER_BASE}/fill_vsh.glsl" TIMEOUT 5)
    execute_process(COMMAND ${NX_UAM_EXE} -s frag -o "${romfs_dir}/fill_aa_fsh.dksh" "${SHADER_BASE}/fill_aa_fsh.glsl" TIMEOUT 5)
    execute_process(COMMAND ${NX_UAM_EXE} -s frag -o "${romfs_dir}/fill_fsh.dksh" "${SHADER_BASE}/fill_fsh.glsl" TIMEOUT 5)
endfunction()
# ps4 platform
# The pacbrew toolchain's add_pkg, with the gp4's directory tree rewritten
# before PkgTool reads it (see ps4_gp4_rootdir.cmake).
function(ps4_pkg project pkgdir title_id title version)
    if (NOT DEFINED AURORE_LIBRARY)
        message(FATAL_ERROR AURORE_LIBRARY is not defined)
    endif ()
    # Title must not exceed 128 characters
    string(SUBSTRING "${title}" 0 127 title)

    # Version in a form the PS4 accepts
    string(SUBSTRING "${version}" 0 7 verclean)
    string(REGEX MATCH "([0-9]+\\.[0-9]+)" verclean ${verclean})
    if ("${verclean}" STREQUAL "")
        message(WARNING "The version string '${version}' is formatted in a way that is incompatable with the PS4, using '01.00'")
        set(verclean "01.00")
    endif ()

    # Content id from the title id and the version
    string(REPLACE "." "0" vercont ${verclean})
    string(APPEND vercont "00000000")
    string(SUBSTRING "${vercont}" 0 7 vercont)
    set(content_id "IV0001-${title_id}_00-${title_id}${vercont}")
    set(PKG_OUT_NAME "${content_id}.pkg" CACHE STRING "ps4 pkg name" FORCE)

    set(attribute 0)
    if (${ARGC} GREATER 5)
        set(attribute ${ARGV5})
    endif ()
    set(category "gde")
    if (${ARGC} GREATER 6)
        set(category "${ARGV6}")
    endif ()

    set(PKGTOOL ${CMAKE_COMMAND} -E env DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1 "${OPENORBIS}/bin/linux/PkgTool.Core")
    set(SFO ${pkgdir}/sce_sys/param.sfo)
    add_custom_command(
            OUTPUT "${project}.pkg"
            COMMAND ${CMAKE_COMMAND} -E copy eboot.bin ${pkgdir}/eboot.bin
            COMMAND ${PKGTOOL} sfo_new ${SFO}
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} APP_TYPE --type Integer --maxsize 4 --value 1
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} APP_VER --type Utf8 --maxsize 8 --value "${verclean}"
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} ATTRIBUTE --type Integer --maxsize 4 --value ${attribute}
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} CATEGORY --type Utf8 --maxsize 4 --value "${category}"
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} FORMAT --type Utf8 --maxsize 4 --value "obs"
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} CONTENT_ID --type Utf8 --maxsize 48 --value "${content_id}"
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} SYSTEM_VER --type Integer --maxsize 4 --value 1020
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} TITLE --type Utf8 --maxsize 128 --value "${title}"
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} TITLE_ID --type Utf8 --maxsize 12 --value "${title_id}"
            COMMAND ${PKGTOOL} sfo_setentry ${SFO} VERSION --type Utf8 --maxsize 8 --value "${verclean}"
            COMMAND "${OPENORBIS}/bin/linux/create-gp4" -out ${pkgdir}/${project}.gp4 --content-id "${content_id}" --path "${pkgdir}"
            COMMAND ${CMAKE_COMMAND} -DGP4=${pkgdir}/${project}.gp4 -P ${AURORE_LIBRARY}/cmake/ps4_gp4_rootdir.cmake
            COMMAND ${PKGTOOL} pkg_build ${pkgdir}/${project}.gp4 ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E remove ${pkgdir}/eboot.bin ${SFO} ${pkgdir}/${project}.gp4
            VERBATIM
            DEPENDS "${project}.self"
    )
    add_custom_target("${project}_pkg" ALL DEPENDS "${project}.pkg")
endfunction()
