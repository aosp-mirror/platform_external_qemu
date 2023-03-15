android_grpc_service_lib(
  TARGET ui-controller-service SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES ui_controller_service.proto)

# Lets distribute the proto file
install(FILES proto/ui_controller_service.proto DESTINATION lib)

add_subdirectory(server)
