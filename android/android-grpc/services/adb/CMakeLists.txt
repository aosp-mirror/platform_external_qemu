android_grpc_service_lib(
  TARGET adb-service SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES adb_service.proto)

# Lets distribute the proto file
install(FILES proto/adb_service.proto DESTINATION lib)

add_subdirectory(server)
