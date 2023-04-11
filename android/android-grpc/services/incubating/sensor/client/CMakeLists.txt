# Add extended controls service
android_add_library(
  TARGET sensor-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/utils/SimpleSensorClient.cpp
  DEPS android-emu-base-headers android-emu-grpc-utils)
target_include_directories(sensor-service-client PUBLIC include PRIVATE src)
target_link_libraries(sensor-service-client PUBLIC sensor-service
                                          android-grpc-client)
