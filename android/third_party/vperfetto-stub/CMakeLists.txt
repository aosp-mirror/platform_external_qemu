cmake_minimum_required(VERSION 3.5)

android_add_library(
    TARGET vperfetto
    LICENSE Apache-2.0
    SRC
    stub.cpp)
target_include_directories(
    vperfetto PUBLIC
    ${ANDROID_QEMU2_TOP_DIR}/android/third_party/perfetto-tracing-only)
