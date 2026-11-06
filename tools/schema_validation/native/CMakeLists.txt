# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit developer artifact; one common callback-free process owner.
add_library(subathon_schema_transport_v1 SHARED transport_entry.cpp)
target_include_directories(subathon_schema_transport_v1 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
subathon_target_options(subathon_schema_transport_v1)
target_compile_options(subathon_schema_transport_v1 PRIVATE -fno-exceptions -fno-rtti -fstack-usage)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(subathon_schema_transport_v1 PRIVATE -fcallgraph-info=su)
endif()
set_target_properties(subathon_schema_transport_v1 PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
target_link_options(subathon_schema_transport_v1 PRIVATE
    "LINKER:-z,now" "LINKER:-z,relro"
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/transport_v1.map")
set_property(TARGET subathon_schema_transport_v1 APPEND PROPERTY
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/transport_v1.map")
