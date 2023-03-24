android_grpc_service_lib(
  TARGET emulator-stats-service SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES emulator_stats.proto)

# Lets distribute the proto file
install(FILES proto/emulator_stats.proto DESTINATION lib)

add_subdirectory(server)
