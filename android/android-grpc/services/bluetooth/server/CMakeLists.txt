android_add_library(
  TARGET emulated-bluetooth-service-server-impl
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.cpp
      android/emulation/bluetooth/EmulatedBluetoothService.cpp
      android/emulation/bluetooth/GrpcLinkChannelServer.cpp
      android/emulation/bluetooth/HciAsyncDataChannelAdapter.cpp
      android/emulation/bluetooth/PhyConnectionClient.cpp
      android/emulation/bluetooth/VhciService.cpp
  DEPS android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-files
       android-emu-grpc-utils
       android-emu-studio-config
       android-grpc-client
       emulated-bluetooth-service
       libbt-rootcanal
       libbt-rootcanal-qemu)
target_include_directories(emulated-bluetooth-service-server-impl PUBLIC include
                           PRIVATE src)
target_compile_options(emulated-bluetooth-service-server-impl
                       PUBLIC -Wno-extern-c-compat)
target_link_libraries(emulated-bluetooth-service-server-impl
                      PUBLIC emulated-bluetooth-service android-grpc-client)

android_add_test(
  TARGET android-emulated-bluetooth-service-server-impl_unittest
  SOURCE_DIR test
  SRC android/emulation/bluetooth/BufferedAsyncDataChannelAdapter_unittest.cpp
      android/emulation/bluetooth/GrpcLinkChannelServer_unittest.cpp
      android/emulation/bluetooth/PhyConnectionClient_unittest.cpp
  DEPS android-emu
       android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-files
       android-emu-grpc-utils
       android-emu-studio-config
       android-emu-test-launcher
       android-grpc-client
       android-grpc-services-stack
       emulated-bluetooth-service
       emulated-bluetooth-service-server-impl
       gmock_main
       libbt-rootcanal
       libbt-rootcanal-qemu)
target_include_directories(
  android-emulated-bluetooth-service-server-impl_unittest PRIVATE test src)
