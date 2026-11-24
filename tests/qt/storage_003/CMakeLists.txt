# SPDX-License-Identifier: GPL-3.0-or-later

# The protected base has no ordinary checkpoint owner object. Each structural
# runner receives exactly this variant once; do not also link an ordinary copy.
add_library(subathon_storage_003_fixture_owner OBJECT
    "${PROJECT_SOURCE_DIR}/server/src/sqlite_clock_checkpoint.cpp")
target_compile_definitions(subathon_storage_003_fixture_owner PRIVATE
    SUBATHON_STORAGE_003_CONSTRAINT_TEST_ACCESS)
target_link_libraries(subathon_storage_003_fixture_owner PRIVATE
    subathon_sqlite_application_bundle)
subathon_target_options(subathon_storage_003_fixture_owner)

foreach(case catalog writer)
    subathon_add_qt_suite(server_storage_003_stage1_${case}
        NAME storage-003-${case} SOURCES ${case}_tests.cpp
        LIBRARIES subathon_storage_003_fixture_owner subathon_contribution_review
        LABELS "storage-003;acceptance;qt;server;intentional-red" TIMEOUT 240)
endforeach()
