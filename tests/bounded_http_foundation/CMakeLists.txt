# SPDX-License-Identifier: GPL-3.0-or-later
# The successor producer replaces the preserved candidate4 NI definitions below.
# The NI target must never contain bounded_http_sdk_adapter.cpp or vendor stubs.
add_library(subathon_bounded_http_ni STATIC
    "${PROJECT_SOURCE_DIR}/server/src/bounded_http.cpp")
target_include_directories(subathon_bounded_http_ni PUBLIC
    "${PROJECT_SOURCE_DIR}/server/include")
subathon_target_options(subathon_bounded_http_ni)

add_executable(bounded_http_probe bounded_http_probe.cpp)
set(sbt_bounded_http_producer subathon_bounded_http_ni)
if(sbt_bounded_http_successor)
    set(sbt_bounded_http_producer subathon_bounded_http_development)
endif()
target_link_libraries(bounded_http_probe PRIVATE ${sbt_bounded_http_producer}
    subathon_native_websocket_profile Drogon::Drogon Threads::Threads OpenSSL::Crypto)
subathon_target_options(bounded_http_probe)
set_target_properties(bounded_http_probe PROPERTIES BUILD_RPATH "${sbt_drogon_root}/lib")

# The external technical gate writes this receipt before any feature execution.
# The reviewed driver consumes it; neither this CMake file nor the test mints it.
set(SUBATHON_BOUNDED_HTTP_ADMISSION "${CMAKE_BINARY_DIR}/bounded-http-admission.json"
    CACHE FILEPATH "Bounded HTTP consumer's technical gate receipt")
add_test(NAME bounded-http-foundation.suite
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_bounded_http.py"
        --probe "$<TARGET_FILE:bounded_http_probe>"
        --admission "${SUBATHON_BOUNDED_HTTP_ADMISSION}")
set_tests_properties(bounded-http-foundation.suite PROPERTIES
    LABELS "bounded-http-foundation;acceptance;server;loopback"
    TIMEOUT 10158)
# Ordinary failures, including NI. No WILL_FAIL, skip, DISABLED or dependency waiver.

# 297 suite children * (30s lifetime + two 2s reap attempts) + 60s runner margin.

# Select account capture explicitly. Admit the binary before PRE_TEST discovery.
if(SUBATHON_BUILD_ACCOUNT_CAPTURE_TESTS)
    if(NOT sbt_bounded_http_successor OR NOT TARGET subathon_bounded_http_development OR
       NOT sbt_bounded_http_producer STREQUAL "subathon_bounded_http_development")
        message(FATAL_ERROR "Account capture requires the actual successor foundation target")
    endif()
    if(NOT TARGET GTest::gtest_main)
        include("${PROJECT_SOURCE_DIR}/cmake/GoogleTestDependency.cmake")
    endif()
    include(GoogleTest)
    set(sbt_account_seal "${sbt_drogon_root}/dependency-build.json")
    set(sbt_account_profile "${sbt_drogon_root}/share/subathon-bounded-http-foundation/manifest.json")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${sbt_account_seal}" "${sbt_account_profile}")
    # Configured-data consistency only; actual loaded-DSO attribution is admission.
    file(SHA256 "${sbt_account_seal}" ACCOUNT_CAPTURE_ARTIFACT_SHA256)
    file(READ "${sbt_account_seal}" sbt_account_capture_seal)
    string(JSON sbt_account_capture_profile GET "${sbt_account_capture_seal}" profile_manifest_sha256)
    file(SHA256 "${sbt_account_profile}" ACCOUNT_CAPTURE_PROFILE_SHA256)
    if(NOT sbt_account_capture_profile STREQUAL ACCOUNT_CAPTURE_PROFILE_SHA256)
        message(FATAL_ERROR "Selected account fixture seal/profile disagreement")
    endif()
    set(sbt_account_private "${CMAKE_CURRENT_BINARY_DIR}/account-capture-private")
    set(ACCOUNT_CAPTURE_RUNTIME_PARENT "${CMAKE_CURRENT_BINARY_DIR}/account-capture-runtime")
    configure_file("${PROJECT_SOURCE_DIR}/tests/account_capture/selected_identity.hpp.in"
        "${sbt_account_private}/selected_identity.hpp" @ONLY)
    configure_file("${PROJECT_SOURCE_DIR}/tests/account_capture/runtime_config.hpp.in"
        "${sbt_account_private}/runtime_config.hpp" @ONLY)
    # Expose ONLY this private seam to this test target, never the whole server/src.
    configure_file("${PROJECT_SOURCE_DIR}/server/src/account_capture_observer.hpp"
        "${sbt_account_private}/account_capture_observer.hpp" COPYONLY)
    add_library(subathon_account_capture_ni STATIC
        "${PROJECT_SOURCE_DIR}/server/src/account_capture.cpp"
        "${PROJECT_SOURCE_DIR}/server/src/account_capture_observer.cpp")
    target_include_directories(subathon_account_capture_ni PUBLIC "${PROJECT_SOURCE_DIR}/server/include")
    subathon_target_options(subathon_account_capture_ni)
    add_executable(account_capture_native
        "${PROJECT_SOURCE_DIR}/tests/account_capture/cold_tests.cpp"
        "${PROJECT_SOURCE_DIR}/tests/account_capture/native_tests.cpp"
        "${PROJECT_SOURCE_DIR}/tests/account_capture/scanner_tests.cpp"
        "${PROJECT_SOURCE_DIR}/tests/account_capture/observer_tests.cpp"
        "${PROJECT_SOURCE_DIR}/tests/account_capture/scratch_tests.cpp")
    target_include_directories(account_capture_native PRIVATE "${sbt_account_private}")
    target_link_libraries(account_capture_native PRIVATE subathon_account_capture_ni
        ${sbt_bounded_http_producer} subathon_native_websocket_profile
        Drogon::Drogon Threads::Threads GTest::gtest_main)
    subathon_target_options(account_capture_native)
    set_target_properties(account_capture_native PROPERTIES BUILD_RPATH "${sbt_drogon_root}/lib")
    subathon_discover_gtests(account_capture_native
        LABELS "account-capture;acceptance;server;loopback" TIMEOUT 40)
endif()
