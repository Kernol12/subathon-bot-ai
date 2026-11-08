# SPDX-License-Identifier: GPL-3.0-or-later
# Configure an explicit stock baseline SDK; never download, rebuild or load it.
include_guard(GLOBAL)
if(TARGET Drogon::Drogon OR TARGET Trantor::Trantor)
    message(FATAL_ERROR
        "Native WebSocket acceptance must import both framework targets from its explicit SDK; use a fresh configuration")
endif()
set(SUBATHON_DROGON_ROOT "" CACHE PATH "Independently admitted stock Drogon 1.9.14 SDK prefix")
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
   OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "The selected native WebSocket baseline requires Linux x86-64")
endif()
if(NOT SUBATHON_DROGON_ROOT)
    message(FATAL_ERROR "Set SUBATHON_DROGON_ROOT to the independently admitted stock SDK")
endif()
file(REAL_PATH "${SUBATHON_DROGON_ROOT}" sbt_drogon_root)
set(sbt_drogon_manifest "${sbt_drogon_root}/dependency-build.json")
foreach(sbt_drogon_file IN ITEMS
        "${sbt_drogon_manifest}"
        "${sbt_drogon_root}/include/drogon/WebSocketConnection.h"
        "${sbt_drogon_root}/include/trantor/net/TcpConnection.h"
        "${sbt_drogon_root}/lib/libdrogon.so.1.9.14"
        "${sbt_drogon_root}/lib/libtrantor.so.1.5.29"
        "${sbt_drogon_root}/share/licenses/drogon/LICENSE"
        "${sbt_drogon_root}/share/licenses/trantor/License")
    if(NOT EXISTS "${sbt_drogon_file}" OR IS_DIRECTORY "${sbt_drogon_file}")
        message(FATAL_ERROR "The explicit stock SDK is incomplete: ${sbt_drogon_file}")
    endif()
endforeach()
file(READ "${sbt_drogon_manifest}" sbt_drogon_identity)
string(JSON sbt_drogon_fields ERROR_VARIABLE sbt_drogon_error LENGTH "${sbt_drogon_identity}")
if(sbt_drogon_error OR NOT sbt_drogon_fields EQUAL 3)
    message(FATAL_ERROR "Unrecognized stock SDK identity; preserve and independently review it")
endif()
foreach(sbt_drogon_pin IN ITEMS
        "drogon=3aae769cc405cf5abb9d477675c6430924f02c7c"
        "trantor=78e480f5e138acdc05f76c55a2716718cd846364")
    string(REPLACE "=" ";" sbt_drogon_parts "${sbt_drogon_pin}")
    list(GET sbt_drogon_parts 0 sbt_drogon_key)
    list(GET sbt_drogon_parts 1 sbt_drogon_expected)
    string(JSON sbt_drogon_type ERROR_VARIABLE sbt_drogon_error
        TYPE "${sbt_drogon_identity}" "${sbt_drogon_key}")
    if(sbt_drogon_error OR NOT sbt_drogon_type STREQUAL "STRING")
        message(FATAL_ERROR "Invalid stock SDK revision type")
    endif()
    string(JSON sbt_drogon_actual ERROR_VARIABLE sbt_drogon_error
        GET "${sbt_drogon_identity}" "${sbt_drogon_key}")
    if(sbt_drogon_error OR NOT sbt_drogon_actual STREQUAL sbt_drogon_expected)
        message(FATAL_ERROR "Unexpected ${sbt_drogon_key} stock SDK revision")
    endif()
endforeach()
string(JSON sbt_drogon_fields ERROR_VARIABLE sbt_drogon_error
    LENGTH "${sbt_drogon_identity}" options)
if(sbt_drogon_error OR NOT sbt_drogon_fields EQUAL 16)
    message(FATAL_ERROR "Unrecognized stock SDK build options")
endif()
# This is the existing setup_drogon.py recipe. The pinned Trantor target itself
# overrides the global C++20 setting to C++14; record effective flags in admission.
foreach(sbt_drogon_option IN ITEMS
        "CMAKE_BUILD_TYPE=Release" "CMAKE_CXX_STANDARD=20"
        "CMAKE_CXX_STANDARD_REQUIRED=ON" "CMAKE_INSTALL_LIBDIR=lib"
        "BUILD_SHARED_LIBS=ON" "USE_SUBMODULE=ON" "BUILD_ORM=OFF"
        "BUILD_CTL=OFF" "BUILD_EXAMPLES=OFF" "BUILD_TESTING=OFF" "BUILD_DOC=OFF"
        "BUILD_BROTLI=OFF" "BUILD_YAML_CONFIG=OFF" "BUILD_C-ARES=OFF"
        "USE_SPDLOG=OFF" "TRANTOR_USE_TLS=openssl")
    string(REPLACE "=" ";" sbt_drogon_parts "${sbt_drogon_option}")
    list(GET sbt_drogon_parts 0 sbt_drogon_key)
    list(GET sbt_drogon_parts 1 sbt_drogon_expected)
    string(JSON sbt_drogon_type ERROR_VARIABLE sbt_drogon_error
        TYPE "${sbt_drogon_identity}" options "${sbt_drogon_key}")
    if(sbt_drogon_error OR NOT sbt_drogon_type STREQUAL "STRING")
        message(FATAL_ERROR "Invalid stock SDK option type ${sbt_drogon_key}")
    endif()
    string(JSON sbt_drogon_actual ERROR_VARIABLE sbt_drogon_error
        GET "${sbt_drogon_identity}" options "${sbt_drogon_key}")
    if(sbt_drogon_error OR NOT sbt_drogon_actual STREQUAL sbt_drogon_expected)
        message(FATAL_ERROR "Unexpected stock SDK option ${sbt_drogon_key}")
    endif()
endforeach()

# Resolve both framework packages from this prefix; system transitive libraries
# and actual loaded objects still require the separate exact foundation review.
set(Trantor_DIR "${sbt_drogon_root}/lib/cmake/Trantor")
set(Drogon_DIR "${sbt_drogon_root}/lib/cmake/Drogon")
find_package(Trantor 1.5.29 EXACT CONFIG REQUIRED
    PATHS "${Trantor_DIR}" NO_DEFAULT_PATH)
find_package(Drogon 1.9.14 EXACT CONFIG REQUIRED
    PATHS "${Drogon_DIR}" NO_DEFAULT_PATH)
foreach(sbt_drogon_target IN ITEMS Drogon::Drogon Trantor::Trantor)
    get_target_property(sbt_drogon_location "${sbt_drogon_target}" IMPORTED_LOCATION_RELEASE)
    file(REAL_PATH "${sbt_drogon_location}" sbt_drogon_location)
    if(sbt_drogon_target STREQUAL "Drogon::Drogon")
        set(sbt_drogon_expected "${sbt_drogon_root}/lib/libdrogon.so.1.9.14")
    else()
        set(sbt_drogon_expected "${sbt_drogon_root}/lib/libtrantor.so.1.5.29")
    endif()
    file(REAL_PATH "${sbt_drogon_expected}" sbt_drogon_expected)
    if(NOT sbt_drogon_location STREQUAL sbt_drogon_expected)
        message(FATAL_ERROR "${sbt_drogon_target} does not use the explicit stock SDK")
    endif()
    get_target_property(sbt_drogon_include "${sbt_drogon_target}" INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT sbt_drogon_include STREQUAL "${sbt_drogon_root}/include")
        message(FATAL_ERROR "${sbt_drogon_target} does not use the explicit stock SDK headers")
    endif()
endforeach()
message(STATUS "Stock Drogon/Trantor SDK: ${sbt_drogon_root}; compiled probe/load identity remains a separate gate")
