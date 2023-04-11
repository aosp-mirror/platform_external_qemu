android_grpc_service_lib(
  TARGET snapshot-service SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/proto
  SOURCES snapshot_service.proto)

target_link_libraries(snapshot-service PUBLIC android-emu-protos)

# Lets distribute the proto file
install(FILES proto/snapshot_service.proto
              ${ANDROID_QEMU2_TOP_DIR}/android/emu/protos/snapshot.proto
        DESTINATION lib)

add_subdirectory(client)
add_subdirectory(server)

android_add_executable(
  TARGET snapshot-service-server-perf
  NODISTRIBUTE SOURCE_DIR test
  SRC android/emulation/control/snapshot/TarStreamPerf.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-utils
       emulator-gbench)
