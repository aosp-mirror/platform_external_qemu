cmake_minimum_required(VERSION 3.5)

# This is a windows only library.
if(WINDOWS_MSVC_X86_64 OR WINDOWS_X86_64)
  add_library(emulator-libmman-win32 mman.c)
  target_include_directories(emulator-libmman-win32 PUBLIC includes)
endif()
