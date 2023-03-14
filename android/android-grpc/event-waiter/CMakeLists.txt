android_add_library(
  TARGET android-grpc-event-waiter LICENSE Apache-2.0 SOURCE_DIR src
  SRC android/emulation/control/utils/EventWaiter.cpp)
target_include_directories(android-grpc-event-waiter PUBLIC include PRIVATE src)

android_add_test(
  TARGET android-grpc-event-waiter_unittests SOURCE_DIR test
  SRC android/emulation/control/utils/EventWaiter_unittest.cpp
  DEPS android-emu-base android-grpc-event-waiter gmock_main grpc++)
target_include_directories(android-grpc-event-waiter_unittests PRIVATE src test)
