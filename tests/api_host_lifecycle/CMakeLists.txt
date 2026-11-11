# SPDX-License-Identifier: GPL-3.0-or-later
add_executable(api_host_probe api_host_probe.cpp)
target_include_directories(api_host_probe PRIVATE "${PROJECT_SOURCE_DIR}/tests/qt")
target_link_libraries(api_host_probe PRIVATE subathon_api_host_drogon
    Qt6::Test Qt6::Sql subathon_test_fixtures ${CMAKE_DL_LIBS})
subathon_target_options(api_host_probe)
set_target_properties(api_host_probe PROPERTIES BUILD_RPATH "${sbt_drogon_root}/lib")

# Receipt production is a separate reviewed, passive-ELF then controlled-identity
# gate. Compilation/discovery needs no receipt and never runs this red baseline.
set(SUBATHON_API_HOST_ADMISSION "${CMAKE_BINARY_DIR}/api-host-admission.json"
    CACHE FILEPATH "New API host consumer's independently checked format-1 receipt")
add_test(NAME api-host-native.suite
    COMMAND "${CMAKE_COMMAND}" -E env "LD_LIBRARY_PATH=${sbt_drogon_root}/lib"
        "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_api_host_lifecycle.py"
        --probe "$<TARGET_FILE:api_host_probe>" --admission "${SUBATHON_API_HOST_ADMISSION}" -v)
# At most23 authored child lifetimes, each20s plus two2s reap attempts; 108s margin.
# This watchdog is fixture supervision, not a graceful shutdown guarantee.
set_tests_properties(api-host-native.suite PROPERTIES
    LABELS "api-host-native;acceptance;server;loopback;intentional-red" TIMEOUT 660)
