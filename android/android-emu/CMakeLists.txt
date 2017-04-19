cmake_minimum_required(VERSION 2.8.11)
project(ANDROID_EMU)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../cmake/")

include(AddCXXCompilerFlag)
include(AddGlobalVariables)

# Call this first to setup various dependencies
add_global_variables()

include(FindProtobuf)

add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-std=c++1y)
add_cxx_compiler_flag(-fPIC)
add_cxx_compiler_flag(-w)
add_definitions(-D__STDC_LIMIT_MACROS)

include_directories(
  ${ANDROID_EMU}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

add_subdirectory(${ANDROID}/third_party third_party)
add_subdirectory(android)
