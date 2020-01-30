# Let's add in the library
android_add_library(
  TARGET android-webrtc
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      android/emulation/control/EventDispatcher.cpp
      android/emulation/control/WebRtcBridge.cpp)
target_include_directories(android-webrtc PUBLIC .)
target_compile_definitions(android-webrtc PUBLIC -DANDROID_WEBRTC)
target_link_libraries(android-webrtc PUBLIC android-net android-grpc
                      PRIVATE android-emu)

android_add_test(
  TARGET android-webrtc_unittest
  SRC # cmake-format: sortable
      android/emulation/control/WebRtcBridge_unittest.cpp)
target_link_libraries(android-webrtc_unittest
                      PRIVATE android-webrtc android-emu-base gmock_main)
