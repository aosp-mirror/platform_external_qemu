android_add_library(
  TARGET adb-service-server-impl
  # SHARED # Testing if all dependencies are linked
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/adb/AdbService.cpp
  DEPS adb-service android-emu-adb-interface android-emu-base
       android-emu-base-headers)
target_include_directories(adb-service-server-impl PUBLIC include PRIVATE src)
target_link_libraries(adb-service-server-impl PUBLIC adb-service)
