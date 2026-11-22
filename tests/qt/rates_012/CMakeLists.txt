# SPDX-License-Identifier: GPL-3.0-or-later
subathon_add_qt_suite(rates_012_stage1
    NAME rate-credentials SOURCES tests.cpp ../../support/credential_native_controls.cpp
    LIBRARIES subathon_rate_credentials subathon_credential_storage subathon_sqlite_credential_catalog
        subathon_open_exchange_rates subathon_exchange_rate_api Sodium::Sodium
    LABELS "rate-credentials;acceptance;qt;server" TIMEOUT 180)
target_include_directories(rates_012_stage1 PRIVATE "${PROJECT_SOURCE_DIR}/server/src")
foreach(symbol IN ITEMS sodium_init randombytes_buf sodium_malloc sodium_mlock sodium_free)
    target_link_options(rates_012_stage1 PRIVATE "LINKER:--wrap=${symbol}")
endforeach()
target_link_options(rates_012_stage1 PRIVATE
    "LINKER:-Map,${CMAKE_CURRENT_BINARY_DIR}/rates_012_stage1.map")
