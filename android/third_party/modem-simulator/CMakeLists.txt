set(MODEM_SIMULATOR_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish/host/commands/modem_simulator/)

include(${MODEM_SIMULATOR_ROOT}/modem-lib-sources.cmake)

set(modem_sources ${modem_simulator_lib_sources})
set(emu_modem_sources
      ${modem_sources}
      android-base/strings.cpp
      common/libs/fs/shared_fd.cpp
      common/libs/utils/files.cpp
      emulator_config.cpp
      android_modem_v2.cpp
      modem_main.cpp
      json/jsoncpp.cpp
      tinyxml2/tinyxml2.cpp
      gflags/src/gflags.cc
      gflags/src/gflags_completions.cc
      gflags/src/gflags_reporting.cc
)

if(WINDOWS)
    list(APPEND ${emu_modem_sources}
        gflags/src/windows_port.cc
        )
endif()

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
      ${emu_modem_sources}
)


target_link_libraries(modem_simulator_lib PRIVATE android-emu-base)
target_link_libraries(modem_simulator_lib PRIVATE android-emu)

target_include_directories(modem_simulator_lib PUBLIC ${MODEM_SIMULATOR_ROOT}/)
target_include_directories(modem_simulator_lib PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android/third_party/modem-simulator)
target_include_directories(modem_simulator_lib PRIVATE gflags/include gflags/include/gflags tinyxml2 json)

target_compile_definitions(modem_simulator_lib PRIVATE -DGOLDFISH_SPECIFIC
        )
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fexceptions -frtti")

android_add_executable(
  TARGET standalone_modem_simulator INSTALL . LICENSE Apache-2.0
  SRC # cmake-format: sortable
      standalone_simulator.cpp)

#add_executable(standalone_modem_simulator standalone_simulator.cpp)
target_include_directories(standalone_modem_simulator PRIVATE gflags/include gflags/include/gflags)
target_link_libraries(standalone_modem_simulator PRIVATE modem_simulator_lib )
target_link_libraries(standalone_modem_simulator PRIVATE android-emu)
target_link_libraries(standalone_modem_simulator PRIVATE android-emu-base)
