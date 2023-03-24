android_grpc_service_lib(
  TARGET sensor-service SOURCE_DIR
         ${CMAKE_CURRENT_LIST_DIR}/proto SOURCES sensor_service.proto)

add_subdirectory(client)
add_subdirectory(server)