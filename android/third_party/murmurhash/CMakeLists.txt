cmake_minimum_required(VERSION 3.5)

android_add_library(TARGET emulator-murmurhash LICENSE None SRC MurmurHash3.cpp)

target_include_directories(emulator-murmurhash PUBLIC .)
