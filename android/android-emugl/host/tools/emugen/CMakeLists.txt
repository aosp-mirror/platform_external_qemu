cmake_minimum_required(VERSION 3.5)
project(emugen)
include(android)

if(NOT TARGET gmock_main)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../googletest)
endif()

# Emugen executable
set(emugen_src ApiGen.cpp EntryPoint.cpp main.cpp Parser.cpp strUtils.cpp TypeFactory.cpp)
android_add_executable(emugen)
android_target_link_libraries(emugen all PRIVATE android-emu-base)

# The unit tests
set(emugen_unittests_src Parser.cpp Parser_unittest.cpp)
android_add_test(emugen_unittests)
android_target_link_libraries(emugen_unittests all PRIVATE gmock_main)
