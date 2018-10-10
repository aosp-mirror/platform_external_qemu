cmake_minimum_required(VERSION 3.5)
project(emugen)
include(android)

if(NOT TARGET gmock_main)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../googletest googletest)
  # Okay, google test is not happy under msvc
  if ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc")
      # Don't include the custom TempDir function. It is for Android only.
    target_compile_definitions(gtest PRIVATE
              -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
  endif ()
endif()

# Emugen executable
set(emugen_src ApiGen.cpp EntryPoint.cpp main.cpp Parser.cpp strUtils.cpp TypeFactory.cpp)
android_add_executable(emugen)
target_include_directories(emugen PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android/android-emu)

# The unit tests
set(emugen_unittests_src Parser.cpp Parser_unittest.cpp)
android_add_test(emugen_unittests)
target_link_libraries(emugen_unittests PRIVATE gmock_main)
