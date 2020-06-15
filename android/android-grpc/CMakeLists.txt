# Let's bring the grpc module.
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../grpc/emulator grpc)

# Add waterfall only for posix, as there are some issues around
# stderr/stdout/stdin in the gRPC compiler. See b/148079576 for details.
if(NOT WINDOWS_MSVC_X86_64)
  protobuf_generate_grpc_cpp(
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES waterfall.proto
                                                 control_socket.proto
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

protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
  INCLUDES ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/snapshot/proto
  SOURCES emulator_controller.proto snapshot_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  GENERATED EMULATOR_GRPC_SRC)

# Let's add in the library
android_add_library(
  TARGET android-grpc
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${EMULATOR_GRPC_SRC}
      android/emulation/control/EmulatorService.cpp
      android/emulation/control/GrpcServices.cpp
      android/emulation/control/clipboard/Clipboard.cpp
      android/emulation/control/interceptor/IdleInterceptor.cpp
      android/emulation/control/interceptor/LoggingInterceptor.cpp
      android/emulation/control/interceptor/MetricsInterceptor.cpp
      android/emulation/control/keyboard/EmulatorKeyEventSender.cpp
      android/emulation/control/keyboard/TouchEventSender.cpp
      android/emulation/control/logcat/LogcatParser.cpp
      android/emulation/control/logcat/RingStreambuf.cpp
      android/emulation/control/secure/BasicTokenAuth.cpp
      android/emulation/control/snapshot/SnapshotService.cpp
      android/emulation/control/snapshot/TarStream.cpp
      android/emulation/control/utils/EventWaiter.cpp
      android/emulation/control/utils/GrpcAndroidLogAdapter.cpp
      android/emulation/control/utils/AudioUtils.cpp
      android/emulation/control/utils/ScreenshotUtils.cpp
      android/emulation/control/utils/ServiceUtils.cpp
      android/emulation/control/waterfall/WaterfallFactory.cpp)

target_link_libraries(android-grpc PRIVATE png PUBLIC libprotobuf android-emu
                                                      android-net grpc++)
target_include_directories(android-grpc PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                               ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(android-grpc PRIVATE -Wno-return-type-c-linkage)
set_target_properties(android-grpc PROPERTIES CXX_STANDARD 17)

if(NOT WINDOWS_MSVC_X86_64)
  target_link_libraries(android-grpc PRIVATE android-waterfall)
endif()

protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES test_echo_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED ECHO_SERVICE_GRPC_SRC)

android_add_test(
  TARGET android-grpc_unittest
  SRC # cmake-format: sortable
      ${ECHO_SERVICE_GRPC_SRC}
      android/emulation/control/GrpcServices_unittest.cpp
      android/emulation/control/logcat/LogcatParser_unittest.cpp
      android/emulation/control/logcat/RingStreambuf_unittest.cpp
      android/emulation/control/snapshot/TarStream_unittest.cpp
      android/emulation/control/utils/EventWaiter_unittest.cpp
      android/emulation/control/test/TestEchoService.cpp
      android/emulation/control/test/CertificateFactory.cpp
  DARWIN android/emulation/control/interceptor/LoggingInterceptor_unittest.cpp
  LINUX android/emulation/control/interceptor/LoggingInterceptor_unittest.cpp)
target_link_libraries(android-grpc_unittest PRIVATE android-grpc grpc++
                                                    gmock_main)
android_target_dependency(android-grpc_unittest linux-x86_64
                          TCMALLOC_OS_DEPENDENCIES)
set_property(TARGET android-grpc_unittest PROPERTY CXX_STANDARD 17)
# Lets distribute the proto file
install(
  FILES
    emulator_controller.proto
    snapshot_service.proto
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/snapshot/proto/snapshot.proto
  DESTINATION lib)

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
