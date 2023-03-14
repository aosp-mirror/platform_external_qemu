# Add extended controls service
android_add_library(
  TARGET android-grpc-services-stack LICENSE Apache-2.0 SOURCE_DIR src
  SRC android/emulation/control/GrpcServices.cpp
      android/emulation/control/utils/GrpcAndroidLogAdapter.cpp)
target_include_directories(android-grpc-services-stack PRIVATE src
                           PUBLIC include)

android_link_complete_archive(TARGET grpc++_reflection AS
                              grpc++_reflection_whole)
target_link_libraries(
  android-grpc-services-stack
  PRIVATE png android-emu android-emu-base-headers android-grpc-interceptors
          android-grpc-modem-service android-grpc-security
  PUBLIC adb-service-server-impl
         emulated-bluetooth-service-server-impl
         emulator_controller-server-impl
         emulator-stats-server-impl
         snapshot-service-server-impl
         ui-controller-server-impl
         android-waterfall-server-impl
         grpc++
         grpc++_reflection_whole)

android_add_test(
  TARGET android-grpc_unittest
  SOURCE_DIR test
  SRC android/emulation/control/GrpcServices_unittest.cpp
  DEPS android-emu
       android-emu-agents
       android-emu-base-headers
       android-grpc-interceptors
       android-grpc-security
       android-grpc-services-stack
       android-grpc-test-services
       gmock_main
       tink::jwt::jwt_key_templates
       tink::jwt::jwt_signature_config)

#
# Test services & Benchmarks
#
android_grpc_service_lib(
  TARGET android-grpc-testing-services SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/test
  SOURCES ipc_test_service.proto test_echo_service.proto)

android_add_library(
  TARGET android-grpc-test-services
  NODISTRIBUTE
  SOURCE_DIR test
  SRC android/emulation/control/test/CertificateFactory.cpp
      android/emulation/control/test/HeartbeatService.cpp
      android/emulation/control/test/TestEchoService.cpp)
target_link_libraries(
  android-grpc-test-services
  PUBLIC android-grpc-services-stack grpc++ android-emu-grpc-utils
         android-grpc-event-waiter android-grpc-testing-services android-emu)
target_include_directories(android-grpc-test-services PUBLIC test)

# IPC benchmarks
android_add_executable(TARGET ipc_benchmark NODISTRIBUTE SOURCE_DIR test
                       SRC android/emulation/control/test/IPC_benchmark.cpp)
target_link_libraries(ipc_benchmark PRIVATE android-emu emulator-gbench
                                            android-grpc-test-services)

android_add_executable(
  TARGET ipc_reader
  NODISTRIBUTE SOURCE_DIR test
  SRC android/emulation/control/test/IPC_reader.cpp
      android/emulation/control/test/TestEchoService.cpp
  DEPS android-grpc-test-services grpc++)
