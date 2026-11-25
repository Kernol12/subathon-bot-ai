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

subathon_add_qt_suite(server_storage_003_w1_open_admission
    NAME storage-003-w1-open-admission SOURCES w1_open_admission_tests.cpp
    LIBRARIES subathon_storage_003_fixture_owner subathon_contribution_review
    LABELS "storage-003-w1-open-admission;acceptance;qt;server;intentional-red" TIMEOUT 240)

subathon_add_qt_suite(server_storage_003_w1_admission_supplement
    NAME storage-003-w1-admission-supplement SOURCES w1_admission_supplement_tests.cpp
    LIBRARIES subathon_storage_003_fixture_owner subathon_contribution_review
    LABELS "storage-003-w1-admission-supplement;acceptance;qt;server;intentional-red" TIMEOUT 240)

# Compile the same assertions into a complete runner for the implemented scope.
# The full runner retains w1RuntimeRecipeDenials for its later stage.
subathon_add_qt_suite(server_storage_003_w1_writer
    NAME storage-003-w1-writer SOURCES writer_tests.cpp
    LIBRARIES subathon_storage_003_fixture_owner subathon_contribution_review
    LABELS "storage-003-w1-writer;acceptance;qt;server" TIMEOUT 240)
target_compile_definitions(server_storage_003_w1_writer PRIVATE
    SUBATHON_STORAGE_003_IMPLEMENTED_W1)

# #834 W2–W4 runner, default OFF under the Stage 1 gate. It links exactly one
# checkpoint owner variant, which also carries the test-only owner arm entries.
# The fixture service objects define every symbol of the ordinary service
# archive, so the linker never extracts an ordinary service member here.
option(SUBATHON_BUILD_STORAGE_003_W2_W5 "Build the reviewed STORAGE-032 W2–W4 runner" OFF)
if(SUBATHON_BUILD_STORAGE_003_W2_W5)
    add_library(subathon_storage_003_w2_w4_owner OBJECT
        "${PROJECT_SOURCE_DIR}/server/src/sqlite_clock_checkpoint.cpp"
        w2_w4_owner_arm_stub.cpp)
    target_compile_definitions(subathon_storage_003_w2_w4_owner PRIVATE
        SUBATHON_STORAGE_003_CONSTRAINT_TEST_ACCESS SUBATHON_STORAGE_003_W2_W5_TEST_ACCESS)
    target_link_libraries(subathon_storage_003_w2_w4_owner PRIVATE
        subathon_sqlite_application_bundle)
    subathon_target_options(subathon_storage_003_w2_w4_owner)

    add_library(subathon_storage_003_w2_w4_service OBJECT
        "${PROJECT_SOURCE_DIR}/server/src/application_service.cpp"
        "${PROJECT_SOURCE_DIR}/server/src/application_service_command.cpp")
    target_compile_definitions(subathon_storage_003_w2_w4_service PRIVATE
        SUBATHON_STORAGE_003_W2_W5_TEST_ACCESS)
    target_link_libraries(subathon_storage_003_w2_w4_service PRIVATE
        subathon_sqlite_application_holds subathon_exact_numeric subathon_time_debt
        subathon_timer_lifecycle Qt6::Core)
    subathon_target_options(subathon_storage_003_w2_w4_service)

    subathon_add_qt_suite(server_storage_003_w2_w4
        NAME storage-003-w2-w4 SOURCES w2_w4_tests.cpp
        LIBRARIES subathon_storage_003_w2_w4_owner subathon_storage_003_w2_w4_service
            subathon_contribution_review
        LABELS "storage-003-w2-w4;acceptance;qt;server;intentional-red" TIMEOUT 240)
endif()
