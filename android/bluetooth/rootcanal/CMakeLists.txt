set(BT_ROOT ${AOSP_ROOT}/packages/modules/Bluetooth/system)
set(ROOTCANAL_ROOT ${AOSP_ROOT}/packages/modules/Bluetooth/tools/rootcanal)
set(PDL_ROOT ${AOSP_ROOT}/packages/modules/Bluetooth/tools/pdl)

corrosion_import_crate(
    MANIFEST_PATH ${PDL_ROOT}/Cargo.toml
    FLAGS --offline --verbose --verbose)

corrosion_set_env_vars(generate-canonical-tests CARGO_HOME=${Rust_CARGO_HOME})
corrosion_set_env_vars(pdl CARGO_HOME=${Rust_CARGO_HOME})

# These tests depend on the tempfile crate which was not imported because
# the crate remove_dir_all does not have a compatible version.
set_tests_properties(cargo-test_pdl PROPERTIES DISABLED True)
set_tests_properties(cargo-test_generate-canonical-tests PROPERTIES DISABLED True)

android_license(
    TARGET "pdl"
    LIBNAME None
    SPDX None
    LICENSE None
    LOCAL None)

# ~~~
# ! foobar : Compile .pdl files to C++/Rust!
#
# This function allows you to compile a set of .pdl files
# to rust or C++. It will generate a set of headers and
# sources.
#
# :GENERATED:  The set of sources that are generated.
# Add these sources to the library you are building
# :SRC:        The set of .pdl files to be compiled.
# :INCLUDES:   Include directory used when generating sources.
# :LANG:       Optional parameter indicating which language to use,
# either rust or c++, defaults to c++
# :NAMESPACE:  Root name space to use for the generated c++ classes.
# :OUTPUT_DIR: Optional paramater of the directory where the generated
# sources will be placed, defaults to CMAKE_CURRENT_BINARY_DIR/gens
# :SOURCE_DIR: Root directory where sources can be found,
# defaults to CMAKE_CURRENT_SOURCE_DIR
# ~~~
function(android_bluetooth_packet_gen)
  # Parse arguments
  set(options)
  set(oneValueArgs OUTPUT_DIR GENERATED SOURCE_DIR INCLUDES NAMESPACE LANG)
  set(multiValueArgs SRC)
  cmake_parse_arguments(packet_gen "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  if(NOT packet_gen_OUTPUT_DIR)
    set(packet_gen_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/packet_gen)
  endif()

  if(packet_gen_NAMESPACE)
    set(packet_gen_NAMESPACE "--root_namespace=${packet_gen_NAMESPACE}")
  endif()

  if(NOT packet_gen_SOURCE_DIR)
    set(packet_gen_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(NOT packet_gen_SRC)
    message(
      FATAL_ERROR
        "Error: android_packet_gen_compile() called without any .yy files")
    return()
  endif()

  set(SUFFIX_GEN "h")

  if(packet_gen_LANG STREQUAL "rust")
    set(SUFFIX_GEN "rs")
    set(packet_gen_LANG "--rust")
  endif()

  # Configure packet_gen
  android_compile_for_host(
    bluetooth_packetgen ${ANDROID_QEMU2_TOP_DIR}/android/bluetooth/packet_gen
    bluetooth_packetgen_EXECUTABLE)

  set(BLUE_GEN "")
  file(MAKE_DIRECTORY ${packet_gen_OUTPUT_DIR})

  foreach(FIL ${packet_gen_SRC})
    get_filename_component(
      ABS_FIL ${packet_gen_SOURCE_DIR}/${packet_gen_INCLUDES}/${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(FIL_DIR ${FIL} DIRECTORY)
    set(FIL_GEN "${packet_gen_OUTPUT_DIR}/${FIL_DIR}/${FIL_WE}.${SUFFIX_GEN}")
    add_custom_command(
      OUTPUT "${FIL_GEN}"
      COMMAND
        ${bluetooth_packetgen_EXECUTABLE} ${packet_gen_NAMESPACE}
        "--include=${packet_gen_INCLUDES}" "--out=${packet_gen_OUTPUT_DIR}"
        ${packet_gen_INCLUDES}/${FIL} ${packet_gen_LANG}
      COMMENT "Creating bluetooth packet headers from ${ABS_FIL}"
      WORKING_DIRECTORY ${packet_gen_SOURCE_DIR}
      VERBATIM
      DEPENDS ${bluetooth_packetgen_EXECUTABLE} ${ABS_FIL})
    list(APPEND BLUE_GEN ${FIL_GEN})
    set_source_files_properties(${FIL_GEN} PROPERTIES GENERATED TRUE)
  endforeach()

  # Make the library available
  if(packet_gen_GENERATED)
    set(${packet_gen_GENERATED} "${BLUE_GEN}" PARENT_SCOPE)
  endif()
endfunction()

android_bluetooth_packet_gen(
  GENERATED BluetoothGeneratedPackets_h INCLUDES tools/rootcanal/packets
  SRC hci/hci_packets.pdl SOURCE_DIR ${BT_ROOT}/..)

android_bluetooth_packet_gen(
  GENERATED RootCanalGeneratedPackets_h INCLUDES tools/rootcanal NAMESPACE model
  SRC packets/link_layer_packets.pdl SOURCE_DIR ${BT_ROOT}/..)

android_add_library(
  TARGET libscriptedbeaconpayload-protos-lite LICENSE Apache-2.0
  SOURCE_DIR ${ROOTCANAL_ROOT} SRC ${libscriptedbeaconpayload_protos_lite_src})

protobuf_generate_with_plugin(
  TARGET libscriptedbeaconpayload-protos-lite
  PROTOS ${ROOTCANAL_ROOT}/model/devices/scripted_beacon_ble_payload.proto
  APPEND_PATH
  PROTOPATH -I${AOSP_ROOT}/external/protobuf/src
            -I${ROOTCANAL_ROOT}/model/devices
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/model/devices)

target_include_directories(
  libscriptedbeaconpayload-protos-lite
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${AOSP_ROOT}/external/protobuf/src)

target_link_libraries(libscriptedbeaconpayload-protos-lite
                      PRIVATE protobuf::libprotobuf)

add_library(libbt-rootcanal.headers INTERFACE)
target_include_directories(libbt-rootcanal.headers INTERFACE .
                                                             ${ROOTCANAL_ROOT})
target_link_libraries(libbt-rootcanal.headers
                      INTERFACE android-emu-base-headers)
android_license(TARGET "libbt-rootcanal.headers" LIBNAME None SPDX Apache-2.0
                LICENSE Apache-2.0 LOCAL "${BT_ROOT}/../NOTICE")

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
      view.cc
  DEPS android-emu-base android-emu-base-headers)

target_include_directories(
  BluetoothPacketSources PUBLIC . ${ROOTCANAL_ROOT} ${ROOTCANAL_ROOT}/include
                                ${BT_ROOT}/gd ${BT_ROOT})
target_compile_options(BluetoothPacketSources
                       PUBLIC -Wno-inconsistent-missing-override)

android_add_library(
  TARGET aemu-rootcanal-log-wrapper LICENSE Apache-2.0 SRC os/log.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-base-logging)
target_include_directories(
  aemu-rootcanal-log-wrapper
  PRIVATE . include ${ROOTCANAL_ROOT} ${ROOTCANAL_ROOT}/include ${BT_ROOT}/gd
          ${BT_ROOT})
android_add_library(
  TARGET libbt-hci-packets LICENSE Apache-2.0 SOURCE_DIR ${ROOTCANAL_ROOT}
  SRC model/hci/h4_parser.cc model/hci/hci_protocol.cc
  DEPS aemu-rootcanal-log-wrapper android-emu-base android-emu-base-headers)
target_include_directories(libbt-hci-packets PUBLIC include ${ROOTCANAL_ROOT})

android_add_library(
  TARGET libbt-rootcanal
  LICENSE Apache-2.0
  SOURCE_DIR ${ROOTCANAL_ROOT}
  SRC ${BluetoothGeneratedPackets_h}
      ${RootCanalGeneratedPackets_h}
      lib/crypto/crypto.cc
      lib/hci/address.cc
      lib/hci/class_of_device.cc
      lib/hci/pcap_filter.cc
      model/controller/acl_connection.cc
      model/controller/acl_connection_handler.cc
      model/controller/controller_properties.cc
      model/controller/dual_mode_controller.cc
      model/controller/isochronous_connection_handler.cc
      model/controller/le_advertiser.cc
      model/controller/link_layer_controller.cc
      model/controller/sco_connection.cc
      model/controller/security_manager.cc
      model/devices/beacon.cc
      model/devices/beacon_swarm.cc
      model/devices/device.cc
      model/devices/hci_device.cc
      model/devices/link_layer_socket_device.cc
      model/devices/scripted_beacon.cc
      model/devices/sniffer.cc
      model/hci/h4_data_channel_packetizer.cc
      model/hci/h4_parser.cc
      model/hci/hci_protocol.cc
      model/hci/hci_sniffer.cc
      model/hci/hci_socket_transport.cc
      model/setup/device_boutique.cc
      model/setup/phy_device.cc
      model/setup/phy_layer.cc
      model/setup/test_channel_transport.cc
      model/setup/test_command_handler.cc
      model/setup/test_model.cc
  LINUX net/posix/posix_async_socket.cc
        net/posix/posix_async_socket_connector.cc
        net/posix/posix_async_socket_server.cc
  DARWIN net/posix/posix_async_socket.cc
         net/posix/posix_async_socket_connector.cc
         net/posix/posix_async_socket_server.cc
  DEPS aemu-rootcanal-log-wrapper
       android-emu-base
       android-emu-base-headers
       BluetoothPacketSources
       crypto
       jsoncpp
       libbt-hci-packets
       libscriptedbeaconpayload-protos-lite)
target_include_directories(
  libbt-rootcanal
  PUBLIC .
         include
         ${ROOTCANAL_ROOT}
         ${ROOTCANAL_ROOT}/include
         ${BT_ROOT}/gd
         ${BT_ROOT}
         ${CMAKE_CURRENT_BINARY_DIR}/packet_gen)
target_compile_options(libbt-rootcanal
                       PUBLIC -Wno-inconsistent-missing-override)

add_subdirectory(lmp)
