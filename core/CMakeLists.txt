add_library(subathon_core STATIC src/time_credit.cpp)
target_include_directories(subathon_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
subathon_target_options(subathon_core)
