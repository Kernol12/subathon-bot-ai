# SPDX-License-Identifier: GPL-3.0-or-later
# Assign complete suite labels after native GoogleTest discovery.
if(NOT subathon_rate_snapshot_cases)
    message(FATAL_ERROR "Rate snapshot cases must be built and discovered first")
endif()
set_tests_properties(${subathon_rate_snapshot_cases} PROPERTIES
    LABELS "rate-snapshot;acceptance;intentional-red;core")
