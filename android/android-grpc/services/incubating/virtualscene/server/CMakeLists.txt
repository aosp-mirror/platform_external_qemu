android_add_library(
  TARGET virtual-scene-service-server-impl
  # SHARED # Testing if all dependencies are linked
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/virtualscene/VirtualSceneService.cpp
  DEPS android-emu android-emu-adb-interface android-emu-agents
       android-emu-base android-emu-base-headers)
target_include_directories(virtual-scene-service-server-impl PUBLIC include
                           PRIVATE src)
target_link_libraries(virtual-scene-service-server-impl PUBLIC virtual-scene-service)
