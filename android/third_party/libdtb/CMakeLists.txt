cmake_minimum_required(VERSION 3.5)

add_library(emulator-libdtb src/libdtb.c)

target_include_directories(emulator-libdtb PUBLIC include)
target_include_directories(emulator-libdtb PRIVATE .)
target_compile_definitions(emulator-libdtb PRIVATE -DHOST)

if (NOT MSVC)
  target_compile_options(emulator-libdtb PRIVATE -Wno-error)
endif()

# Work around some gcc/mingw issues
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_definitions(emulator-libdtb PRIVATE -DUSE_MINGW=1)
endif()

