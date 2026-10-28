# The demo's views, without its main(): what the tests, the bench and
# aurore-preview show
file(GLOB_RECURSE DEMO_VIEWS_SRC ${CMAKE_SOURCE_DIR}/demo/src/*.cpp)
list(REMOVE_ITEM DEMO_VIEWS_SRC ${CMAKE_SOURCE_DIR}/demo/src/main.cpp)
add_library(aurore_demo_views STATIC ${DEMO_VIEWS_SRC})
target_include_directories(aurore_demo_views PUBLIC ${CMAKE_SOURCE_DIR}/demo/include ${CMAKE_SOURCE_DIR}/demo)
target_link_libraries(aurore_demo_views PUBLIC Aurore::aurore)

# Test cases over the demo (ctest runs each in a process of its own)
add_executable(aurore_tests demo_tests.cpp)
target_link_libraries(aurore_tests PRIVATE aurore_demo_views Aurore::test)
brls_add_tests(aurore_tests REFERENCES ${CMAKE_CURRENT_SOURCE_DIR}/references)

# openBrowser(): accepted links, no shell between the link and the opener
add_executable(aurore_open_url_tests open_url_tests.cpp)
target_link_libraries(aurore_open_url_tests PRIVATE Aurore::test)
brls_add_tests(aurore_open_url_tests)

# Pictures too large are refused before they are decoded
add_executable(aurore_image_limits_tests image_limits_tests.cpp)
target_link_libraries(aurore_image_limits_tests PRIVATE Aurore::test)
target_compile_definitions(aurore_image_limits_tests PRIVATE AURORE_SOURCE_RESOURCES="${CMAKE_SOURCE_DIR}/resources/")
brls_add_tests(aurore_image_limits_tests)

# The main loop's cost on the demo (Profiler), checked against a baseline
add_executable(aurore_bench bench.cpp)
target_link_libraries(aurore_bench PRIVATE aurore_demo_views Aurore::test)
add_test(NAME aurore_bench COMMAND aurore_bench --check ${CMAKE_CURRENT_SOURCE_DIR}/bench_baseline.json)

# An XML layout drawn to a PNG
add_executable(aurore-preview preview.cpp)
target_link_libraries(aurore-preview PRIVATE aurore_demo_views Aurore::test)
add_test(NAME aurore_preview
        COMMAND aurore-preview ${CMAKE_SOURCE_DIR}/resources/xml/tabs/layout.xml --theme light --lang fr --size 960x540
                -o ${CMAKE_CURRENT_BINARY_DIR}/test-output/preview_layout.png)
set_tests_properties(aurore_bench aurore_preview PROPERTIES TIMEOUT 300)

# Cadran's plain C++ parts, without a window
add_subdirectory(host)
