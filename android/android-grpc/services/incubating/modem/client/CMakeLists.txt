# Add extended controls service
android_add_library(
  TARGET modem-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/utils/SimpleModemClient.cpp
  DEPS android-emu-base-headers android-emu-grpc-utils)
target_include_directories(modem-service-client PUBLIC include PRIVATE src)
target_link_libraries(modem-service-client PUBLIC modem-service
                                                  android-grpc-client)
