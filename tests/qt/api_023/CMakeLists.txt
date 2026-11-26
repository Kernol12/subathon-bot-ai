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

# Stage 3 selectors stay out of the required label until the implementation
# lands (AGENT_WORKFLOW: no required check runs reviewed tests red).
foreach(host_case IN ITEMS
        lease_take_under_fatal:fatal
        lease_take_under_fatal:ordinary
        post_failure_is_contained:stop_refuse_fatal
        post_failure_is_contained:stop_raise_fatal
        post_failure_is_contained:stop_refuse_ordinary
        post_failure_is_contained:disconnect_raise_fatal
        post_failure_is_contained:disconnect_refuse_ordinary
        post_failure_is_contained:lease_raise_fatal
        post_failure_is_contained:lease_refuse_ordinary
        post_failure_is_contained:stop_raise_ordinary
        post_failure_is_contained:disconnect_raise_ordinary
        post_failure_is_contained:lease_raise_ordinary
        cold_stop_without_start:refuse
        cold_stop_without_start:raise
        stop_before_writer_open:refuse
        stop_before_writer_open:raise
        timer_registration_failure_refuses_start
        deferred_bootstrap:stop_latched
        deferred_bootstrap:stop_latched_raise
        deferred_bootstrap:arms_once
        deferred_bootstrap:start_before_bootstrap)
    add_test(NAME "api-023-host-consumer.${host_case}"
        COMMAND server_api_023_host_consumer "${host_case}")
    set_tests_properties("api-023-host-consumer.${host_case}" PROPERTIES
        LABELS "api-023-host-consumer-stage3;acceptance;qt;server;intentional-red"
        TIMEOUT 180 ENVIRONMENT "LD_LIBRARY_PATH=${sbt_drogon_root}/lib")
endforeach()
