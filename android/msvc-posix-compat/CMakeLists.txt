cmake_minimum_required(VERSION 3.5)

# This is a posix wrapper for windows-msvc build.
set(msvc-posix-compat_src
        src/msvc-posix.c
        src/getopt.c
        src/gettimeofday.c)
android_add_library(msvc-posix-compat)
target_link_libraries(msvc-posix-compat android-emu-base)
android_target_include_directories(msvc-posix-compat windows_msvc-x86_64 PUBLIC include/)
