# A library that contains harware related things
# This can includes:
# - Communcation with guest
# - Information about the host
android_add_library(
  TARGET android-emu-hardware
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/boot-properties.c
      android/cros.c
      android/emulation/android_pipe_host.cpp
      android/emulation/android_pipe_pingpong.c
      android/emulation/android_pipe_throttle.c
      android/emulation/android_pipe_unix.cpp
      android/emulation/android_pipe_zero.c
      android/emulation/android_qemud.cpp
      android/emulation/AndroidAsyncMessagePipe.cpp
      android/emulation/AndroidMessagePipe.cpp
      android/emulation/AndroidPipe.cpp
      android/emulation/ClipboardPipe.cpp
      android/emulation/goldfish_sync.cpp
      android/emulation/GoldfishSyncCommandQueue.cpp
      android/emulation/LogcatPipe.cpp
      android/emulation/qemud/android_qemud_client.cpp
      android/emulation/qemud/android_qemud_multiplexer.cpp
      android/emulation/qemud/android_qemud_serial.cpp
      android/emulation/qemud/android_qemud_service.cpp
      android/emulation/qemud/android_qemud_sink.cpp
      android/emulation/RefcountPipe.cpp
      android/emulation/serial_line.cpp
      android/emulation/SerialLine.cpp
      android/emulation/testing/TestVmLock.cpp
      android/emulation/VmLock.cpp
      android/hw-control.cpp
      android/hw-events.c
      android/hw-fingerprint.c
      android/hw-kmsg.c
      android/hw-qemud.cpp
  DEPS android-emu-adb-interface
       android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-crashreport
       android-emu-feature
       android-emu-files
       android-emu-metrics
       android-emu-sockets
       android-emu-utils
       android-hw-config
       protobuf::libprotobuf)
target_link_libraries(android-emu-hardware PUBLIC android-emu-base)
target_include_directories(android-emu-hardware PRIVATE src PUBLIC include)
target_compile_options(android-emu-hardware PUBLIC -Wno-extern-c-compat)

android_add_library(
  TARGET android-emu-hardware-test
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR test
  SRC android/emulation/testing/TestAndroidPipeDevice.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-hardware)
target_include_directories(android-emu-hardware-test PRIVATE src PUBLIC include
                                                                        test)

android_add_test(
  TARGET android-emu-hardware_unittests
  SOURCE_DIR test
  SRC android/emulation/android_pipe_pingpong_unittest.cpp
      android/emulation/android_pipe_zero_unittest.cpp
      android/emulation/AndroidAsyncMessagePipe_unittest.cpp
      android/emulation/DeviceContextRunner_unittest.cpp
      android/emulation/VmLock_unittest.cpp
  DEPS android-emu
       android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-hardware
       android-emu-hardware-test
       android-emu-protos
       android-emu-test-launcher
       gmock
       gmock_main
       android-emu-crashreport
       android-emu-crashreport-consent-never
       protobuf::libprotobuf)
target_include_directories(android-emu-hardware_unittests PRIVATE src test)
