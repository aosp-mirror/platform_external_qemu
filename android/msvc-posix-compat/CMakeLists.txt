cmake_minimum_required(VERSION 3.5)

if(NOT TARGET msvc-posix-compat)
  # This is a posix wrapper for windows-msvc build.
  set(msvc-posix-compat_src src/msvc-posix.c src/getopt.c src/gettimeofday.c)

  android_nasm_compile(TARGET setjmp_asm_lib SOURCES src/setjmp.asm)
  add_library(msvc-posix-compat ${msvc-posix-compat_src})

  # Msvc posix needs the file-io libs from android-emu-base.
  target_link_libraries(msvc-posix-compat PRIVATE setjmp_asm_lib)

  # Msvc redefines macro's to inject compatibility.
  target_compile_options(msvc-posix-compat
                         PUBLIC "-Wno-macro-redefined"
                                "-Wno-deprecated-declarations" # A lot of the POSIX names are deprecated..
                         )
  target_include_directories(msvc-posix-compat PUBLIC include/ ${ANDROID_QEMU2_TOP_DIR}/android/android-emu)
endif()
