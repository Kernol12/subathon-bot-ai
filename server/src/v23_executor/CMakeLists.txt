cmake_minimum_required(VERSION 3.25)
project(subathon_v23_executor LANGUAGES CXX)

add_library(subathon_v23_executor STATIC fixture_bounds.cpp step_data.cpp)
target_compile_features(subathon_v23_executor PRIVATE cxx_std_20)

option(SUBATHON_BUILD_V23_SPILL_EVIDENCE "Build disposable v23 spill evidence fixture" OFF)
if(SUBATHON_BUILD_V23_SPILL_EVIDENCE)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Core Sql)
    add_executable(subathon_v23_spill_evidence spill_evidence.cpp)
    target_compile_features(subathon_v23_spill_evidence PRIVATE cxx_std_20)
    target_link_libraries(subathon_v23_spill_evidence PRIVATE Qt6::Core Qt6::Sql)
endif()
