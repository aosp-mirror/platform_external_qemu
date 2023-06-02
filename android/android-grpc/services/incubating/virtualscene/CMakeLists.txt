android_grpc_service_lib(
  TARGET virtual-scene-service SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES virtual_scene_service.proto)

target_link_libraries(virtual-scene-service PUBLIC android-emu-protos)

add_subdirectory(client)
add_subdirectory(server)
