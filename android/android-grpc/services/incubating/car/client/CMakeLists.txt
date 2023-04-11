# Add extended controls service
android_add_library(
  TARGET car-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/utils/SimpleCarClient.cpp
  DEPS android-emu-base-headers android-emu-grpc-utils)
target_include_directories(car-service-client PUBLIC include PRIVATE src)
target_link_libraries(car-service-client PUBLIC car-service
                                          android-grpc-client)
