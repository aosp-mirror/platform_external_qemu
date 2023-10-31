# This library contains all the protobufs used by the emulator
# They are gathered here as a shared library due to the fact
# that protobufs get registered on module load (dll/so/exe)
# and register themselves with the libprotobuf database.
# a protobuf can only be registered once.
# Since we compile protobuf as a shared library we cannot use protobufs
# as a static library that occurs in multiple shared libs.
android_add_library(TARGET android-emu-protos SHARED LICENSE Apache-2.0)

if(WINDOWS_MSVC_X86_64)
  protobuf_generate_with_plugin(
    TARGET android-emu-protos
    PROTOS automation.proto
           crash_info.proto
           emulator_feature_patterns.proto
           emulator_features.proto
           emulator_compatible.proto
           observation.proto
           offworld.proto
           point.proto
           route.proto
           sensor_session.proto
           sim_access_rules.proto
           snapshot.proto
           snapshot_deps.proto
           VehicleHalProto.proto
           verified_boot_config.proto
    OUT_VAR AEMU_PROTOS_SRC
    EXPORT_MACRO "AEMU_PROTOS")
  set_source_files_properties(
    ${AEMU_PROTOS_SRC} PROPERTIES COMPILE_DEFINITIONS
                                  AEMU_PROTOS=__declspec\(dllexport\))
  # Make sure our dependents are using dll import on all the properties.
  target_compile_definitions(android-emu-protos
                             PUBLIC AEMU_PROTOS=__declspec\(dllimport\))
else()
  protobuf_generate_with_plugin(
    TARGET android-emu-protos
    PROTOS automation.proto
           crash_info.proto
           emulator_feature_patterns.proto
           emulator_features.proto
           emulator_compatible.proto
           observation.proto
           offworld.proto
           point.proto
           route.proto
           sensor_session.proto
           sim_access_rules.proto
           snapshot.proto
           snapshot_deps.proto
           VehicleHalProto.proto
           verified_boot_config.proto)
endif()
target_link_libraries(android-emu-protos PRIVATE protobuf::libprotobuf)
target_include_directories(android-emu-protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
android_install_shared_library(TARGET android-emu-protos)
