android_grpc_service_lib(
  TARGET avd-service SOURCE_DIR
         ${CMAKE_CURRENT_LIST_DIR}/proto SOURCES avd_service.proto)

add_subdirectory(client)
add_subdirectory(server)