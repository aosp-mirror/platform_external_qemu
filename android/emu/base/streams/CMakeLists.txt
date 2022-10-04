android_add_library(
  TARGET android-emu-base-streams LICENSE Apache-2.0 SOURCE_DIR src
  SRC android/base/streams/RingStreambuf.cpp DEPS android-emu-base-headers)

target_include_directories(android-emu-base-streams PRIVATE src)
target_link_libraries(android-emu-base-streams PRIVATE android-emu-base-headers)

android_add_test(
  TARGET android-emu-base-streams_unittests SOURCE_DIR test
  SRC android/base/streams/RingStreambuf_unittest.cpp
  DEPS android-emu-base-headers android-emu-base-streams gmock_main)
target_include_directories(android-emu-base-streams_unittests PRIVATE src)

# Add the benchmark
android_add_executable(
  TARGET android-emu-base-streams_benchmark SOURCE_DIR test NODISTRIBUTE
  SRC android/base/streams/RingStreamPerf.cpp DEPS android-emu-base-streams
                                                   android-emu-base-headers
                                                   emulator-gbench)
