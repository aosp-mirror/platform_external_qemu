protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES rtc_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED EMULATOR_WEBRTC_GRPC_SRC)

# Lets distribute the proto file
install(FILES rtc_service.proto DESTINATION lib)

# Let's add in the library
android_add_library(
  TARGET android-webrtc
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${EMULATOR_WEBRTC_GRPC_SRC}
      android/emulation/control/EventDispatcher.cpp
      android/emulation/control/RtcService.cpp
      android/emulation/control/TurnConfig.cpp
      android/emulation/control/WebRtcBridge.cpp)
target_include_directories(android-webrtc PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(android-webrtc PUBLIC -DANDROID_WEBRTC)
target_link_libraries(android-webrtc PUBLIC android-net android-grpc
                      PRIVATE android-emu)

if(NOT LINUX_AARCH64)
  android_add_test(
    TARGET android-webrtc_unittest
    SRC # cmake-format: sortable
        android/emulation/control/TurnConfig_unittest.cpp
        android/emulation/control/WebRtcBridge_unittest.cpp)
  target_link_libraries(android-webrtc_unittest
                        PRIVATE android-webrtc android-emu-base gmock_main)
endif()
