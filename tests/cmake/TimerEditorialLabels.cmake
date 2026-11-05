# SPDX-License-Identifier: GPL-3.0-or-later
# Assign complete suite labels after native GoogleTest discovery.
if(NOT subathon_timer_editorial_cases)
    message(FATAL_ERROR "Timer editorial cases must be built and discovered first")
endif()
set_tests_properties(${subathon_timer_editorial_cases} PROPERTIES
    LABELS "timer-editorial;acceptance;core")
