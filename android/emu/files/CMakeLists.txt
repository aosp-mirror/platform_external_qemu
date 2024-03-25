prebuilt(E2FSPROGS)

# In order for this to be a proper shared library you must:
#
# * Figure out which functions in ./include/... need to be made public
# * Setup an export/import header
#
# The library should already be self contained and can be made shared on linux
# with no changes.
android_add_library(
  TARGET android-emu-files
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/ConfigDirs.cpp
      android/filesystems/ext4_resize.cpp
      android/filesystems/ext4_utils.cpp
      android/filesystems/fstab_parser.cpp
      android/filesystems/internal/PartitionConfigBackend.cpp
      android/filesystems/partition_config.cpp
      android/filesystems/partition_types.cpp
      android/filesystems/ramdisk_extractor.cpp
      android/qt/qt_path.cpp
      android/qt/qt_setup.cpp
      android/uncompress.cpp
      android/utils/aconfig-file.c
      android/utils/dll.c
      android/utils/file_data.c
      android/utils/filelock.cpp
      android/utils/ini.cpp
      android/utils/property_file.c
  DEPS android-emu-base android-emu-base-headers android-hw-config emulator-libext4_utils android-emu-utils)
target_include_directories(
  android-emu-files PUBLIC include
  PRIVATE src ${ANDROID_QEMU2_TOP_DIR}/android/android-emu)

# Boo, we need the make_ext4fs executable
add_dependencies(android-emu-files emulator_make_ext4fs)
android_add_test(
  TARGET android-emu-files_unittests
  SOURCE_DIR test
  SRC android/emulation/ConfigDirs_unittest.cpp
      android/filesystems/ext4_resize_unittest.cpp
      android/filesystems/ext4_utils_unittest.cpp
      android/filesystems/fstab_parser_unittest.cpp
      android/filesystems/partition_config_unittest.cpp
      android/filesystems/partition_types_unittest.cpp
      android/filesystems/ramdisk_extractor_unittest.cpp
      android/filesystems/testing/TestSupport.cpp
      android/qt/qt_path_unittest.cpp
      android/qt/qt_setup_unittest.cpp
      android/utils/aconfig-file_unittest.cpp
      android/utils/file_data_unittest.cpp
      android/utils/filelock_unittest.cpp
      android/utils/property_file_unittest.cpp)

target_compile_options(android-emu-files_unittests PRIVATE -O0)
target_include_directories(android-emu-files_unittests PRIVATE src)
target_link_libraries(
  android-emu-files_unittests
  PRIVATE gmock_main android-emu android-emu-test-launcher android-emu-files
          android-hw-config android-emu-base-headers)

# Boo! We depend on makeext
add_custom_command(
  TARGET android-emu-files_unittests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:emulator_make_ext4fs>
          ${CMAKE_CURRENT_BINARY_DIR})

android_target_dependency(android-emu-files_unittests all
                          E2FSPROGS_DEPENDENCIES)
