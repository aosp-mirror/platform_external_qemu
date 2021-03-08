cmake_minimum_required(VERSION 3.5)

add_library(emulator-libdtb src/libdtb.c)

target_include_directories(emulator-libdtb PUBLIC include)
target_include_directories(emulator-libdtb PRIVATE .)
target_compile_definitions(emulator-libdtb PRIVATE -DHOST)

if(NOT WINDOWS_MSVC_X86_64)
  target_compile_options(emulator-libdtb PRIVATE -Wno-error)
endif()

android_license(
  TARGET emulator-libdtb
  LIBNAME dtc
  URL "https://kernel.googlesource.com/pub/scm/utils/dtc/dtc.git/+/refs/heads/master/"
  SPDX "GPL-2.0-or-later"
  LICENSE
    "https://kernel.googlesource.com/pub/scm/utils/dtc/dtc.git/+/refs/heads/master/README.license"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/android/third_party/libdtb/LICENSE")
