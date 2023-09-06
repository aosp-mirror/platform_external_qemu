set(GNSS_ROOT
    ${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish/host/commands/gnss_grpc_proxy/
)

android_grpc_service_lib(TARGET gnss_proxy_service SOURCE_DIR ${GNSS_ROOT}
                         SOURCES ${GNSS_ROOT}/gnss_grpc_proxy.proto)

android_add_library(
  TARGET gnss_proxy_lib
  LIBNAME
    gnss_proxy_lib
    URL
    "https://android.googlesource.com/device/google/cuttlefish/+/refs/heads/master/host/commands/gnss_grpc_proxy"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish}"
  LICENSE "Apache-2.0"
  SOURCE_DIR
  src
  SRC location/gnss_grpc_proxy.cpp location/gnss_main.cpp
  DEPS android-emu
       android-emu-base
       android-emu-base-headers
       glib2
       gnss_proxy_service
       grpc++
       modem_simulator_lib
       protobuf::libprotobuf)

target_include_directories(
  gnss_proxy_lib
  PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android/third_party/modem-simulator
         include/location PRIVATE src)
