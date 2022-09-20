android_add_library(
  TARGET android-emu-base-logging # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/base/logging/Log.cpp android/base/logging/LogConfiguration.cpp
      android/base/logging/LogFormatter.cpp
  DEPS absl::str_format
       android-emu-base-headers)

target_include_directories(android-emu-base-logging PRIVATE src)
target_link_libraries(android-emu-base-logging PRIVATE android-emu-base-headers)

android_add_test(
  TARGET android-emu-base-logging_unittests
  SOURCE_DIR test
  SRC android/base/logging/Log_unittest.cpp
      android/base/logging/LogFormatter_unittest.cpp
  DEPS android-emu-base-headers android-emu-base-logging gmock_main)
target_include_directories(android-emu-base-logging_unittests PRIVATE src)
