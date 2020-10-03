cmake_minimum_required(VERSION 3.5)
project(LZ4)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
  include(android)
endif()

set(LIBLZ4_SRC # cmake-format: sortable
               ${ANDROID_QEMU2_TOP_DIR}/../lz4/lib)
android_add_library(
  TARGET lz4
  LICENSE
    "BSD-2-Clause"
    URL
    "https://android.googlesource.com/platform/external/lz4/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../lz4"
  NOTICE "REPO/NOTICE"
  SRC # cmake-format: sortable
      ${LIBLZ4_SRC}/lz4.c ${LIBLZ4_SRC}/lz4frame.c ${LIBLZ4_SRC}/lz4hc.c
      ${LIBLZ4_SRC}/xxhash.c)
target_include_directories(lz4 PUBLIC ${LIBLZ4_SRC})
