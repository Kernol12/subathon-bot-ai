# SPDX-License-Identifier: GPL-3.0-or-later
add_executable(native_websocket_probe native_ws_probe.cpp)
target_link_libraries(native_websocket_probe PRIVATE
    subathon_native_websocket_profile Drogon::Drogon Threads::Threads
    OpenSSL::Crypto ${CMAKE_DL_LIBS})
subathon_target_options(native_websocket_probe)
set_target_properties(native_websocket_probe PROPERTIES
    BUILD_RPATH "${sbt_drogon_root}/lib")

# Drogon's own library has no RUNPATH. Bind transitive loading for this selected
# fixture as well; a probe RUNPATH alone does not bind its dependencies' lookup.
add_test(NAME native-websocket-profile.suite
    COMMAND "${CMAKE_COMMAND}" -E env
        "LD_LIBRARY_PATH=${sbt_drogon_root}/lib"
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_native_ws_profile.py"
        --probe "$<TARGET_FILE:native_websocket_probe>" -v)
# Authored maximum: 102 child scenarios, each 10s plus two bounded 10s cleanup
# attempts, then 60s runner margin. This is a failure watchdog, not OS preemption.
set_tests_properties(native-websocket-profile.suite PROPERTIES
    LABELS "native-websocket-profile;acceptance;intentional-red;server;loopback"
    TIMEOUT 3120)
