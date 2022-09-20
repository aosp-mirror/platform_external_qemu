# base header library
add_library(android-emu-base-headers INTERFACE)
target_include_directories(android-emu-base-headers INTERFACE include)
android_license(TARGET "android-emu-base-headers" LIBNAME None SPDX None LICENSE None LOCAL None)

add_subdirectory(logging)
add_subdirectory(memory)
add_subdirectory(process)
add_subdirectory(streams)
