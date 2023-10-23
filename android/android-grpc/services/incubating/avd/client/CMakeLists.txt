# Add extended controls service
android_add_library(
  TARGET avd-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/utils/AvdClient.cpp
  DEPS android-emu-base-headers android-grpc-utils)
target_include_directories(avd-service-client PUBLIC include PRIVATE src)
target_link_libraries(avd-service-client PUBLIC avd-service
                                          android-grpc-client)
