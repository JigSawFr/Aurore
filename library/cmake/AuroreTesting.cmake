# brls_add_tests(), for the tests of an app on the test harness (Aurore::test,
# docs/testing.md). Included by the library when BRLS_BUILD_TESTS is on, and
# by the installed package along with Aurore::test.

# Registers each case of a test executable (brls::test::runTests) with
# ctest, read from `<target> --list` when ctest runs. REFERENCES: the
# folder of the reference screenshots; what the tests get goes to
# <build dir>/test-output.
function(brls_add_tests target)
    cmake_parse_arguments(ARG "" "REFERENCES" "" ${ARGN})
    set(script "${CMAKE_CURRENT_BINARY_DIR}/${target}_cases.cmake")
    set(env "BRLS_TEST_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-output")
    if (ARG_REFERENCES)
        set(env "${env};BRLS_TEST_REFERENCES=${ARG_REFERENCES}")
    endif ()
    file(GENERATE OUTPUT "${script}" CONTENT "
execute_process(COMMAND \"$<TARGET_FILE:${target}>\" --list OUTPUT_VARIABLE cases RESULT_VARIABLE result)
if (NOT result EQUAL 0)
add_test(${target}_not_built \"$<TARGET_FILE:${target}>\" --list)
return()
endif ()
# The names only, not what the program logged
string(REGEX MATCHALL \"brls-test: [A-Za-z0-9_]+\" cases \"\${cases}\")
list(TRANSFORM cases REPLACE \"^brls-test: \" \"\")
foreach (case IN LISTS cases)
if (case)
    add_test(${target}.\${case} \"$<TARGET_FILE:${target}>\" \${case})
    set_tests_properties(${target}.\${case} PROPERTIES
        WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\"
        ENVIRONMENT \"${env}\"
        TIMEOUT 300)
endif ()
endforeach ()
")
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${script}")
endfunction()
