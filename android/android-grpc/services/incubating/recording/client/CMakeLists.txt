# Add extended controls service
android_add_library(
  TARGET screen-recording-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/utils/SimpleScreenRecordingClient.cpp
  DEPS android-emu-base-headers android-grpc-utils)
target_include_directories(screen-recording-service-client PUBLIC include PRIVATE src)
target_link_libraries(screen-recording-service-client PUBLIC screen-recording-service
                                                  android-grpc-client)
