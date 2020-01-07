cmake_minimum_required(VERSION 3.5)

set(libqemu2-glue_src
    android_qemud.cpp
    audio-capturer.cpp
    audio-output.cpp
    base/async/CpuLooper.cpp
    base/async/Looper.cpp
    base/files/QemuFileStream.cpp
    display.cpp
    drive-share.cpp
    emulation/android_pipe_device.cpp
    emulation/android_address_space_device.cpp
    emulation/charpipe.c
    emulation/CharSerialLine.cpp
    emulation/DmaMap.cpp
    emulation/goldfish_sync.cpp
    emulation/serial_line.cpp
    emulation/VmLock.cpp
    emulation/virtio-goldfish-pipe.cpp
    emulation/virtio-input-multi-touch.c
    looper-qemu.cpp
    net-android.cpp
    proxy/slirp_proxy.cpp
    qemu-automation-agent-impl.cpp
    qemu-battery-agent-impl.cpp
    qemu-cellular-agent-impl.c
    qemu-clipboard-agent-impl.cpp
    qemu-display-agent-impl.cpp
    qemu-finger-agent-impl.c
    qemu-grpc-agent-impl.cpp
    qemu-location-agent-impl.c
    qemu-http-proxy-agent-impl.c
    qemu-multi-display-agent-impl.cpp
    qemu-net-agent-impl.c
    qemu-car-data-agent-impl.cpp
    qemu-record-screen-agent-impl.c
    qemu-sensors-agent-impl.cpp
    qemu-setup.cpp
    qemu-setup-dns-servers.cpp
    qemu-telephony-agent-impl.c
    qemu-user-event-agent-impl.c
    qemu-virtual-scene-agent-impl.cpp
    dtb.cpp
    snapshot_compression.cpp
    telephony/modem_init.c
    utils/stream.cpp)

android_add_library(libqemu2-glue)
android_target_include_directories(libqemu2-glue
                                   all
                                   PRIVATE
                                   ..
                                   ../include
                                   ../qemu2-auto-generated
                                   ../slirp
                                   ${VIRGLRENDERER_INCLUDE_DIR}
                                   ${ANDROID_QEMU2_TOP_DIR}/android/third_party/libdtb/include/)
if(GRPC)
  target_link_libraries(libqemu2-glue PRIVATE android-grpc)
endif()
target_link_libraries(libqemu2-glue
                      PRIVATE android-emu
                              GLIB2::GLIB2
                              PIXMAN::PIXMAN
                              emulator-libdtb
                              ssl)
target_compile_definitions(libqemu2-glue INTERFACE "-DCONFIG_VIRGL")

set(libqemu2-glue-vm-operations_src
    ${ANDROID_HW_CONFIG_H}
    qemu-vm-operations-impl.cpp)

android_add_library(libqemu2-glue-vm-operations)
android_target_include_directories(libqemu2-glue-vm-operations
                                   all
                                   PRIVATE
                                   ..
                                   ../include
                                   ../qemu2-auto-generated)
target_link_libraries(libqemu2-glue-vm-operations PRIVATE android-emu GLIB2::GLIB2 android-hw-config nlohmann_json::nlohmann_json)
