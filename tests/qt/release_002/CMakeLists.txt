# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.25)
project(Release002Stage1 LANGUAGES CXX)

include(CTest)
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Sql Test)
# Import only the existing supplier libraries. The parent project's acceptance
# selectors (including "all") never select this standalone project.
function(import_suppliers)
    set(BUILD_TESTING OFF)
    add_subdirectory(../../.. suppliers EXCLUDE_FROM_ALL)
endfunction()
import_suppliers()

add_executable(release_002_stage1 release_002_tests.cpp stage1_stub.cpp)
set_target_properties(release_002_stage1 PROPERTIES AUTOMOC ON CXX_EXTENSIONS OFF)
target_compile_definitions(release_002_stage1 PRIVATE QT_NO_KEYWORDS)
target_link_libraries(release_002_stage1 PRIVATE
    Qt6::Test
    subathon_diagnostics)
subathon_target_options(release_002_stage1)

if(BUILD_TESTING)
    add_test(NAME release-002.acceptance COMMAND release_002_stage1
        A A_unadapted E1 E2 E2_empty E3_denied E3_invalid_duplicate_attempt
        E3_invalid_duplicate_key E4_revoke
        E4_expiry E4_after_transfer E5 E5_unlink_refused E6_cutoff E6_clock
        E7 E8_capacity E8_uncertain_final_read E9 E10 E11 E12_hold
        E12_pending E12_restart E13 E14)
    set_tests_properties(release-002.acceptance PROPERTIES LABELS "release-002;acceptance;qt;server"
        TIMEOUT 180)
    add_test(NAME release-002.producers COMMAND release_002_stage1 producer_controls)
    set_tests_properties(release-002.producers PROPERTIES LABELS "release-002;acceptance;qt;server;producer-control"
        TIMEOUT 30)
endif()
