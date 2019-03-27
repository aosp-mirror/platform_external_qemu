cmake_minimum_required(VERSION 3.5)

# This is a posix wrapper for windows-msvc build.
set(msvc-posix-compat_src
        src/msvc-posix.c
        src/getopt.c
        src/gettimeofday.c)


android_yasm_compile(TARGET setjmp_asm_lib SOURCES src/setjmp.asm)

android_add_library(msvc-posix-compat)
# Msvc posix needs the file-io libs from android-emu-base.
target_link_libraries(msvc-posix-compat PUBLIC android-emu-base  PRIVATE setjmp_asm_lib)
android_target_include_directories(msvc-posix-compat windows_msvc-x86_64 PUBLIC include/)
