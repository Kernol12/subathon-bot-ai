# SPDX-License-Identifier: GPL-3.0-or-later
# Assign the complete list after discovery on all supported CMake versions.
if(NOT subathon_password_cases)
    message(FATAL_ERROR "Password hashing cases must be built and discovered first")
endif()
set_tests_properties(${subathon_password_cases} PROPERTIES
    LABELS "password-hashing;acceptance;server")
