project(CRASHPAD)
cmake_minimum_required(VERSION 3.12)

if(NOT DEFINED ANDROID_TARGET_TAG)
  message(
    WARNING
      "You need to invoke the cmake generator with a proper toolchain from android/build/cmake, "
      "It is best to invoke: android/scripts/rebuild. "
      "Using the system toolchain might have unintended consequences, and you will need all required "
      "packages to build qemu (https://wiki.qemu.org/Hosts/Linux,https://wiki.qemu.org/Hosts/Mac)."
  )
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../build/cmake/")
  include(toolchain)
  _get_host_tag(TAG)
  toolchain_configure_tags(${TAG})
endif()

if(NOT ANDROID_EMULATOR_BUILD)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/archives)
  include(android)
  include(prebuilts)
  prebuilt(Threads)
  prebuilt(UUID)
  if(WINDOWS)
    include(emu-windows-libs)
  endif()

  add_subdirectory(../googletest gtest)
  add_subdirectory(../zlib zlib)
  add_subdirectory(../lz4 lz4)
 add_subdirectory(../boringssl boringssl)
  set(AEMU_COMMON_BUILD_CONFIG "qemu-android" CACHE STRING "")
  set(AEMU_COMMON_REPO_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/aemu)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/aemu
                   aemu_common)
endif()

set(CRASHPAD_ROOT ${CMAKE_CURRENT_LIST_DIR})
if(WINDOWS_MSVC_X86_64)
  string(REPLACE "/" "\\" CRASHPAD_TEST_DATA_ROOT "${CRASHPAD_ROOT}")
else()
  set(CRASHPAD_TEST_DATA_ROOT "${CRASHPAD_ROOT}")
endif()

# Make sure we do not have definitions that will break us.
remove_definitions(
  "-DNDEBUG"
  "-DNDEBUG=1"
  "-D_FILE_OFFSET_BITS=64"
  "-D_GNU_SOURCE=1"
  "-D_LARGEFILE_SOURCE"
  "-D_LIBCPP_VERSION=__GLIBCPP__"
  "-D__STDC_CONSTANT_MACROS"
  "-D__STDC_FORMAT_MACROS=1"
  "-D__STDC_LIMIT_MACROS=1"
  "-DANDROID_DEBUG")

add_subdirectory(third_party/mini_chromium)
add_subdirectory(client)
add_subdirectory(compat)
add_subdirectory(handler)
add_subdirectory(minidump)
add_subdirectory(snapshot)
add_subdirectory(test)
add_subdirectory(tools)
add_subdirectory(util)
