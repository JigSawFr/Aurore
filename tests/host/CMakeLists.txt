# Host suites: Cadran's plain C++ parts (palettes, Markdown, text helpers),
# each built without a window and linked only to the sources it tests.
# ctest -R host runs them.
set(CADRAN_SRC ${CMAKE_SOURCE_DIR}/library/lib/cadran)

function(aurore_host_test name)
    add_executable(aurore_host_${name} ${ARGN})
    target_include_directories(aurore_host_${name} PRIVATE ${CMAKE_SOURCE_DIR}/library/include ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(aurore_host_${name} PRIVATE cxx_std_20)
    target_compile_definitions(aurore_host_${name} PRIVATE AURORE_SOURCE_RESOURCES="${CMAKE_SOURCE_DIR}/resources/")
    add_test(NAME host_${name} COMMAND aurore_host_${name})
endfunction()

# Every ink at 4.5:1 on its surfaces, the focus ring at 3:1 (WCAG)
aurore_host_test(contrast contrast_tests.cpp)

# Text from GitHub cleaned, and GitHub-flavoured Markdown as blocks
aurore_host_test(markdown markdown_tests.cpp ${CADRAN_SRC}/util/safe_text.cpp ${CADRAN_SRC}/util/markdown.cpp)

# ui::guarded: a callback that does nothing once its screen is gone
aurore_host_test(guarded guarded_tests.cpp)

# The text helpers over Cadran's real catalogs (ui/tr.hpp without borealis)
aurore_host_test(cadran_text cadran_text_tests.cpp ${CADRAN_SRC}/ui/cadran_text.cpp)
aurore_host_test(format_text format_text_tests.cpp ${CADRAN_SRC}/ui/format_text.cpp)
foreach (name cadran_text format_text)
    target_compile_definitions(aurore_host_${name} PRIVATE BRLS_CADRAN_HOST_I18N)
    target_link_libraries(aurore_host_${name} PRIVATE fmt::fmt)
endforeach ()
