android_add_library(
  TARGET android-emu-base-logging
  SHARED
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/base/logging/ColorLogSink.cpp
      android/base/logging/Log.cpp
      android/base/logging/LogConfiguration.cpp
      android/base/logging/LogFormatter.cpp
      android/base/logging/StudioLogSink.cpp
  DEPS absl::absl_log absl::log_initialize android-emu-base-headers)

target_compile_definitions(android-emu-base-logging PRIVATE LOGGING_API_SHARED)
target_include_directories(android-emu-base-logging PRIVATE src PUBLIC include)
target_link_libraries(
  android-emu-base-logging PUBLIC absl::str_format absl::absl_log
                                  absl::log_initialize
  PRIVATE android-emu-base-headers)
android_install_shared_library(TARGET android-emu-base-logging
                               DUPLICATE_FOR_WINDOWS)

android_add_test(
  TARGET android-emu-base-logging_unittests
  SOURCE_DIR .
  SRC src/android/base/logging/LogFormatter.cpp
      test/android/base/logging/Log_unittest.cpp
      test/android/base/logging/LogFormatter_unittest.cpp
      test/android/base/logging/StudioLogSink_unittest.cpp
  DEPS absl::absl_log absl::log_initialize absl::str_format
       android-emu-base-headers android-emu-base-logging gmock_main)
target_include_directories(android-emu-base-logging_unittests PRIVATE src)

# Add the benchmark
if(ENABLE_LOGGING_PERF)
  android_add_executable(
    TARGET android-emu-base-logging_benchmark
    NODISTRIBUTE
    SRC test/android/base/logging/Log_benchmark.cpp
    DEPS absl::str_format android-emu-base android-emu-base-headers
         android-emu-base-logging emulator-gbench)
endif()
