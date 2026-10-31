add_library(subathon_core STATIC src/time_credit.cpp)
target_include_directories(subathon_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
subathon_target_options(subathon_core)

# Exact arithmetic has an explicit target and installed-header dependency.
if(SUBATHON_BUILD_EXACT_NUMERIC_TESTS)
    find_package(Boost 1.83 CONFIG REQUIRED)
    message(STATUS "Exact numeric Boost ${Boost_VERSION}: ${Boost_INCLUDE_DIRS}")
    add_library(subathon_exact_numeric STATIC src/exact_decimal.cpp)
    target_include_directories(subathon_exact_numeric PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_exact_numeric PUBLIC Boost::headers)
    subathon_target_options(subathon_exact_numeric)
endif()

if(SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS)
    add_library(subathon_contribution STATIC src/contribution.cpp)
    target_include_directories(subathon_contribution PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    subathon_target_options(subathon_contribution)
endif()
