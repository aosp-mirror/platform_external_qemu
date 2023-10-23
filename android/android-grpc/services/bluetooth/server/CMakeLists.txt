android_add_library(
  TARGET emulated-bluetooth-service-server-impl
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/bluetooth/EmulatedBluetoothService.cpp
  DEPS android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-files
       android-grpc-utils
       android-emu-studio-config
       android-grpc-client
       emulated-bluetooth-service)
target_include_directories(emulated-bluetooth-service-server-impl PUBLIC include
                           PRIVATE src)
target_compile_options(emulated-bluetooth-service-server-impl
                       PUBLIC -Wno-extern-c-compat)
target_link_libraries(emulated-bluetooth-service-server-impl
                      PUBLIC emulated-bluetooth-service android-grpc-client)
