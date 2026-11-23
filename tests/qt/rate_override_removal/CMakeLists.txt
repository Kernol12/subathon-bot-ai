# SPDX-License-Identifier: GPL-3.0-or-later
subathon_add_qt_suite(rate_override_removal_stage1
    NAME rate-override-removal SOURCES tests.cpp
    LIBRARIES subathon_sqlite_rate_overrides subathon_sqlite_modifier_activations
    LABELS "rate-override-removal;acceptance;qt;server" TIMEOUT 180)
if(NOT SUBATHON_RATE_REMOVAL_OWNER_PRESENT)
    target_compile_definitions(rate_override_removal_stage1 PRIVATE SUBATHON_RATE_REMOVAL_MISSING_OWNER)
    set_property(TEST rate-override-removal.suite APPEND PROPERTY LABELS intentional-red)
endif()
