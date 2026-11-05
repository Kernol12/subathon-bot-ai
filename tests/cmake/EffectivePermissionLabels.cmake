# SPDX-License-Identifier: GPL-3.0-or-later
# Runs after GoogleTest discovery in CTest, including on the supported older CMake.
if(NOT subathon_permission_cases)
    message(FATAL_ERROR "Effective permission cases must be built and discovered first")
endif()
set_tests_properties(${subathon_permission_cases} PROPERTIES
    LABELS "effective-permissions;acceptance;core")
