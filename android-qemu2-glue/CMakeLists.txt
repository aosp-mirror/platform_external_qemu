cmake_minimum_required(VERSION 3.5)

# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge
# to the android build system.
string(REPLACE " " ";" INCLUDES ${LOCAL_C_INCLUDES})
include_directories(${INCLUDES})
include_directories(.)

# Use same compiler config as android build system
set(CMAKE_C_FLAGS ${LOCAL_CFLAGS})
set(CMAKE_CXX_FLAGS  "${LOCAL_CFLAGS} ${LOCAL_CXXFLAGS}")

# Work around some gcc/mingw issues
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()


# Make sure we create archives in the same way..
SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")

# END COMPILER CONFIGURATION
# ==========================
add_library(libqemu2-glue
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
target_include_directories(libqemu2-glue PRIVATE ../android/third_party/libdtb/include/)
