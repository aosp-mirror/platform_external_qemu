android_grpc_service_lib(
  TARGET car-service SOURCE_DIR
         ${CMAKE_CURRENT_LIST_DIR}/proto SOURCES car_service.proto)

add_subdirectory(client)
add_subdirectory(server)