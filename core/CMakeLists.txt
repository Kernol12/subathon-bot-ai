add_library(subathon_core STATIC src/time_credit.cpp)
target_include_directories(subathon_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
subathon_target_options(subathon_core)

# The explicit numeric lane is separate while its reviewed baseline is red.
if(SUBATHON_BUILD_EXACT_NUMERIC_TESTS)
    find_package(Boost CONFIG REQUIRED)
    add_library(subathon_exact_numeric STATIC src/exact_decimal.cpp)
    target_include_directories(subathon_exact_numeric PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_exact_numeric PUBLIC Boost::headers)
    subathon_target_options(subathon_exact_numeric)
endif()
