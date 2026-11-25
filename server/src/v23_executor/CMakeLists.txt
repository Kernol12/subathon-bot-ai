cmake_minimum_required(VERSION 3.25)
project(subathon_v23_executor LANGUAGES CXX)
find_package(Qt6 6.12 REQUIRED COMPONENTS Core)

add_library(subathon_v23_executor STATIC fixture_bounds.cpp step_data.cpp copy_comparison.cpp)
target_compile_features(subathon_v23_executor PRIVATE cxx_std_20)
target_include_directories(subathon_v23_executor PRIVATE "../../include")
target_link_libraries(subathon_v23_executor PRIVATE Qt6::Core)

option(SUBATHON_BUILD_V23_SPILL_EVIDENCE "Build disposable v23 spill evidence fixture" OFF)
if(SUBATHON_BUILD_V23_SPILL_EVIDENCE)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Core Sql)
    add_executable(subathon_v23_spill_evidence spill_evidence.cpp)
    target_compile_features(subathon_v23_spill_evidence PRIVATE cxx_std_20)
    target_link_libraries(subathon_v23_spill_evidence PRIVATE Qt6::Core Qt6::Sql)
endif()

option(SUBATHON_BUILD_V23_TYPED_COPY_WITNESS "Build disposable v23 typed copy witness" OFF)
if(SUBATHON_BUILD_V23_TYPED_COPY_WITNESS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Sql)
    add_executable(server_storage_021_v23_negative
        typed_copy_witness.cpp typed_copy_witness_access.cpp)
    target_compile_features(server_storage_021_v23_negative PRIVATE cxx_std_20)
    target_include_directories(server_storage_021_v23_negative PRIVATE "../../include")
    target_link_libraries(server_storage_021_v23_negative PRIVATE
        subathon_v23_executor Qt6::Core Qt6::Sql)
endif()
