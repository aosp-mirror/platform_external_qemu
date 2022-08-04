protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES ice_config.proto rtc_service.proto rtc_service_v2.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED EMULATOR_WEBRTC_GRPC_SRC)

# Lets distribute the proto file
install(FILES rtc_service.proto DESTINATION lib)
install(FILES rtc_service_v2.proto DESTINATION lib)
install(FILES ice_config.proto DESTINATION lib)

# Let's add in the library
android_add_library(
  TARGET android-webrtc
  SHARED
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${EMULATOR_WEBRTC_GRPC_SRC}
      android/emulation/control/RtcService.cpp
      android/emulation/control/RtcServiceV2.cpp
      android/emulation/control/TurnConfig.cpp
      emulator/webrtc/capture/AudioSource.cpp
      emulator/webrtc/capture/InprocessAudioSource.cpp
      emulator/webrtc/capture/InprocessVideoSource.cpp
      emulator/webrtc/capture/MediaSourceLibrary.cpp
      emulator/webrtc/capture/VideoTrackReceiver.cpp
      emulator/webrtc/capture/VideoTrackSource.cpp
      emulator/webrtc/Participant.cpp
      emulator/webrtc/RtcConfig.cpp
      emulator/webrtc/RtcConnection.cpp
      emulator/webrtc/Switchboard.cpp)

target_include_directories(android-webrtc PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(android-webrtc PUBLIC -Wno-deprecated-non-prototype)
target_compile_definitions(android-webrtc PUBLIC -DANDROID_WEBRTC)
target_compile_options(android-webrtc PUBLIC -Wno-inconsistent-missing-override)
target_link_libraries(android-webrtc PUBLIC nlohmann_json::nlohmann_json
                      PRIVATE protobuf::libprotobuf
                              grpc++
                              android-emu
                              emulator_controller-server-impl
                              android-grpc-event-waiter
                              qemu-host-common-headers
                              webrtc_platform_defs
                              webrtc_api_libjingle_peerconnection_api
                              webrtc_pc_peerconnection
                              webrtc_api_create_peerconnection_factory
                              webrtc_api_audio_codecs_builtin_audio_decoder_factory
                              webrtc_api_audio_codecs_builtin_audio_encoder_factory
                              webrtc_api_video_codecs_builtin_video_decoder_factory
                              webrtc_api_video_codecs_builtin_video_encoder_factory
                              Threads::Threads)

android_install_shared(android-webrtc)
if(NOT LINUX_AARCH64)
  android_add_test(
    TARGET android-webrtc_unittest
    SRC # cmake-format: sortable
        android/emulation/control/TurnConfig_unittest.cpp
        emulator/webrtc/RtcConfig_unittest.cpp)
  target_link_libraries(android-webrtc_unittest
                        PUBLIC android-webrtc
                              android-emu
                              webrtc_platform_defs
                              webrtc_api_libjingle_peerconnection_api
                              gmock_main)
endif()
