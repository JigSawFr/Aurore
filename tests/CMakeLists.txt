# The demo's views, without its main(): what the tests, the bench and
# aurore-preview show
file(GLOB_RECURSE DEMO_VIEWS_SRC CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/demo/src/*.cpp)
list(REMOVE_ITEM DEMO_VIEWS_SRC ${PROJECT_SOURCE_DIR}/demo/src/main.cpp)
add_library(aurore_demo_views STATIC ${DEMO_VIEWS_SRC})
target_include_directories(aurore_demo_views PUBLIC ${PROJECT_SOURCE_DIR}/demo/include ${PROJECT_SOURCE_DIR}/demo)
target_link_libraries(aurore_demo_views PUBLIC Aurore::aurore)

# Test cases over the demo (ctest runs each in a process of its own)
add_executable(aurore_tests demo_tests.cpp)
target_link_libraries(aurore_tests PRIVATE aurore_demo_views Aurore::test)
aurore_add_tests(aurore_tests REFERENCES ${CMAKE_CURRENT_SOURCE_DIR}/references)

# openBrowser(): accepted links, no shell between the link and the opener
add_executable(aurore_open_url_tests open_url_tests.cpp)
target_link_libraries(aurore_open_url_tests PRIVATE Aurore::test)
aurore_add_tests(aurore_open_url_tests)

# Pictures too large are refused before they are decoded
add_executable(aurore_image_limits_tests image_limits_tests.cpp)
target_link_libraries(aurore_image_limits_tests PRIVATE Aurore::test)
target_compile_definitions(aurore_image_limits_tests PRIVATE AURORE_SOURCE_RESOURCES="${PROJECT_SOURCE_DIR}/resources/")
aurore_add_tests(aurore_image_limits_tests)

# XML layouts: bounds, errors
add_executable(aurore_parser_tests parser_tests.cpp)
target_link_libraries(aurore_parser_tests PRIVATE Aurore::test)
aurore_add_tests(aurore_parser_tests)

# The density: the style's lengths, dp in XML
add_executable(aurore_density_tests density_tests.cpp)
target_link_libraries(aurore_density_tests PRIVATE Aurore::test)
aurore_add_tests(aurore_density_tests)

# The SDL2 driver's input, on a virtual controller
if (USE_SDL2 AND PLATFORM_DESKTOP)
    add_executable(aurore_sdl_input_tests sdl_input_tests.cpp)
    target_link_libraries(aurore_sdl_input_tests PRIVATE Aurore::test)
    aurore_add_tests(aurore_sdl_input_tests)
endif ()

# libFuzzer targets, with clang: -DAURORE_BUILD_FUZZERS=ON (docs/testing.md)
option(AURORE_BUILD_FUZZERS "Build the libFuzzer targets of tests/fuzz (clang)" OFF)
if (AURORE_BUILD_FUZZERS)
    set(FUZZ_FLAGS -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
    add_executable(aurore_fuzz_markdown fuzz/markdown_fuzz.cpp
            ${PROJECT_SOURCE_DIR}/library/lib/cadran/util/markdown.cpp
            ${PROJECT_SOURCE_DIR}/library/lib/cadran/util/safe_text.cpp)
    target_include_directories(aurore_fuzz_markdown PRIVATE ${PROJECT_SOURCE_DIR}/library/include)
    target_compile_features(aurore_fuzz_markdown PRIVATE cxx_std_20)
    target_compile_options(aurore_fuzz_markdown PRIVATE ${FUZZ_FLAGS})
    target_link_options(aurore_fuzz_markdown PRIVATE ${FUZZ_FLAGS})
endif ()

# Tasks coming back to a view from another thread, task queues, events
add_executable(aurore_async_tests async_tests.cpp)
target_link_libraries(aurore_async_tests PRIVATE Aurore::test)
target_compile_definitions(aurore_async_tests PRIVATE AURORE_SOURCE_RESOURCES="${PROJECT_SOURCE_DIR}/resources/")
aurore_add_tests(aurore_async_tests)

# Every translation catalog parses and is loaded
add_executable(aurore_i18n_tests i18n_tests.cpp)
target_link_libraries(aurore_i18n_tests PRIVATE Aurore::test)
target_compile_definitions(aurore_i18n_tests PRIVATE AURORE_SOURCE_RESOURCES="${PROJECT_SOURCE_DIR}/resources/")
aurore_add_tests(aurore_i18n_tests)

# The debugging aids' environment variables, only in a build with AURORE_DEV_TOOLS
add_executable(aurore_dev_tools_tests dev_tools_tests.cpp)
target_link_libraries(aurore_dev_tools_tests PRIVATE Aurore::test)
target_compile_definitions(aurore_dev_tools_tests PRIVATE ${AURORE_DEV_TOOLS_DEFINITION})
aurore_add_tests(aurore_dev_tools_tests)


# The main loop's cost on the demo (Profiler), checked against a baseline
add_executable(aurore_bench bench.cpp)
target_link_libraries(aurore_bench PRIVATE aurore_demo_views Aurore::test)
add_test(NAME aurore_bench COMMAND aurore_bench --check ${CMAKE_CURRENT_SOURCE_DIR}/bench_baseline.json)

# An XML layout drawn to a PNG
add_executable(aurore-preview preview.cpp)
target_link_libraries(aurore-preview PRIVATE aurore_demo_views Aurore::test)
add_test(NAME aurore_preview
        COMMAND aurore-preview ${PROJECT_SOURCE_DIR}/resources/xml/tabs/layout.xml --theme light --lang fr --size 960x540
                -o ${CMAKE_CURRENT_BINARY_DIR}/test-output/preview_layout.png)
set_tests_properties(aurore_bench aurore_preview PROPERTIES TIMEOUT 300)

# docs/xml-attributes.md as the sources register the attributes
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME xml_attributes_doc COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/xml_attributes.py --check)
endif()

# Cadran's plain C++ parts, without a window
add_subdirectory(host)

# The demo's screens drawn to PNGs, to look at them in a language, a theme
# and a size at once
add_executable(aurore-tour tour.cpp)
target_link_libraries(aurore-tour PRIVATE aurore_demo_views Aurore::test)
