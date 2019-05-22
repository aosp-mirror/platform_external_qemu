# Let's bring the grpc module.
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../grpc/emulator grpc)

set(grpc_proto_src automation.proto)
protobuf_generate_grpc_cpp(SOURCE_DIR
                           ${CMAKE_CURRENT_LIST_DIR}
                           SOURCES
                           android/emulation/control/emulator_controller.proto
                           OUTPUT_DIR
                           ${CMAKE_CURRENT_BINARY_DIR}
                           GENERATED
                           EMULATOR_GRPC_SRC)

set(android-grpc_src
    ${EMULATOR_GRPC_SRC}
    android/emulation/control/EmulatorService.cpp
    android/emulation/control/EmulatorServiceStub.cpp
    android/emulation/control/keyboard/EmulatorKeyEventSender.cpp
    android/emulation/control/GrpcServices.cpp
    android/emulation/control/logcat/RingStreambuf.cpp)

# Let's add in the library
android_add_library(android-grpc)
target_link_libraries(android-grpc PRIVATE grpc++ png PUBLIC libprotobuf android-emu-base)
target_include_directories(android-grpc
                           PRIVATE # For Renderer.h
                                   ${ANDROID_QEMU2_TOP_DIR}/android/android-emugl/host/include
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(android-grpc PUBLIC -DANDROID_GRPC)

set(android-grpc_unittest_src android/emulation/control/logcat/RingStreambuf_unittest.cpp)

android_add_test(android-grpc_unittest)
target_link_libraries(android-grpc_unittest PRIVATE android-grpc gmock_main)

# Lets distribute the proto file
install(FILES android/emulation/control/emulator_controller.proto DESTINATION lib)

# Add the benchmark
set(grpc_benchmark_src android/emulation/control/keyboard/KeytranslatePerf.cpp
    android/emulation/control/logcat/RingStreamPerf.cpp)
android_add_executable(grpc_benchmark)
target_link_libraries(grpc_benchmark PRIVATE android-grpc android-emu-base emulator-gbench)
