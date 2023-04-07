android_grpc_service_lib(
  TARGET emulated-bluetooth-service SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES emulated_bluetooth.proto emulated_bluetooth_device.proto)
target_link_libraries(emulated-bluetooth-service PUBLIC android-grpc-endpoint)

# Lets distribute the proto file
install(FILES proto/emulated_bluetooth.proto
              proto/emulated_bluetooth_device.proto DESTINATION lib)
add_subdirectory(server)
