cmake_minimum_required(VERSION 3.5)
<<<<<<< HEAD   (464e37 Merge "Merge empty history for sparse-5409122-L7540000028739)

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

# hw-config generator
add_custom_command(PRE_BUILD
                   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                   COMMAND python ${LOCAL_QEMU2_TOP_DIR}/android/scripts/gen-hw-config.py
                                  ${LOCAL_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
                                  ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                   VERBATIM )
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES GENERATED TRUE)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES HEADER_FILE_ONLY TRUE)
set(GENERATED_SOURCES       ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h)

# END COMPILER CONFIGURATION
# ==========================
set(COMMON_QEMU2GLUE_SOURCES
    adbkey.cpp
    android_pubkey.cpp
=======
set(android-qemu2-glue_src
>>>>>>> BRANCH (510a80 Merge "Merge cherrypicks of [1623139] into sparse-7187391-L1)
    android_qemud.cpp
    audio-capturer.cpp
    audio-output.cpp
    base/async/CpuLooper.cpp
    base/async/Looper.cpp
    base/files/QemuFileStream.cpp
    display.cpp
    drive-share.cpp
    dtb.cpp
    emulation/CharSerialLine.cpp
    emulation/DmaMap.cpp
    emulation/VmLock.cpp
    emulation/android_address_space_device.cpp
    emulation/android_pipe_device.cpp
    emulation/charpipe.c
    emulation/goldfish_sync.cpp
    emulation/serial_line.cpp
    emulation/virtio-goldfish-pipe.cpp
    emulation/virtio-input-multi-touch.c
    emulation/virtio-input-rotary.c
    emulation/virtio_vsock_device.cpp
    looper-qemu.cpp
    net-android.cpp
    proxy/slirp_proxy.cpp
    qemu-automation-agent-impl.cpp
    qemu-battery-agent-impl.cpp
    qemu-car-data-agent-impl.cpp
    qemu-cellular-agent-impl.c
    qemu-console-factory.cpp
    qemu-clipboard-agent-impl.cpp
    qemu-display-agent-impl.cpp
    qemu-finger-agent-impl.c
    qemu-grpc-agent-impl.cpp
    qemu-http-proxy-agent-impl.c
    qemu-location-agent-impl.c
    qemu-multi-display-agent-impl.cpp
    qemu-net-agent-impl.c
    qemu-record-screen-agent-impl.c
    qemu-sensors-agent-impl.cpp
    qemu-setup-dns-servers.cpp
    qemu-setup.cpp
    qemu-telephony-agent-impl.c
    qemu-user-event-agent-impl.c
    qemu-virtual-scene-agent-impl.cpp
    snapshot_compression.cpp
    telephony/modem_init.c
    utils/stream.cpp)

if(NOT OPTION_GFXSTREAM_BACKEND)
  list(APPEND android-qemu2-glue_src "emulation/VirtioWifiForwarder.cpp"
       "emulation/virtio-wifi.cpp" "emulation/HostapdController.cpp")
endif()

android_add_library(
  TARGET libqemu2-glue LICENSE Apache-2.0 SRC # cmake-format: sortable
                                              ${android-qemu2-glue_src})

target_include_directories(
  libqemu2-glue
  PRIVATE .. ../include ../qemu2-auto-generated ../slirp
          ${VIRGLRENDERER_INCLUDE_DIR}
          ${ANDROID_QEMU2_TOP_DIR}/android/third_party/libdtb/include/)

target_link_libraries(libqemu2-glue PRIVATE android-grpc android-ui-controller)
if(WEBRTC)
  target_link_libraries(libqemu2-glue PRIVATE android-webrtc)
endif()
target_link_libraries(libqemu2-glue PRIVATE android-emu GLIB2::GLIB2
                                            PIXMAN::PIXMAN emulator-libdtb ssl)

<<<<<<< HEAD   (464e37 Merge "Merge empty history for sparse-5409122-L7540000028739)
if (DEFINED LOCAL_QEMU2_TOP_DIR)
    get_filename_component(REPO_ROOT "${LOCAL_QEMU2_TOP_DIR}/../.." ABSOLUTE)
    set(CURL_ROOT "${REPO_ROOT}/prebuilts/android-emulator-build/curl/${LOCAL_TARGET_TAG}")
    set(CURL_LIBRARY ${CURL_ROOT}/lib/libcurl.a)
    set(CURL_INCLUDE_DIR ${CURL_ROOT}/include)

    # We will now find the prebuilt package.
    find_package(CURL REQUIRED)
    if (NOT CURL_LIBRARIES MATCHES "${REPO_ROOT}.*")
        message(FATAL_ERROR "Discovered curl at: ${CURL_LIBRARIES}, ${CURL_INCLUDE_DIRS} which is outside of tree ${REPO_ROOT}!")
    endif()
endif()

target_include_directories(libqemu2-glue PRIVATE ../android/third_party/libdtb/include ${CURL_INCLUDE_DIRS})
=======
if(NOT OPTION_GFXSTREAM_BACKEND)
  target_link_libraries(libqemu2-glue PRIVATE hostapd)
endif()
>>>>>>> BRANCH (510a80 Merge "Merge cherrypicks of [1623139] into sparse-7187391-L1)

target_compile_definitions(libqemu2-glue INTERFACE "-DCONFIG_VIRGL")
android_add_library(
  TARGET libqemu2-glue-vm-operations LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${ANDROID_HW_CONFIG_H} qemu-vm-operations-impl.cpp)
target_include_directories(libqemu2-glue-vm-operations
                           PRIVATE .. ../include ../qemu2-auto-generated)
target_link_libraries(
  libqemu2-glue-vm-operations
  PRIVATE android-emu GLIB2::GLIB2 android-hw-config
          nlohmann_json::nlohmann_json)
