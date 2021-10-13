set(BT_ROOT ${AOSP_ROOT}/system/bt)
set(VL_LIB ${BT_ROOT}/vendor_libs/test_vendor_lib)

android_bluetooth_packet_gen(
  GENERATED BluetoothGeneratedPackets_h INCLUDES system/bt/gd
  SRC hci/hci_packets.pdl l2cap/l2cap_packets.pdl security/smp_packets.pdl
  SOURCE_DIR ${AOSP_ROOT})

android_bluetooth_packet_gen(
  GENERATED RootCanalGeneratedPackets_h
  INCLUDES system/bt/vendor_libs/test_vendor_lib NAMESPACE model
  SRC packets/link_layer_packets.pdl SOURCE_DIR ${AOSP_ROOT})

android_add_library(
  TARGET libscriptedbeaconpayload-protos-lite LICENSE Apache-2.0
  SOURCE_DIR ${BT_ROOT}/vendor_libs/test_vendor_lib
  SRC ${libscriptedbeaconpayload_protos_lite_src})

protobuf_generate_with_plugin(
  TARGET libscriptedbeaconpayload-protos-lite
  PROTOS ${VL_LIB}/model/devices/scripted_beacon_ble_payload.proto
  APPEND_PATH
  PROTOPATH -I${AOSP_ROOT}/external/protobuf/src -I${VL_LIB}/model/devices
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/model/devices)

target_include_directories(
  libscriptedbeaconpayload-protos-lite
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${AOSP_ROOT}/external/protobuf/src)

target_link_libraries(libscriptedbeaconpayload-protos-lite PRIVATE libprotobuf)

android_add_library(
  TARGET libbt-rootcanal-types
  LICENSE Apache-2.0
  SOURCE_DIR ${BT_ROOT}/vendor_libs/test_vendor_lib/types/
  SRC # cmake-format: sortable
      "bluetooth/uuid.cc")
target_include_directories(
  libbt-rootcanal-types PUBLIC . ${BT_ROOT}/vendor_libs/test_vendor_lib/types)

if(NOT LINUX_AARCH64)
  android_add_test(
    TARGET rootcanal-test_types
    SOURCE_DIR ${BT_ROOT}/vendor_libs/test_vendor_lib/types/
    SRC # cmake-format: sortable
        test/bluetooth/uuid_unittest.cc)
  target_link_libraries(rootcanal-test_types PRIVATE libbt-rootcanal-types
                                                     gtest gtest_main)
endif()

add_library(libbt-rootcanal.headers INTERFACE)
target_include_directories(libbt-rootcanal.headers INTERFACE . ${VL_LIB})
android_license(TARGET "libbt-rootcanal.headers" LIBNAME None SPDX Apache-2.0
                LICENSE Apache-2.0 LOCAL "${BT_ROOT}/NOTICE")


android_add_library(
  TARGET BluetoothPacketSources
  LICENSE Apache-2.0
  SOURCE_DIR ${BT_ROOT}/gd/packet
  SRC bit_inserter.cc
      byte_inserter.cc
      byte_observer.cc
      fragmenting_inserter.cc
      iterator.cc
      packet_view.cc
      raw_builder.cc
      view.cc)

target_include_directories(
  BluetoothPacketSources PUBLIC . ${VL_LIB} ${VL_LIB}/include ${BT_ROOT}/gd
                                ${BT_ROOT})
target_link_libraries(BluetoothPacketSources PRIVATE android-emu-base)
target_compile_options(BluetoothPacketSources
                       PUBLIC -Wno-inconsistent-missing-override)

# android_add_library( TARGET libbt-rootcanal-net LICENSE Apache-2.0 SOURCE_DIR
# ${BT_ROOT}/vendor_libs/test_vendor_lib SRC # cmake-format: sortable #
# net/posix/DefaultAsyncSocket.cc # net/posix/DefaultAsyncSocketConnector.cc #
# net/posix/DefaultAsyncSocketServer.cc )

# target_include_directories(libbt-rootcanal-net PUBLIC ${VL_LIB})

android_add_library(
  TARGET libbt-rootcanal
  LICENSE Apache-2.0
  SOURCE_DIR ${BT_ROOT}/vendor_libs/test_vendor_lib
  SRC # cmake-format: sortable
      ${BluetoothGeneratedPackets_h}
      ${BT_ROOT}/gd/hci/address.cc
      ${BT_ROOT}/gd/hci/class_of_device.cc
      ${RootCanalGeneratedPackets_h}
      desktop/test_environment.cc
      model/controller/acl_connection.cc
      model/controller/acl_connection_handler.cc
      model/controller/dual_mode_controller.cc
      model/controller/isochronous_connection_handler.cc
      model/controller/le_advertiser.cc
      model/controller/link_layer_controller.cc
      model/controller/security_manager.cc
      model/devices/beacon.cc
      model/devices/beacon_swarm.cc
      model/devices/broken_adv.cc
      model/devices/car_kit.cc
      model/devices/classic.cc
      model/devices/device.cc
      model/devices/device_properties.cc
      model/devices/h4_data_channel_packetizer.cc
      model/devices/h4_parser.cc
      model/devices/hci_protocol.cc
      model/devices/hci_socket_device.cc
      model/devices/keyboard.cc
      model/devices/link_layer_socket_device.cc
      model/devices/loopback.cc
      model/devices/remote_loopback_device.cc
      model/devices/scripted_beacon.cc
      model/devices/sniffer.cc
      model/setup/device_boutique.cc
      model/setup/phy_layer_factory.cc
      model/setup/test_channel_transport.cc
      model/setup/test_command_handler.cc
      model/setup/test_model.cc
  LINUX # cmake-format: sortable
        net/posix/posix_async_socket.cc
        net/posix/posix_async_socket_connector.cc
        net/posix/posix_async_socket_server.cc
  DARWIN # cmake-format: sortable
         net/posix/posix_async_socket.cc
         net/posix/posix_async_socket_connector.cc
         net/posix/posix_async_socket_server.cc)

target_link_libraries(
  libbt-rootcanal PRIVATE libscriptedbeaconpayload-protos-lite jsoncpp
                          BluetoothPacketSources android-emu-base)
target_include_directories(
  libbt-rootcanal PUBLIC . ${VL_LIB} ${VL_LIB}/include ${BT_ROOT}/gd ${BT_ROOT}
                         ${CMAKE_CURRENT_BINARY_DIR}/packet_gen)
target_compile_options(libbt-rootcanal
                       PUBLIC -Wno-inconsistent-missing-override)

# Compatibility layer for android emulator.
android_add_library(
  TARGET libbt-rootcanal-qemu
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      async_manager.cc
      net/hci_datachannel_server.cpp
      net/mutli_datachannel_server.cpp
      net/qemu_datachannel.cpp
      net/sockets/async_socket.cc
      net/sockets/loopback_async_socket_connector.cc
      net/sockets/loopback_async_socket_server.cc
      root_canal_qemu.cpp)

target_link_libraries(libbt-rootcanal-qemu PRIVATE libbt-rootcanal android-emu)
target_include_directories(
  libbt-rootcanal-qemu PUBLIC .
  PRIVATE ${BT_ROOT}/vendor_libs/test_vendor_lib/desktop/)

android_add_test(
  TARGET test-vendor_test_host
  SOURCE_DIR ${BT_ROOT}/vendor_libs/test_vendor_lib/
  SRC # cmake-format: sortable
      test/h4_parser_unittest.cc test/security_manager_unittest.cc
  DARWIN # cmake-format: sortable
         test/async_manager_unittest.cc
  LINUX # cmake-format: sortable
        test/async_manager_unittest.cc test/posix_socket_unittest.cc)

# Disable until we stabilise the non determinism DARWIN # cmake-format: sortable
# test/posix_socket_unittest.cc)

target_compile_definitions(test-vendor_test_host PRIVATE -DLOG_NDEBUG=1)
target_include_directories(test-vendor_test_host PRIVATE .)
target_link_libraries(
  test-vendor_test_host
  PRIVATE libbt-rootcanal libbt-rootcanal-qemu libbt-rootcanal-types
          android-emu gtest gtest_main)
