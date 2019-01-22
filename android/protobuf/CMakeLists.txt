# Add the protobuf dependencies
set(android-emu-protobuf_src "${ANDROID_QEMU2_TOP_DIR}/dummy.c")

# Add the protobuf dependencies
add_subdirectory(android/metrics/proto)
add_subdirectory(android/featurecontrol/proto)
add_subdirectory(android/snapshot/proto)
add_subdirectory(android/crashreport/proto)
add_subdirectory(android/location/proto)
add_subdirectory(android/emulation/proto)
add_subdirectory(android/telephony/proto)
add_subdirectory(android/verified-boot/proto)
add_subdirectory(android/automation/proto)
add_subdirectory(android/offworld/proto)
add_subdirectory(android/darwinn/external/proto/platforms/darwinn/model/config)
add_subdirectory(android/darwinn/proto)
android_add_shared_library(android-emu-protobuf)
target_link_libraries(android-emu-protobuf PUBLIC
    metrics
    featurecontrol
    crashreport
    location
    emulation
    snapshot
    telephony
    verified-boot
    automation
    darwinnpipe
    darwinnmodelconfig
    offworld
    ${PROTOBUF_LIBRARIES}
    )
target_include_directories(android-emu-protobuf PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
android_install_shared(android-emu-protobuf)
#android_add_protobuf(android-emu-proto "${android-emu-proto_src}")
