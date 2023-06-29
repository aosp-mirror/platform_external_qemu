android_grpc_service_lib(
  TARGET screen-recording-service SOURCE_DIR
         ${CMAKE_CURRENT_LIST_DIR}/proto SOURCES screen_recording_service.proto)

add_subdirectory(server)
add_subdirectory(client)