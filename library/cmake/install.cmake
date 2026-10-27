# Installs the library for other CMake projects (AURORE_INSTALL=ON):
#
#   find_package(Aurore REQUIRED)
#   target_link_libraries(app PRIVATE Aurore::aurore)
#
# Included from library/CMakeLists.txt once the aurore target exists.
# Desktop only: the consoles build the library with the app.

if (NOT PLATFORM_DESKTOP)
    message(FATAL_ERROR "AURORE_INSTALL is for the desktop build (PLATFORM_DESKTOP=ON)")
endif ()
if (USE_LIBROMFS)
    message(FATAL_ERROR "AURORE_INSTALL needs USE_LIBROMFS=OFF: libromfs embeds one app's resources")
endif ()

include(CMakePackageConfigHelpers)

set(AURORE_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/Aurore)
set(AURORE_EXPORTED aurore yogacore)
if (NOT USE_SYSTEM_TINYXML2 AND NOT AURORE_FETCH_DEPS)
    list(APPEND AURORE_EXPORTED tinyxml2)
endif ()

install(TARGETS ${AURORE_EXPORTED} EXPORT AuroreTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY ${BOREALIS_PATH}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# yoga's headers, where its target looks for them (<prefix>/include/yoga)
install(DIRECTORY ${BOREALIS_PATH}/lib/extern/yoga/yoga DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/yoga
        FILES_MATCHING PATTERN "*.h")
install(EXPORT AuroreTargets NAMESPACE Aurore:: DESTINATION ${AURORE_CMAKE_DIR})

# What AuroreConfig.cmake looks for before it loads the targets: the
# vendored fmt, tweeny, GLFW and SDL2 install their own packages
set(AURORE_DEPENDENCIES "find_dependency(Threads)\nfind_dependency(fmt)\nfind_dependency(Tweeny)\n")
if (USE_SYSTEM_TINYXML2 OR AURORE_FETCH_DEPS)
    string(APPEND AURORE_DEPENDENCIES "find_dependency(tinyxml2)\n")
endif ()
if (USE_BOOST_FILESYSTEM)
    string(APPEND AURORE_DEPENDENCIES "find_dependency(Boost 1.44 COMPONENTS filesystem)\n")
endif ()
if (USE_SDL2)
    string(APPEND AURORE_DEPENDENCIES "find_dependency(SDL2)\n")
else ()
    string(APPEND AURORE_DEPENDENCIES "find_dependency(glfw3)\n")
endif ()

configure_package_config_file(${BOREALIS_PATH}/cmake/AuroreConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/AuroreConfig.cmake
        INSTALL_DESTINATION ${AURORE_CMAKE_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AuroreConfig.cmake DESTINATION ${AURORE_CMAKE_DIR})
