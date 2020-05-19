set(MODEM_SIMULATOR_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish/host/commands/modem_simulator/)

include(${MODEM_SIMULATOR_ROOT}/lib-sources.cmake)

set(modem_sources ${modem_simulator_lib_sources})
android_add_library(
  TARGET modem_simulator_lib
  LIBNAME
    modem_simulator_lib
    URL
    "https://android.googlesource.com/device/google/cuttlefish/+/refs/heads/master/host/commands/modem_simulator"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish}"
#  NOTICE "REPO/NOTICE"
  LICENSE "Apache-2.0"
  SRC # cmake-format: sortable
      ${modem_sources}
)

target_include_directories(modem_simulator_lib PUBLIC ${MODEM_SIMULATOR_ROOT}/)
target_include_directories(modem_simulator_lib PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android/third_party/modem_simulator)

target_compile_definitions(modem_simulator_lib PRIVATE -DGOLDFISH_SPECIFIC
        )
