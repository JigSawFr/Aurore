# A project that uses an installed Aurore (AURORE_INSTALL=ON), as a packager's
# user would: CI installs the library, moves the prefix elsewhere and builds
# this against it (tools/ci_variant.sh install).
#
#   cmake -S tests/consumer -B build-consumer -DCMAKE_PREFIX_PATH=<prefix>
cmake_minimum_required(VERSION 3.16...3.31)
project(aurore_consumer CXX)

find_package(Aurore 0.1 REQUIRED)

add_executable(aurore_consumer main.cpp)
target_link_libraries(aurore_consumer PRIVATE Aurore::aurore)

# The harness, installed along when the library was built with AURORE_BUILD_TESTS
if (TARGET Aurore::test)
    enable_testing()
    add_executable(aurore_consumer_tests consumer_tests.cpp)
    target_link_libraries(aurore_consumer_tests PRIVATE Aurore::test)
    aurore_add_tests(aurore_consumer_tests)
endif ()
