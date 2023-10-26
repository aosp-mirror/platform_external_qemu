# Add extended controls service
android_add_library(
  TARGET emulator_controller-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/utils/EmulatorControlClient.cpp
      android/emulation/control/utils/VmControlClient.cpp
  DEPS android-emu-base-headers)
target_include_directories(emulator_controller-service-client PUBLIC include
                           PRIVATE src)
target_link_libraries(
  emulator_controller-service-client
  PUBLIC emulator_controller-service android-grpc-utils android-grpc-client)
