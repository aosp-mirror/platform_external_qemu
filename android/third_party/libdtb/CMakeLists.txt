cmake_minimum_required(VERSION 3.5)

add_library(emulator-libdtb src/libdtb.c)

target_include_directories(emulator-libdtb PUBLIC include)
target_include_directories(emulator-libdtb PRIVATE .)
target_compile_definitions(emulator-libdtb PRIVATE -DHOST)

if (NOT WINDOWS_MSVC_X86_64)
  target_compile_options(emulator-libdtb PRIVATE -Wno-error)
endif()

# Work around some gcc/mingw issues
android_target_compile_definitions(emulator-libdtb windows-x86_64 PRIVATE -DUSE_MINGW=1)
android_license(libdtb  "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.GPLv2")