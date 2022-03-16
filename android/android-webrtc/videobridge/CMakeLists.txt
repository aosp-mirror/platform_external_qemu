android_add_library(
  TARGET goldfish-webrtc
  LICENSE Apache-2.0 INSTALL .
  SRC # cmake-format: sortable
      emulator/avd/FakeAvd.cpp
      emulator/net/EmulatorForwarder.cpp
      emulator/net/SocketForwarder.cpp
      emulator/webrtc/capture/AudioSource.cpp
      emulator/webrtc/capture/GrpcAudioSource.cpp
      emulator/webrtc/capture/GrpcVideoSource.cpp
      emulator/webrtc/capture/MediaSourceLibrary.cpp
      emulator/webrtc/capture/VideoCapturerFactory.cpp
      emulator/webrtc/capture/VideoShareCapturer.cpp
      emulator/webrtc/capture/VideoTrackReceiver.cpp
      emulator/webrtc/capture/VideoTrackSource.cpp
      emulator/webrtc/Participant.cpp
      emulator/webrtc/RtcConfig.cpp
      emulator/webrtc/RtcConnection.cpp
      emulator/webrtc/StandaloneConnection.cpp
      emulator/webrtc/Switchboard.cpp)

target_link_libraries(
  goldfish-webrtc
  PUBLIC webrtc_platform_defs
         webrtc_api_libjingle_peerconnection_api.headers
         webrtc_api_libjingle_peerconnection_api
         webrtc_pc_peerconnection
         webrtc_api_create_peerconnection_factory
         webrtc_api_audio_codecs_builtin_audio_decoder_factory
         webrtc_api_audio_codecs_builtin_audio_encoder_factory
         webrtc_api_video_codecs_builtin_video_decoder_factory
         webrtc_api_video_codecs_builtin_video_encoder_factory
         Threads::Threads
         android-net
         android-emu
         android-grpc
         android-webrtc
         nlohmann_json::nlohmann_json
         ssl)
target_include_directories(goldfish-webrtc PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})

android_add_executable(
  TARGET goldfish-webrtc-bridge LICENSE Apache-2.0 INSTALL .
  SRC # cmake-format: sortable
      emulator/main/GoldfishBridge.cpp)
target_link_libraries(goldfish-webrtc-bridge PRIVATE goldfish-webrtc)

android_add_test(
  TARGET goldfish-videobridge_unittest
  SRC # cmake-format: sortable
      emulator/webrtc//RtcConfig_unittest.cpp)
target_include_directories(goldfish-videobridge_unittest
                           PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(goldfish-videobridge_unittest PRIVATE goldfish-webrtc
                                                            gmock_main)
