# The New 3DS, before its renderer: a smoke test that links the library and
# runs on the console (docs/platforms.md). Built by the root CMakeLists.txt
# with -DAURORE_PLATFORM=3DS instead of the demo.

add_executable(aurore_3ds_smoke smoke.cpp)
set_target_properties(aurore_3ds_smoke PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_SCAN_FOR_MODULES OFF)
target_link_libraries(aurore_3ds_smoke PRIVATE aurore)

ctr_generate_smdh(aurore_3ds_smoke.smdh
        NAME "Aurore smoke test"
        DESCRIPTION "Aurore on the New 3DS, before its renderer"
        AUTHOR "JigSawFr")
ctr_create_3dsx(aurore_3ds_smoke SMDH aurore_3ds_smoke.smdh)
