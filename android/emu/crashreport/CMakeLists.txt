android_add_library(
  TARGET android-emu-crashreport
  SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/crashreport/CrashReporter_common.cpp
      android/crashreport/CrashSystem.cpp
      android/crashreport/detectors/CrashDetectors.cpp
      android/crashreport/HangDetector.cpp
  WINDOWS android/crashreport/CrashReporter_windows.cpp
  LINUX android/crashreport/CrashReporter_linux.cpp
  DARWIN android/crashreport/CrashReporter_darwin.cpp
  DEPS android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-files
       android-emu-metrics
       android-emu-utils
       android-hw-config
       breakpad_client
       protobuf::libprotobuf)
target_include_directories(android-emu-crashreport PRIVATE src PUBLIC include)

target_compile_definitions(android-emu-crashreport
                           PRIVATE "-DCRASHUPLOAD=${OPTION_CRASHUPLOAD}")
# android_add_test(TARGET android-emu-crashreport_unittests SOURCE_DIR test SRC)

# target_link_libraries(android-emu-crashreport_unittests PRIVATE
# android-emu-crashreport gmock_main)

# This file defines emulator crash service
if(NOT BUILDING_FOR_AARCH64 AND NOT OPTION_GFXSTREAM_BACKEND)
  prebuilt(QT5)
  set(CRASH_WINDOWS_ICON
      ${ANDROID_QEMU2_TOP_DIR}/android/images/emulator_icon.rc)

  android_add_library(
    TARGET android-emu-crash-service
    LICENSE Apache-2.0
    SOURCE_DIR
    src
    SRC android/crashreport/CrashService_common.cpp
    LINUX android/crashreport/CrashService_linux.cpp
    DARWIN android/crashreport/CrashService_darwin.cpp
    WINDOWS android/crashreport/CrashService_windows.cpp
    DEPS android-hw-config breakpad_server curl)
  target_include_directories(android-emu-crash-service PRIVATE src
                             PUBLIC include)

  android_target_link_libraries(
    android-emu-crash-service windows_msvc-x86_64
    PRIVATE # For GetPerformanceInfo in CrashService_windows.cpp
            psapi::psapi dirent-win32)

  android_add_executable(
    TARGET emulator-crash-service
    LICENSE Apache-2.0 SOURCE_DIR
    src
    SRC ${CRASH_WINDOWS_ICON}
        android/crashreport/main-crash-service.cpp
        android/crashreport/ui/ConfirmDialog.cpp
        android/crashreport/ui/ConfirmDialog.h
        android/crashreport/ui/ConfirmDialog.ui
    DEPS android-emu-crash-service breakpad_server emulator-libui Qt5::Gui)

  set_target_properties(emulator-crash-service
                        PROPERTIES OUTPUT_NAME "emulator64-crash-service")
  target_compile_definitions(
    emulator-crash-service
    PRIVATE CONFIG_QT CRASHUPLOAD=${OPTION_CRASHUPLOAD}
            ANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
            ANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER})
  android_install_exe(emulator-crash-service .)
  android_add_executable(
    TARGET emulator64_test_crasher NODISTRIBUTE
    SRC test/android/crashreport/testing/main-test-crasher.cpp)
  target_link_libraries(emulator64_test_crasher PRIVATE libqemu2-glue
                                                        breakpad_server)

  android_add_test(
    TARGET emulator_crashreport_unittests
    SOURCE_DIR test
    SRC android/crashreport/CrashService_unittest.cpp
        android/crashreport/CrashSystem_unittest.cpp
        android/crashreport/detectors/CrashDetectors_unittest.cpp
        android/crashreport/HangDetector_unittest.cpp
    DEPS android-emu-test-launcher breakpad_server curl gtest libqemu2-glue)
  target_include_directories(emulator_crashreport_unittests PRIVATE src)
  # Windows-msvc specific dependencies. Need these for posix support.
  android_target_link_libraries(emulator_crashreport_unittests
                                windows_msvc-x86_64 PUBLIC dirent-win32)

endif()
