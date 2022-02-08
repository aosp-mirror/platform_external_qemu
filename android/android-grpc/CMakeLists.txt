# Let's bring the grpc module.
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../grpc/emulator grpc)

if(LINUX_X86_64)
  # Our linux headers are from 2013, and do not define newer socket options.
  # (b/156635589)
  target_compile_options(grpc PRIVATE -DSO_REUSEPORT=15)
  target_compile_options(grpc_unsecure PRIVATE -DSO_REUSEPORT=15)
  target_compile_options(grpc_cronet PRIVATE -DSO_REUSEPORT=15)
endif()

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
    SRC # cmake-format: sortable
        ${WATERFALL_GRPC_SRC}
        android/emulation/control/waterfall/AdbWaterfallService.cpp
        android/emulation/control/waterfall/SocketController.cpp
        android/emulation/control/waterfall/WaterfallService.cpp)
  target_link_libraries(android-waterfall PRIVATE png
                        PUBLIC libprotobuf android-emu android-net grpc++)
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
  SRC # cmake-format: sortable
      ${emu_gnss_sources})

target_include_directories(
  gnss_proxy_lib
  PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android/third_party/modem-simulator)
target_include_directories(gnss_proxy_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/)
target_link_libraries(gnss_proxy_lib PRIVATE libprotobuf grpc++)
target_link_libraries(gnss_proxy_lib PRIVATE android-emu-base)
target_link_libraries(gnss_proxy_lib PRIVATE android-emu GLIB2::GLIB2)
target_link_libraries(gnss_proxy_lib PRIVATE modem_simulator_lib)

protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
  INCLUDES ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/snapshot/proto
  SOURCES adb_service.proto emulator_controller.proto snapshot_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  GENERATED EMULATOR_GRPC_SRC)

# Let's add in the library
android_add_library(
  TARGET android-grpc
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${EMULATOR_GRPC_SRC}
      android/emulation/control/adb/AdbService.cpp
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
      android/emulation/control/logcat/RingStreambuf.cpp
      android/emulation/control/secure/BasicTokenAuth.cpp
      android/emulation/control/secure/JwkDirectoryObserver.cpp
      android/emulation/control/secure/JwtTokenAuth.cpp
      android/emulation/control/snapshot/SnapshotService.cpp
      android/emulation/control/utils/AudioUtils.cpp
      android/emulation/control/utils/EventWaiter.cpp
      android/emulation/control/utils/GrpcAndroidLogAdapter.cpp
      android/emulation/control/utils/ScreenshotUtils.cpp
      android/emulation/control/utils/SharedMemoryLibrary.cpp
      android/emulation/control/waterfall/WaterfallFactory.cpp)

target_link_libraries(
  android-grpc
  PRIVATE png
  PUBLIC libprotobuf
         android-emu
         android-net
         grpc++
         tink::config::tink_config
         tink::jwt::jwk_set_converter
         tink::jwt::jwt_public_key_verify
         tink::jwt::jwt_signature_config
         tink::jwt::jwt_validator
         tink::jwt::verified_jwt
         nlohmann_json::nlohmann_json)
target_include_directories(android-grpc PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(android-grpc PRIVATE -Wno-return-type-c-linkage)
set_target_properties(android-grpc PROPERTIES CXX_STANDARD 17)

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
      android/emulation/control/UiController.cpp)

target_include_directories(
  android-ui-controller PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
                                ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(android-ui-controller PUBLIC libprotobuf android-emu
                                                   grpc++)

if(NOT WINDOWS_MSVC_X86_64)
  target_link_libraries(android-grpc PRIVATE android-waterfall)
endif()

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
  SRC # cmake-format: sortable
      ${ECHO_SERVICE_GRPC_SRC}
      ${IPC_SERVICE_GRPC_SRC}
      android/emulation/control/test/CertificateFactory.cpp
      android/emulation/control/test/HeartbeatService.cpp
      android/emulation/control/test/TestEchoService.cpp)
target_link_libraries(android-grpc-test-services PUBLIC android-grpc grpc++)
target_include_directories(android-grpc-test-services
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

android_add_test(
  TARGET android-grpc_unittest
  SRC # cmake-format: sortable
      android/emulation/control/GrpcServices_unittest.cpp
      android/emulation/control/logcat/LogcatParser_unittest.cpp
      android/emulation/control/logcat/RingStreambuf_unittest.cpp
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
# Lets distribute the proto file
install(
  FILES
    emulator_controller.proto
    snapshot_service.proto
    ui_controller_service.proto
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/snapshot/proto/snapshot.proto
  DESTINATION lib)

android_add_executable(
  TARGET qsn INSTALL . LICENSE Apache-2.0
  SRC # cmake-format: sortable
      android/emulation/control/QueryEmulator.cpp)
target_link_libraries(qsn PRIVATE android-emu android-grpc libqemu2-glue)

# Add the benchmark
android_add_executable(
  TARGET grpc_benchmark
  NODISTRIBUTE
  SRC # cmake-format: sortable
      android/emulation/control/keyboard/KeytranslatePerf.cpp
      android/emulation/control/logcat/RingStreamPerf.cpp
      android/emulation/control/snapshot/TarStreamPerf.cpp)
target_link_libraries(grpc_benchmark PRIVATE android-grpc android-emu-base
                                             emulator-gbench)

# IPC benchmarks
android_add_executable(
  TARGET ipc_benchmark NODISTRIBUTE
  SRC # cmake-format: sortable
      android/emulation/control/test/IPC_benchmark.cpp)
target_link_libraries(ipc_benchmark PRIVATE android-emu emulator-gbench
                                            android-grpc-test-services)

android_add_executable(
  TARGET ipc_reader
  NODISTRIBUTE
  SRC # cmake-format: sortable
      android/emulation/control/test/IPC_reader.cpp
      android/emulation/control/test/TestEchoService.cpp)
target_link_libraries(ipc_reader PRIVATE android-emu android-grpc-test-services)
add_dependencies(ipc_benchmark ipc_reader)
