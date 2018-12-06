cmake_minimum_required(VERSION 3.5)

# This is a posix wrapper for windows-msvc build.
set(msvc-posix-compat_src
        src/msvc-posix.c
        src/getopt.c
        src/gettimeofday.c)
android_add_library(msvc-posix-compat)
android_target_include_directories(msvc-posix-compat windows_msvc PUBLIC include/)
