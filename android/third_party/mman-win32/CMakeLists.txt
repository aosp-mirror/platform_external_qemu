cmake_minimum_required(VERSION 3.5)

# This is a windows only library.
if(WINDOWS)
  add_library(emulator-libmman-win32 mman.c)
  target_include_directories(emulator-libmman-win32 PUBLIC includes)
  target_compile_options(emulator-libmman-win32 PRIVATE "-Wno-macro-redefined")
endif()

android_license(
  TARGET emulator-libmman-win32
  LIBNAME regex
  URL "https://android.googlesource.com/platform/external/qemu.git/+/refs/heads/emu-master-dev/android/third_party/mman-win32/"
  SPDX "MIT"
  LICENSE "https://spdx.org/licenses/MIT.html"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.MIT")
