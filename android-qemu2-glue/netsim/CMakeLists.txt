android_add_library(
  TARGET android-wifi-service
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/netsim/libslirp_driver.cpp
      ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/emulation/VirtioWifiForwarder.cpp
      ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/emulation/WifiService.cpp)

target_include_directories(android-wifi-service
                           PUBLIC ${ANDROID_QEMU2_TOP_DIR}
                           PRIVATE ${ANDROID_QEMU2_TOP_DIR}/include
                                   ${ANDROID_AUTOGEN})

target_compile_definitions(android-wifi-service PUBLIC -DLIBSLIRP)

android_target_compile_definitions(android-wifi-service windows PRIVATE "-DIOV_MAX=1")

target_link_libraries(
  android-wifi-service
  PUBLIC android-emu-base
         android-emu-net
  PRIVATE
         libslirp
         GLIB2::GLIB2
         ssl
         hostapd
         android-emu-base-headers)
