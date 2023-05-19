android_add_library(
  TARGET emulator_controller-server-impl
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/audio/AudioStream.cpp
      android/emulation/control/camera/VirtualSceneCamera.cpp
      android/emulation/control/clipboard/Clipboard.cpp
      android/emulation/control/EmulatorService.cpp
      android/emulation/control/keyboard/EmulatorKeyEventSender.cpp
      android/emulation/control/keyboard/TouchEventSender.cpp
      android/emulation/control/logcat/LogcatParser.cpp
      android/emulation/control/utils/AudioUtils.cpp
      android/emulation/control/utils/ScreenshotUtils.cpp
      android/emulation/control/utils/SharedMemoryLibrary.cpp
  DEPS ui::gl-bridge
       android-emu
       android-emu-base-headers
       android-emu-base-streams
       android-emu-tracing
       android-grpc-event-waiter
       android-grpc-interceptors
       qemu-host-common-headers)
target_link_libraries(emulator_controller-server-impl
                      PUBLIC emulator_controller-service)
target_include_directories(emulator_controller-server-impl PUBLIC include
                           PRIVATE src)

android_add_test(
  TARGET emulator_controller-server-impl_unittest
  SOURCE_DIR test
  SRC android/emulation/control/logcat/LogcatParser_unittest.cpp
  DEPS android-emu-base emulator_controller-server-impl
       emulator_controller-service gmock_main grpc++)
target_include_directories(emulator_controller-server-impl_unittest
                           PRIVATE src test)

#
# Benchmarks
#
android_add_executable(
  TARGET keytranslate-perf
  NODISTRIBUTE SOURCE_DIR test
  SRC android/emulation/control/keyboard/KeytranslatePerf.cpp
  DEPS android-emu android-emu-base android-emu-base-headers emulator-gbench
       emulator_controller-server-impl)
target_include_directories(keytranslate-perf PRIVATE src)

android_add_executable(
  TARGET grpc_stream_screen_benchmark
  NODISTRIBUTE SOURCE_DIR test
  SRC android/emulation/control/StreamScreenPerf.cpp
  DEPS android-emu
       android-emu-base
       android-grpc-client
       emulator-gbench
       emulator_controller-server-impl
       emulator_controller-service
       grpc++)

target_include_directories(grpc_stream_screen_benchmark PRIVATE src)
