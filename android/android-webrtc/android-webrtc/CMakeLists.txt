set(android-webrtc_src android/emulation/control/EventDispatcher.cpp android/emulation/control/WebRtcBridge.cpp)

# Let's add in the library
android_add_library(android-webrtc)
target_include_directories(android-webrtc PUBLIC .)
target_compile_definitions(android-webrtc PUBLIC -DANDROID_WEBRTC)
target_link_libraries(
  android-webrtc
  PUBLIC android-net android-grpc
  PRIVATE android-emu)

set(android-webrtc_unittest_src android/emulation/control/WebRtcBridge_unittest.cpp)
android_add_test(android-webrtc_unittest)
target_link_libraries(android-webrtc_unittest PRIVATE android-webrtc android-emu-base gmock_main)
