android_grpc_service_lib(
  TARGET android-grpc-incubating-services SOURCE_DIR
         ${CMAKE_CURRENT_LIST_DIR}/proto SOURCES modem_service.proto)

add_subdirectory(server)
add_subdirectory(client)