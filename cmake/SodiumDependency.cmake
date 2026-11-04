# SPDX-License-Identifier: GPL-3.0-or-later
# Import an explicitly prepared SDK; configure never downloads or runs libsodium.
include_guard(GLOBAL)
set(SUBATHON_SODIUM_ROOT "" CACHE PATH "Verified libsodium 1.0.22 static SDK prefix")
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
   OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "The selected password baseline SDK lane requires Linux x86-64")
endif()
if(NOT SUBATHON_SODIUM_ROOT)
    message(FATAL_ERROR "Set SUBATHON_SODIUM_ROOT to an independently prepared libsodium 1.0.22 static SDK")
endif()
file(REAL_PATH "${SUBATHON_SODIUM_ROOT}" sbt_sodium_root)
set(sbt_sodium_include "${sbt_sodium_root}/include")
set(sbt_sodium_static "${sbt_sodium_root}/lib/libsodium.a")
set(sbt_sodium_version_header "${sbt_sodium_include}/sodium/version.h")
foreach(sbt_sodium_file IN ITEMS "${sbt_sodium_include}/sodium.h" "${sbt_sodium_version_header}" "${sbt_sodium_static}")
    if(NOT EXISTS "${sbt_sodium_file}" OR IS_DIRECTORY "${sbt_sodium_file}")
        message(FATAL_ERROR "The explicit static SDK is incomplete: ${sbt_sodium_file}")
    endif()
endforeach()
file(STRINGS "${sbt_sodium_version_header}" sbt_sodium_version_line REGEX "^#define SODIUM_VERSION_STRING ")
file(STRINGS "${sbt_sodium_version_header}" sbt_sodium_major_line REGEX "^#define SODIUM_LIBRARY_VERSION_MAJOR ")
file(STRINGS "${sbt_sodium_version_header}" sbt_sodium_minor_line REGEX "^#define SODIUM_LIBRARY_VERSION_MINOR ")
if(NOT sbt_sodium_version_line STREQUAL "#define SODIUM_VERSION_STRING \"1.0.22\""
   OR NOT sbt_sodium_major_line STREQUAL "#define SODIUM_LIBRARY_VERSION_MAJOR 26"
   OR NOT sbt_sodium_minor_line STREQUAL "#define SODIUM_LIBRARY_VERSION_MINOR 4")
    message(FATAL_ERROR "The explicit password baseline SDK must declare libsodium 1.0.22 ABI 26/4")
endif()
find_package(Threads REQUIRED)
add_library(Sodium::Sodium STATIC IMPORTED GLOBAL)
set_target_properties(Sodium::Sodium PROPERTIES
    IMPORTED_LOCATION "${sbt_sodium_static}"
    INTERFACE_INCLUDE_DIRECTORIES "${sbt_sodium_include}"
    INTERFACE_LINK_LIBRARIES Threads::Threads)
message(STATUS "Password baseline libsodium 1.0.22 static SDK: ${sbt_sodium_root}; runtime identity is not yet verified")
