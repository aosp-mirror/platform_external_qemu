android_grpc_service_lib(
  TARGET emulator_controller-service SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES emulator_controller.proto)

# Lets distribute the proto file
install(FILES proto/emulator_controller.proto DESTINATION lib)

add_subdirectory(server)
