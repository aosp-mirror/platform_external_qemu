set(CUTTLEFISH_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish/)
set(GRPCINC ${ANDROID_QEMU2_TOP_DIR}/../../external/grpc/include/)
set(GNSS_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish/host/commands/gnss_grpc_proxy/)


set(gnss_sources
    ${GNSS_ROOT}/gnss_grpc_proxy.cpp
)

set(emu_gnss_sources
      ${gnss_sources}
      ../modem-simulator/common/libs/fs/shared_fd.cpp
      ../modem-simulator/common/libs/utils/files.cpp
)

android_add_library(
  TARGET gnss_proxy_lib
  LIBNAME
    gnss_proxy_lib
    URL
    "https://android.googlesource.com/device/google/cuttlefish/+/refs/heads/master/host/commands/gnss_grpc_proxy"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish}"
  LICENSE "Apache-2.0"
  SRC # cmake-format: sortable
      ${emu_gnss_sources}
)

android_license(TARGET "gnss_proxy_lib" LIBNAME None SPDX None LICENSE None LOCAL None)

target_link_libraries(gnss_proxy_lib PRIVATE android-emu-base)
target_link_libraries(gnss_proxy_lib PRIVATE android-emu GLIB2::GLIB2)

target_include_directories(gnss_proxy_lib PUBLIC ${MODEM_SIMULATOR_ROOT}/)
target_include_directories(gnss_proxy_lib PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android/third_party/location)

target_include_directories(gnss_proxy_lib PUBLIC ${GNSS_ROOT}/)
target_include_directories(gnss_proxy_lib PUBLIC ${GRPCINC}/)

target_compile_definitions(gnss_proxy_lib PRIVATE -DGOLDFISH_SPECIFIC
        )


android_add_protobuf(gnss_proxy_proto gnss_grpc_proxy.proto)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  -fexceptions -frtti")
