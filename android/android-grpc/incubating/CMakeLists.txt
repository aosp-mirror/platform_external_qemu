protobuf_generate_grpc_cpp(
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} SOURCES modem_service.proto
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED EMULATOR_INCUBATING_GRPC_SRC)

android_add_library(TARGET android-grpc-incubating-services LICENSE Apache-2.0
                    SRC ${EMULATOR_INCUBATING_GRPC_SRC})

target_include_directories(
  android-grpc-incubating-services PUBLIC ${CMAKE_CURRENT_BINARY_DIR} include
                                          ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(android-grpc-incubating-services
                      PUBLIC protobuf::libprotobuf grpc++)

# Let's add in the library
android_add_library(
  TARGET android-grpc-modem-service
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/incubating/ModemService.cpp
  DEPS android-emu-feature android-emu-grpc-utils android-grpc
       android-grpc-incubating-services modem_simulator_lib)
