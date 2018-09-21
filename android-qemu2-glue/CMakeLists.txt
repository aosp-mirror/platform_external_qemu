cmake_minimum_required(VERSION 3.5)

prebuilt(GLIB2)
prebuilt(PIXMAN)
prebuilt(LZ4)

# hw-config generator
add_custom_command(PRE_BUILD
                   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                   COMMAND python ${ANDROID_QEMU2_TOP_DIR}/android/scripts/gen-hw-config.py
                                  ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
                                  ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                   VERBATIM )
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES GENERATED TRUE)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES HEADER_FILE_ONLY TRUE)
set(GENERATED_SOURCES       ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h)

# END COMPILER CONFIGURATION
# ==========================
set(COMMON_QEMU2GLUE_SOURCES
    android_qemud.cpp
    audio-capturer.cpp
    audio-output.cpp
    base/async/CpuLooper.cpp
    base/async/Looper.cpp
    base/files/QemuFileStream.cpp
    display.cpp
    drive-share.cpp
    emulation/android_pipe_device.cpp
    emulation/charpipe.c
    emulation/CharSerialLine.cpp
    emulation/DmaMap.cpp
    emulation/goldfish_sync.cpp
    emulation/serial_line.cpp
    emulation/VmLock.cpp
    looper-qemu.cpp
    net-android.cpp
    proxy/slirp_proxy.cpp
    qemu-battery-agent-impl.c
    qemu-cellular-agent-impl.c
    qemu-clipboard-agent-impl.cpp
    qemu-display-agent-impl.cpp
    qemu-finger-agent-impl.c
    qemu-location-agent-impl.c
    qemu-http-proxy-agent-impl.c
    qemu-net-agent-impl.c
    qemu-car-data-agent-impl.cpp
    qemu-record-screen-agent-impl.c
    qemu-sensors-agent-impl.cpp
    qemu-setup.cpp
    qemu-setup-dns-servers.cpp
    qemu-telephony-agent-impl.c
    qemu-user-event-agent-impl.c
    qemu-virtual-scene-agent-impl.cpp
    qemu-vm-operations-impl.cpp
    dtb.cpp
    snapshot_compression.cpp
    telephony/modem_init.c
    utils/stream.cpp
    )


set_source_files_properties(${COMMON_QEMU2GLUE_SOURCES} PROPERTIES OBJECT_DEPENDS ${GENERATED_SOURCES})

add_library(libqemu2-glue ${COMMON_QEMU2GLUE_SOURCES})
target_link_libraries(libqemu2-glue PUBLIC emulator-libdtb ${PIXMAN_LIBRARIES} ${GLIB2_LIBRARIES} ${LZ4_LIBRARIES})

target_compile_definitions(libqemu2-glue PRIVATE  -D__STDC_CONSTANT_MACROS -D_LIBCPP_VERSION=__GLIBCPP__ -D_GNU_SOURCE)
target_compile_options(libqemu2-glue PRIVATE "-Wno-extern-c-compat" "-Wno-invalid-constexpr" "-Wno-c++14-extensions" "-Wno-deprecated-register" "-Wno-inconsistent-missing-override" "-Wno-return-type-c-linkage")
# Pick up the right config.
target_include_directories(libqemu2-glue PUBLIC
    ${ANDROID_QEMU2_TOP_DIR}
    ${ANDROID_QEMU2_TOP_DIR}/slirp
    ${ANDROID_QEMU2_TOP_DIR}/qemu2-auto-generated
    ${ANDROID_QEMU2_TOP_DIR}/include
    config/${ANDROID_TARGET_TAG})

target_include_directories(libqemu2-glue PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android/android-emu ${GLIB2_INCLUDE_DIRS} ${PIXMAN_INCLUDE_DIRS} ${LZ4_INCLUDE_DIRS})

