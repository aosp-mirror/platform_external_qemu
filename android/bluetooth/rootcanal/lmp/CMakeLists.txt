message(STATUS "Enabling bluetooth LMP module.")

android_bluetooth_packet_gen(
  GENERATED HciGeneratedPackets_rs INCLUDES gd SRC hci/hci_packets.pdl
  SOURCE_DIR ${BT_ROOT} LANG rust)

android_bluetooth_packet_gen(
  GENERATED LmpGeneratedPackets_rs INCLUDES lmp SRC lmp_packets.pdl
  SOURCE_DIR ${ROOTCANAL_ROOT} LANG rust)
# This will automatically register all the tests as well.
corrosion_import_crate(MANIFEST_PATH ${ROOTCANAL_ROOT}/lmp/Cargo.toml FLAGS
                       --offline --verbose --verbose)
corrosion_set_env_vars(lmp
  LMP_PACKETS_PREBUILT=${LmpGeneratedPackets_rs}
  HCI_PACKETS_PREBUILT=${HciGeneratedPackets_rs}
  CARGO_HOME=${Rust_CARGO_HOME})
android_license(TARGET "lmp" LIBNAME None SPDX None LICENSE None LOCAL None)

add_custom_target(lmp_rust_packets DEPENDS ${LmpGeneratedPackets_rs}
                                           ${HciGeneratedPackets_rs})

# Make sure we have the rust packets generated before we build them.
add_dependencies(cargo-build_lmp lmp_rust_packets)

# Enable lmp in rootcanal
target_link_libraries(libbt-rootcanal PRIVATE lmp)
target_compile_definitions(libbt-rootcanal PRIVATE ROOTCANAL_LMP)
target_include_directories(libbt-rootcanal
                           PRIVATE ${ROOTCANAL_ROOT}/lmp/include)
