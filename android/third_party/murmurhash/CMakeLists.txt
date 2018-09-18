cmake_minimum_required(VERSION 3.5)

add_library(emulator-murmurhash MurmurHash3.cpp)

target_include_directories(emulator-murmurhash PUBLIC .)
