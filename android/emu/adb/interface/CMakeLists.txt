# This library enables you to make calls using an adb binary or the direct connection.
android_add_library(
  TARGET android-emu-adb-interface
  # SHARED
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/AdbHostServer.cpp
      android/emulation/control/adb/AdbConnection.cpp
      android/emulation/control/adb/AdbInterface.cpp
      android/emulation/control/adb/adbkey.cpp
      android/emulation/control/adb/AdbShellStream.cpp
  DEPS android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-files
       android-emu-sockets
       android-emu-utils
       android-hw-config
       crypto
       ssl)
target_include_directories(android-emu-adb-interface PRIVATE src PUBLIC include)
target_compile_options(android-emu-adb-interface PRIVATE -Wno-extern-c-compat)

android_add_test(
  TARGET android-emu-adb-interface_unittests
  SOURCE_DIR test
  SRC android/emulation/control/adb/AdbConnection_unittest.cpp
      android/emulation/control/adb/AdbInterface_unittest.cpp
      android/emulation/control/adb/adbkey_unittest.cpp
  DARWIN android/emulation/control/adb/AdbShellStream_unittest.cpp
  LINUX android/emulation/control/adb/AdbShellStream_unittest.cpp
  DEPS android-emu-adb-interface
       android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-files
       android-emu-test-launcher
       android-emu-utils
       android-hw-config
       android-net
       crypto
       gmock
       ssl)
target_include_directories(android-emu-adb-interface_unittests PRIVATE src test)
