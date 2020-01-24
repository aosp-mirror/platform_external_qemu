cmake_minimum_required(VERSION 3.5)
project(LZ4)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
  get_filename_component(ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
  include(android)
endif()


set(LIBLZ4_SRC ${ANDROID_QEMU2_TOP_DIR}/../lz4/lib)
set(lz4_src
        ${LIBLZ4_SRC}/lz4.c
        ${LIBLZ4_SRC}/lz4hc.c
        ${LIBLZ4_SRC}/lz4frame.c
        ${LIBLZ4_SRC}/xxhash.c)

android_add_library(lz4)
target_include_directories(lz4 PUBLIC ${LIBLZ4_SRC})
android_license(lz4  "${LIBLZ4_SRC}/../NOTICE")