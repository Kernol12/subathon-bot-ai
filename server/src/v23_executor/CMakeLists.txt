cmake_minimum_required(VERSION 3.25)
project(subathon_v23_executor LANGUAGES CXX)

add_library(subathon_v23_executor STATIC fixture_bounds.cpp step_data.cpp)
target_compile_features(subathon_v23_executor PRIVATE cxx_std_20)
