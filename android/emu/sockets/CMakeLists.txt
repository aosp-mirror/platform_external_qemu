# A library that has socket like things.
android_add_library(
  TARGET android-emu-sockets
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/async-console.c
      android/async-socket-connector.c
      android/async-socket.c
      android/async-utils.c
      android/base/async/AsyncSocket.cpp
      android/emulation/CrossSessionSocket.cpp
      android/sdk-controller-socket.c
  DEPS android-emu-base android-emu-base-headers android-emu-utils)
target_include_directories(android-emu-sockets PUBLIC include PRIVATE src)

android_add_test(
  TARGET android-emu-sockets_unittests SOURCE_DIR test
  SRC android/emulation/CrossSessionSocket_unittest.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-sockets gmock gmock_main gtest)
target_include_directories(android-emu-sockets_unittests PRIVATE src)
