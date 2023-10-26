# Add extended controls service
android_add_library(
  TARGET virtual-scene-service-client
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/utils/VirtualSceneClient.cpp
  DEPS android-emu-base-headers android-grpc-utils)
target_include_directories(virtual-scene-service-client PUBLIC include PRIVATE src)
target_link_libraries(virtual-scene-service-client PUBLIC virtual-scene-service
                                                     android-grpc-client)
