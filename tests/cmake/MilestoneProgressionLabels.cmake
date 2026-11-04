# SPDX-License-Identifier: GPL-3.0-or-later
# Assign complete suite labels after native GoogleTest discovery.
if(NOT subathon_milestone_progression_cases)
    message(FATAL_ERROR "Milestone progression cases must be built and discovered first")
endif()
set_tests_properties(${subathon_milestone_progression_cases} PROPERTIES
    LABELS "milestone-progression;acceptance;intentional-red;core")
