add_library(android-grpc-utils INTERFACE)
target_include_directories(android-grpc-utils INTERFACE include)
target_link_libraries(android-grpc-utils INTERFACE grpc++)
android_license(TARGET "android-grpc-utils" LIBNAME None SPDX None LICENSE None LOCAL None)


android_add_test(
  TARGET android-grpc-utils_unittest SOURCE_DIR test
  SRC android/emulation/control/utils/LibraryTest.cpp
  DEPS android-grpc-client gmock_main)
