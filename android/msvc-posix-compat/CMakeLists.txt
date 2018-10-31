cmake_minimum_required(VERSION 3.5)
# This is a posix wrapper for windows-msvc build.
if ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc")
    add_library(msvc-posix-compat
            src/msvc-posix.c
            src/getopt.c
            src/gettimeofday.c)
    target_include_directories(msvc-posix-compat PUBLIC include)
endif ()
