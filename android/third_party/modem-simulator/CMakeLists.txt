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
      json/json.cpp
      tinyxml2/tinyxml2.cpp
      gflags/src/gflags.cc
      gflags/src/gflags_completions.cc
      gflags/src/gflags_reporting.cc
#TODO: for windows, add the
#gflags/src/windows_port.cc
)


target_include_directories(modem_simulator_lib PUBLIC ${MODEM_SIMULATOR_ROOT}/)
target_include_directories(modem_simulator_lib PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android/third_party/modem_simulator)
target_include_directories(modem_simulator_lib PRIVATE gflags/include gflags/include/gflags)

target_compile_definitions(modem_simulator_lib PRIVATE -DGOLDFISH_SPECIFIC
        )
