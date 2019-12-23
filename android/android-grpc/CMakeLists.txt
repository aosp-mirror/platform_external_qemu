# Let's bring the grpc module.
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../grpc/emulator grpc)

set(grpc_proto_src automation.proto)
protobuf_generate_grpc_cpp(SOURCE_DIR
                           ${CMAKE_CURRENT_LIST_DIR}
                           INCLUDES
                           ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/snapshot/proto
                           SOURCES
                           emulator_controller.proto
                           waterfall.proto
                           control_socket.proto
                           snapshot-service.proto
                           OUTPUT_DIR
                           ${CMAKE_CURRENT_BINARY_DIR}
                           GENERATED
                           EMULATOR_GRPC_SRC)

set(android-grpc_src
    ${EMULATOR_GRPC_SRC}
    android/emulation/control/EmulatorService.cpp
    android/emulation/control/keyboard/EmulatorKeyEventSender.cpp
    android/emulation/control/keyboard/TouchEventSender.cpp
    android/emulation/control/GrpcServices.cpp
    android/emulation/control/interceptor/LoggingInterceptor.cpp
    android/emulation/control/interceptor/MetricsInterceptor.cpp
    android/emulation/control/logcat/RingStreambuf.cpp
    android/emulation/control/logcat/LogcatParser.cpp
    android/emulation/control/snapshot/SnapshotService.cpp
    android/emulation/control/snapshot/TarStream.cpp
    android/emulation/control/utils/ServiceUtils.cpp
    android/emulation/control/waterfall/AdbWaterfallService.cpp
    android/emulation/control/waterfall/SocketController.cpp
    android/emulation/control/waterfall/WaterfallService.cpp
)

# Let's add in the library
android_add_library(android-grpc)
target_link_libraries(android-grpc PRIVATE png PUBLIC libprotobuf android-emu android-net grpc++)
target_include_directories(android-grpc
                           PRIVATE # For Renderer.h ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/host/include
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(android-grpc PUBLIC -DANDROID_GRPC)
target_compile_options(android-grpc PRIVATE -Wno-return-type-c-linkage)
set_target_properties(android-grpc PROPERTIES CXX_STANDARD 17)

set(android-grpc_unittest_src
    android/emulation/control/logcat/LogcatParser_unittest.cpp
    android/emulation/control/logcat/RingStreambuf_unittest.cpp
    android/emulation/control/snapshot/TarStream_unittest.cpp
    android/emulation/control/interceptor/LoggingInterceptor_unittest.cpp
)

android_add_test(android-grpc_unittest)
target_link_libraries(android-grpc_unittest PRIVATE android-grpc grpc++ gmock_main)
android_target_dependency(android-grpc_unittest linux-x86_64 TCMALLOC_OS_DEPENDENCIES)

# Lets distribute the proto file
install(FILES emulator_controller.proto snapshot-service.proto waterfall.proto ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/snapshot/proto/snapshot.proto DESTINATION lib)

# Add the benchmark
set(grpc_benchmark_src android/emulation/control/keyboard/KeytranslatePerf.cpp
    android/emulation/control/logcat/RingStreamPerf.cpp)
android_add_executable(grpc_benchmark)
target_link_libraries(grpc_benchmark PRIVATE android-grpc android-emu-base emulator-gbench)
