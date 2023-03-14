android_add_library(
  TARGET snapshot-service-server-impl
  # SHARED # Testing if all dependencies are linked
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/snapshot/SnapshotService.cpp
  DEPS android-emu android-emu-adb-interface android-emu-agents
       android-emu-base android-emu-base-headers android-emu-crashreport)
target_include_directories(snapshot-service-server-impl PUBLIC include
                           PRIVATE src)
target_link_libraries(snapshot-service-server-impl PUBLIC snapshot-service)
