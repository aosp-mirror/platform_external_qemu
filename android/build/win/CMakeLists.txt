project(cc)
cmake_minimum_required(VERSION 3.10)

add_executable(cc flattenrsp.cpp)
target_compile_features(cc PRIVATE cxx_std_17)
target_compile_definitions(cc PRIVATE UNICODE _UNICODE)