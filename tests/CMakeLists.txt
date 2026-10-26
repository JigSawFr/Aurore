# The demo's views, without its main(): what the tests show
file(GLOB_RECURSE DEMO_VIEWS_SRC ${CMAKE_SOURCE_DIR}/demo/src/*.cpp)
list(REMOVE_ITEM DEMO_VIEWS_SRC ${CMAKE_SOURCE_DIR}/demo/src/main.cpp)
add_library(aurore_demo_views STATIC ${DEMO_VIEWS_SRC})
target_include_directories(aurore_demo_views PUBLIC ${CMAKE_SOURCE_DIR}/demo/include ${CMAKE_SOURCE_DIR}/demo)
target_link_libraries(aurore_demo_views PUBLIC borealis)

# Test cases over the demo (ctest runs each in a process of its own)
add_executable(aurore_tests demo_tests.cpp)
target_link_libraries(aurore_tests PRIVATE aurore_demo_views borealis_test)
brls_add_tests(aurore_tests REFERENCES ${CMAKE_CURRENT_SOURCE_DIR}/references)
