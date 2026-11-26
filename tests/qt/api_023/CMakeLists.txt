# SPDX-License-Identifier: GPL-3.0-or-later
add_executable(server_api_023_host_consumer host_fatal_consumer_tests.cpp)
set_target_properties(server_api_023_host_consumer PROPERTIES AUTOMOC ON)
target_link_libraries(server_api_023_host_consumer PRIVATE
    subathon_api_host Qt6::Test subathon_test_fixtures)
subathon_target_options(server_api_023_host_consumer)

# One selector and one Host per process, matching the existing Host policy.
foreach(host_case IN ITEMS
        ordinary_stop_control
        fatal_stop_order:fatal_first
        fatal_stop_order:ordinary_first_sink_refuses
        published_before_queued_host_readiness
        accepted_units_drain_and_later_command_stays_out)
    add_test(NAME "api-023-host-consumer.${host_case}"
        COMMAND server_api_023_host_consumer "${host_case}")
    set_tests_properties("api-023-host-consumer.${host_case}" PROPERTIES
        LABELS "api-023-host-consumer;acceptance;qt;server"
        TIMEOUT 180 ENVIRONMENT "LD_LIBRARY_PATH=${sbt_drogon_root}/lib")
endforeach()
