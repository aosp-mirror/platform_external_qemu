android_add_library(
  TARGET emulator-libext4_utils
  LIBNAME
    ext4_utils
    URL
    "https://android.googlesource.com/platform/system/extras/+/master/ext4_utils/"
  LICENSE "Apache-2.0"
  REPO "${ANDROID_QEMU2_TOP_DIR}/android/third_party/ext4_utils"
  NOTICE "REPO/NOTICE"
  SRC # cmake-format: sortable
      src/allocate.c
      src/contents.c
      src/crc16.c
      src/ext4_sb.c
      src/ext4_utils.c
      src/extent.c
      src/fs_config.c
      src/indirect.c
      src/make_ext4fs.c
      src/sha1.c
      src/uuid.c
      src/wipe.c)

target_include_directories(emulator-libext4_utils PUBLIC include)
target_link_libraries(emulator-libext4_utils PRIVATE zlib android-emu-base)
target_link_libraries(emulator-libext4_utils PUBLIC emulator-libsparse
                                                    emulator-libselinux)
if(NOT WINDOWS_MSVC_X86_64)
  target_compile_options(emulator-libext4_utils PRIVATE -Wno-error)
endif()

target_compile_definitions(emulator-libext4_utils PRIVATE -DHOST)

# Windows specific settings

android_target_link_libraries(emulator-libext4_utils windows
                              PUBLIC emulator-libmman-win32 ws2_32::ws2_32)
android_target_compile_definitions(emulator-libext4_utils windows PRIVATE
                                   -DUSE_MINGW=1)

set(emulator_make_ext4fs_src)
android_add_executable(
  TARGET emulator_make_ext4fs LICENSE "Apache-2.0" SRC # cmake-format: sortable
                                                       src/make_ext4fs_main.c)
target_link_libraries(emulator_make_ext4fs PRIVATE emulator-libext4_utils)
target_compile_definitions(emulator_make_ext4fs PRIVATE -DHOST)

# Windows specific settings
android_target_link_libraries(emulator_make_ext4fs windows
                              PUBLIC emulator-libmman-win32)
android_target_compile_definitions(emulator_make_ext4fs windows PRIVATE
                                   -DUSE_MINGW=1)
