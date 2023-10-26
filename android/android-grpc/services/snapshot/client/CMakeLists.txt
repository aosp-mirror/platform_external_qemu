# Add extended controls service
android_add_library(
  TARGET snapshot-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/utils/SnapshotClient.cpp
  DEPS android-emu-base-headers android-grpc-utils)
target_include_directories(snapshot-service-client PUBLIC include PRIVATE src)
target_link_libraries(snapshot-service-client PUBLIC snapshot-service
                                                     android-grpc-client)
