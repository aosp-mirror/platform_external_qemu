#################################################################
#
#  GTest libraries.
#
#  GoogleTest is used to build the emulator's Android-specific
#  unit tests. The sources are located under
#  $ANDROID/external/gtest, not sure if they'll take a
#  CMakeLists.txt there

set(GTEST_SOURCES_DIR ../../../../external/gtest)

add_library(gtest
    ${GTEST_SOURCES_DIR}/src/gtest-all.cc
    ${GTEST_SOURCES_DIR}/src/gtest_main.cc
    )

include_directories(
    ${GTEST_SOURCES_DIR}/include
    ${GTEST_SOURCES_DIR}/
    )

add_compile_options(-O0)
