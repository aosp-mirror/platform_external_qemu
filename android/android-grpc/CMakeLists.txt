# Add waterfall only for posix, as there are some issues around
# stderr/stdout/stdin in the gRPC compiler. See b/148079576 for details.
if(NOT WINDOWS_MSVC_X86_64)
  protobuf_generate_grpc_cpp(
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES control_socket.proto
                                                 waterfall.proto
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED WATERFALL_GRPC_SRC)
  android_add_library(
    TARGET android-waterfall
    LICENSE Apache-2.0
    SRC ${WATERFALL_GRPC_SRC}
        android/emulation/control/waterfall/AdbWaterfallService.cpp
        android/emulation/control/waterfall/SocketController.cpp
        android/emulation/control/waterfall/WaterfallService.cpp)
  target_link_libraries(
    android-waterfall PRIVATE png android-emu-base-headers qemu-host-common-headers
                      PUBLIC protobuf::libprotobuf android-emu android-net grpc++)
  target_include_directories(
    android-waterfall PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_options(android-waterfall PRIVATE -Wno-return-type-c-linkage)
  set_target_properties(android-waterfall PROPERTIES CXX_STANDARD 17)

  # Lets distribute the proto file
  install(FILES waterfall.proto DESTINATION lib)
endif()

set(GNSS_ROOT
    ${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish/host/commands/gnss_grpc_proxy/
)

protobuf_generate_grpc_cpp(
  SOURCE_DIR ${GNSS_ROOT} SOURCES ${GNSS_ROOT}/gnss_grpc_proxy.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED GNSS_GRPC_PROXY_SRC)

set(emu_gnss_sources location/gnss_grpc_proxy.cpp location/gnss_main.cpp
                     ${GNSS_GRPC_PROXY_SRC})

android_add_library(
  TARGET gnss_proxy_lib
  LIBNAME
    gnss_proxy_lib
    URL
    "https://android.googlesource.com/device/google/cuttlefish/+/refs/heads/master/host/commands/gnss_grpc_proxy"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../../device/google/cuttlefish}"
  LICENSE "Apache-2.0"
  SRC ${emu_gnss_sources})

target_include_directories(
  gnss_proxy_lib
  PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android/third_party/modem-simulator)
target_include_directories(gnss_proxy_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/)
target_link_libraries(gnss_proxy_lib PRIVATE protobuf::libprotobuf grpc++)
target_link_libraries(gnss_proxy_lib PRIVATE android-emu-base android-emu-base-headers)
target_link_libraries(gnss_proxy_lib PRIVATE android-emu GLIB2::GLIB2)
target_link_libraries(gnss_proxy_lib PRIVATE modem_simulator_lib)

protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
  INCLUDES ${ANDROID_QEMU2_TOP_DIR}/android/emu/protos
  SOURCES adb_service.proto emulator_controller.proto
          grpc_endpoint_description.proto snapshot_service.proto
          emulator_stats.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  GENERATED EMULATOR_GRPC_SRC)

android_add_library(TARGET android-grpc-services LICENSE Apache-2.0
                    SRC ${EMULATOR_GRPC_SRC})
target_include_directories(
  android-grpc-services PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(android-grpc-services PUBLIC protobuf::libprotobuf grpc++ android-emu-grpc-utils
                                                   android-emu-protos)
#lib which doesn't include android-emu
android_add_library(
  TARGET android-grpc-utils
  LICENSE Apache-2.0
  SRC
      android/emulation/control/audio/AudioStream.cpp
      android/emulation/control/keyboard/EmulatorKeyEventSender.cpp
      android/emulation/control/keyboard/TouchEventSender.cpp
      android/emulation/control/utils/AudioUtils.cpp
      android/emulation/control/utils/EventWaiter.cpp
      android/emulation/control/utils/GrpcAndroidLogAdapter.cpp
      android/emulation/control/utils/ScreenshotUtils.cpp
      android/emulation/control/utils/SharedMemoryLibrary.cpp)

target_link_libraries(android-grpc-utils PUBLIC android-grpc-services android-emu-shared
                                                 android-emu-tracing android-emu-base-streams)
target_include_directories(android-grpc-utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Let's add in the library
android_add_library(
  TARGET android-grpc
  LICENSE Apache-2.0
  SRC android/emulation/control/adb/AdbService.cpp
      android/emulation/control/audio/AudioStream.cpp
      android/emulation/control/camera/VirtualSceneCamera.cpp
      android/emulation/control/clipboard/Clipboard.cpp
      android/emulation/control/EmulatorService.cpp
      android/emulation/control/GrpcServices.cpp
      android/emulation/control/interceptor/IdleInterceptor.cpp
      android/emulation/control/interceptor/LoggingInterceptor.cpp
      android/emulation/control/interceptor/MetricsInterceptor.cpp
      android/emulation/control/keyboard/EmulatorKeyEventSender.cpp
      android/emulation/control/keyboard/TouchEventSender.cpp
      android/emulation/control/logcat/LogcatParser.cpp
      android/emulation/control/secure/AllowList.cpp
      android/emulation/control/secure/BasicTokenAuth.cpp
      android/emulation/control/secure/JwkDirectoryObserver.cpp
      android/emulation/control/secure/JwtTokenAuth.cpp
      android/emulation/control/snapshot/SnapshotService.cpp
      android/emulation/control/utils/AudioUtils.cpp
      android/emulation/control/utils/EmulatorGrpcClient.cpp
      android/emulation/control/utils/EventWaiter.cpp
      android/emulation/control/utils/GrpcAndroidLogAdapter.cpp
      android/emulation/control/utils/ScreenshotUtils.cpp
      android/emulation/control/utils/SharedMemoryLibrary.cpp
      android/emulation/control/waterfall/WaterfallFactory.cpp
      android/emulation/stats/EmulatorStats.cpp)

android_link_complete_archive(TARGET grpc++_reflection AS grpc++_reflection_whole)
add_dependencies(android-grpc-services grpc++_reflection)
target_link_libraries(
  android-grpc
  PRIVATE png
  PUBLIC android-grpc-services
         android-emu
         android-emu-base-headers
         android-emu-base-streams
         android-emu-tracing
         android-net
         grpc++
         grpc++_reflection_whole
         protobuf::libprotobuf
         tink::config::tink_config
         tink::jwt::jwk_set_converter
         tink::jwt::jwt_public_key_verify
         tink::jwt::jwt_signature_config
         tink::jwt::jwt_validator
         tink::jwt::verified_jwt
         nlohmann_json::nlohmann_json
         qemu-host-common-headers)
target_include_directories(android-grpc PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(android-grpc PRIVATE -Wno-return-type-c-linkage)
if(WINDOWS_MSVC_X86_64)
  message(
    STATUS
      "Using async<->sync bridge until Win32 gRPC is properly supported b/225895409,"
  )
  target_compile_definitions(android-grpc PUBLIC DISABLE_ASYNC_GRPC)
endif()
set_target_properties(android-grpc PROPERTIES CXX_STANDARD 17)

add_custom_command(
  TARGET android-grpc
  PRE_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${ANDROID_QEMU2_TOP_DIR}/android/android-grpc/android/emulation/control/secure/emulator_access.json
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib/emulator_access.json)

protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES ui_controller_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  GENERATED UI_CONTROLLER_SERVICE_GRPC_SRC)

# Add extended controls service
android_add_library(
  TARGET android-ui-controller
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${UI_CONTROLLER_SERVICE_GRPC_SRC}
      android/emulation/control/UiController.cpp
  DEPS android-emu-base-headers
       qemu-host-common-headers)

target_include_directories(
  android-ui-controller PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
                                ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(android-ui-controller PUBLIC protobuf::libprotobuf
                                                   android-emu grpc++)

if(NOT WINDOWS_MSVC_X86_64)
  target_link_libraries(android-grpc PRIVATE android-waterfall)
endif()

# Header only library..
add_library(android-grpc-headers INTERFACE)
target_include_directories(android-grpc-headers INTERFACE .)
android_license(
  TARGET "android-grpc-headers " LIBNAME None SPDX Apache-2.0 LICENSE Apache-2.0
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.APACHE2")

# Set of test services.
protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES test_echo_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED ECHO_SERVICE_GRPC_SRC)

protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES ipc_test_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED IPC_SERVICE_GRPC_SRC)

android_add_library(
  TARGET android-grpc-test-services
  NODISTRIBUTE
  SRC ${ECHO_SERVICE_GRPC_SRC}
      ${IPC_SERVICE_GRPC_SRC}
      android/emulation/control/test/CertificateFactory.cpp
      android/emulation/control/test/HeartbeatService.cpp
      android/emulation/control/test/TestEchoService.cpp)
target_link_libraries(android-grpc-test-services PUBLIC android-grpc grpc++)
target_include_directories(android-grpc-test-services
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

                           android_license(TARGET "frontend-grpc-lib" LIBNAME None SPDX None LICENSE None LOCAL None)
android_add_test(
  TARGET android-grpc_unittest
  SRC android/emulation/control/GrpcServices_unittest.cpp
      android/emulation/control/logcat/LogcatParser_unittest.cpp
      android/emulation/control/secure/AllowList_unittest.cpp
      android/emulation/control/secure/JwkDirectoryObserver_unittest.cpp
      android/emulation/control/secure/JwtTokenAuth_unittest.cpp
      android/emulation/control/utils/EventWaiter_unittest.cpp
  DARWIN android/emulation/control/interceptor/LoggingInterceptor_unittest.cpp
  LINUX android/emulation/control/interceptor/LoggingInterceptor_unittest.cpp)
target_link_libraries(
  android-grpc_unittest
  PRIVATE android-grpc android-grpc-test-services grpc++
          tink::jwt::jwt_key_templates tink::jwt::jwt_signature_config
          gmock_main)
android_target_dependency(android-grpc_unittest linux-x86_64
                          TCMALLOC_OS_DEPENDENCIES)
set_property(TARGET android-grpc_unittest PROPERTY CXX_STANDARD 17)

add_custom_command(
  TARGET android-grpc_unittest
  PRE_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${ANDROID_QEMU2_TOP_DIR}/android/android-grpc/android/emulation/control/test/test_allow_list.json
    $<TARGET_FILE_DIR:android-grpc_unittest>/test_allow_list.json)

# Lets distribute the proto files, and the security configuration
install(
  FILES emulator_controller.proto snapshot_service.proto
        ui_controller_service.proto emulator_stats.proto
        ${ANDROID_QEMU2_TOP_DIR}/android/emu/protos/snapshot.proto
        ${ANDROID_QEMU2_TOP_DIR}/android/android-grpc/android/emulation/control/secure/emulator_access.json
  DESTINATION lib)

android_add_executable(TARGET qsn INSTALL . LICENSE Apache-2.0
                       SRC android/emulation/control/QueryEmulator.cpp)
target_link_libraries(
  qsn PRIVATE android-emu android-grpc libqemu2-glue android-emu-crashreport
              android-emu-crashreport-consent-never)

# Add the benchmark
android_add_executable(
  TARGET grpc_benchmark
  NODISTRIBUTE
  SRC android/emulation/control/keyboard/KeytranslatePerf.cpp
      android/emulation/control/snapshot/TarStreamPerf.cpp
      android/emulation/control/test/AllowListPerf.cpp)
target_link_libraries(grpc_benchmark PRIVATE android-grpc android-emu-base
                                             emulator-gbench)

# IPC benchmarks
android_add_executable(TARGET ipc_benchmark NODISTRIBUTE
                       SRC android/emulation/control/test/IPC_benchmark.cpp)
target_link_libraries(ipc_benchmark PRIVATE android-emu emulator-gbench
                                            android-grpc-test-services)

android_add_executable(
  TARGET ipc_reader NODISTRIBUTE
  SRC android/emulation/control/test/IPC_reader.cpp
      android/emulation/control/test/TestEchoService.cpp)
target_link_libraries(ipc_reader PRIVATE android-emu android-grpc-test-services)
add_dependencies(ipc_benchmark ipc_reader)

# Add the benchmark for streaming screenshots
android_add_executable(TARGET grpc_stream_screen_benchmark NODISTRIBUTE
                       SRC android/emulation/control/test/StreamScreenPerf.cpp)
target_link_libraries(
  grpc_stream_screen_benchmark PRIVATE android-grpc android-emu-base
                                       android-emu emulator-gbench)

# Rootcanal service endpoints.
if(BLUETOOTH_EMULATION)
  protobuf_generate_grpc_cpp(
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    SOURCES emulated_bluetooth.proto emulated_bluetooth_device.proto
            emulated_bluetooth_packets.proto emulated_bluetooth_vhci.proto
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    GENERATED BLUETOOTH_SERVICE_GRPC_SRC)

  # Lets distribute the proto file
  install(FILES emulated_bluetooth.proto emulated_bluetooth_device.proto grpc_endpoint_description.proto
                emulated_bluetooth_packets.proto emulated_bluetooth_vhci.proto grpc_endpoint_description.proto
          DESTINATION lib)

  android_add_library(
    TARGET grpc-emulated-bluetooth
    LICENSE Apache-2.0
    SRC ${BLUETOOTH_SERVICE_GRPC_SRC}
        android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.cpp
        android/emulation/bluetooth/EmulatedBluetoothService.cpp
        android/emulation/bluetooth/GrpcLinkChannelServer.cpp
        android/emulation/bluetooth/HciAsyncDataChannelAdapter.cpp
        android/emulation/bluetooth/PhyConnectionClient.cpp
        android/emulation/bluetooth/VhciService.cpp)

  target_link_libraries(
    grpc-emulated-bluetooth PRIVATE libbt-rootcanal libbt-rootcanal-qemu
                                    android-grpc android-emu-base)
  target_include_directories(grpc-emulated-bluetooth
                             PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})

  android_add_test(
    TARGET android-grpc-emulated-bluetooth_unittest
    SRC android/emulation/bluetooth/BufferedAsyncDataChannelAdapter_unittest.cpp
        android/emulation/bluetooth/GrpcLinkChannelServer_unittest.cpp
        android/emulation/bluetooth/PhyConnectionClient_unittest.cpp)
  target_link_libraries(
    android-grpc-emulated-bluetooth_unittest
    PRIVATE grpc-emulated-bluetooth libbt-rootcanal libbt-rootcanal-qemu
            android-grpc android-emu-base gmock_main)
endif()

add_subdirectory(incubating)