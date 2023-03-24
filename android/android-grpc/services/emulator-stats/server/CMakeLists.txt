# Add extended controls service
android_add_library(
  TARGET emulator-stats-server-impl
  # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/stats/EmulatorStats.cpp
  DEPS android-emu-agents android-emu-base android-emu-base-headers
       android-emu-base-headers)
target_include_directories(emulator-stats-server-impl PUBLIC include
                           PRIVATE src)
target_compile_options(emulator-stats-server-impl PRIVATE -Wno-extern-c-compat)
target_link_libraries(emulator-stats-server-impl PUBLIC emulator-stats-service)
