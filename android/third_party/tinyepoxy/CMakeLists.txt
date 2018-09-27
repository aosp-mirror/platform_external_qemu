cmake_minimum_required(VERSION 3.5)

add_library(emulator-tinyepoxy tinyepoxy.cpp)

target_include_directories(emulator-tinyepoxy PRIVATE ../../android-emugl/host/include)
