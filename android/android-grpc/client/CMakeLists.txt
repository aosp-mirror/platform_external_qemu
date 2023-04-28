android_grpc_service_lib(
  TARGET android-grpc-endpoint SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES grpc_endpoint_description.proto)
# Lets distribute the proto file
install(FILES proto/grpc_endpoint_description.proto DESTINATION lib)

# This contains a general grpc client that can discover an emulator and can
# create stubs of arbitrary service definitions.

# You usually want this client in combination with a service protobuf.
android_add_library(
  TARGET android-grpc-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/utils/EmulatorGrpcClient.cpp
  DEPS android-emu-base-headers android-emu-metrics android-emu-studio-config
       android-grpc-interceptors android-grpc-security)
target_link_libraries(
  android-grpc-client PUBLIC android-emu-base android-emu-base-headers
                             android-grpc-endpoint grpc++)
target_include_directories(android-grpc-client PUBLIC include PRIVATE src)
