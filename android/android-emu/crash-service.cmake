# This file defines emulator crash service
if(NOT LINUX_AARCH64)
  prebuilt(QT5)
  set(CRASH_WINDOWS_ICON ../images/emulator_icon.rc)

  android_add_library(
    TARGET android-emu-crash-service
    LICENSE Apache-2.0
    SRC # cmake-format: sortable
        android/crashreport/CrashService_common.cpp
    LINUX android/crashreport/CrashService_linux.cpp
    DARWIN android/crashreport/CrashService_darwin.cpp
    WINDOWS android/crashreport/CrashService_windows.cpp)

  target_include_directories(android-emu-crash-service
                             PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(
    android-emu-crash-service PRIVATE breakpad_server curl android-emu
                                      android-hw-config)
  # Windows-msvc specific dependencies. Need these for posix support.
  android_target_link_libraries(android-emu-crash-service windows_msvc-x86_64
                                PUBLIC dirent-win32)

  android_add_executable(
    TARGET emulator-crash-service
    LICENSE Apache-2.0
    SRC # cmake-format: sortable
        ${CRASH_WINDOWS_ICON}
        android/crashreport/main-crash-service.cpp
        android/crashreport/ui/ConfirmDialog.cpp
        android/crashreport/ui/ConfirmDialog.h
        android/crashreport/ui/ConfirmDialog.ui
        android/resource.c
        android/skin/resource.c)

  set_target_properties(emulator-crash-service
                        PROPERTIES OUTPUT_NAME "emulator64-crash-service")
  target_compile_definitions(
    emulator-crash-service
    PRIVATE -DCONFIG_QT -DCRASHUPLOAD=${OPTION_CRASHUPLOAD}
            -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
            -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER})
  target_link_libraries(
    emulator-crash-service PRIVATE android-emu-crash-service android-emu
                                   emulator-libui breakpad_server Qt5::Gui)

  android_install_exe(emulator-crash-service .)
  android_add_executable(
    TARGET emulator64_test_crasher NODISTRIBUTE
    SRC # cmake-format: sortable
        android/crashreport/testing/main-test-crasher.cpp)
  target_link_libraries(emulator64_test_crasher
                        PRIVATE android-emu libqemu2-glue breakpad_server)

  android_add_test(
    TARGET emulator_crashreport_unittests
    SRC # cmake-format: sortable
        android/crashreport/CrashService_common.cpp
        android/crashreport/CrashService_unittest.cpp
        android/crashreport/CrashSystem_unittest.cpp
        android/crashreport/detectors/CrashDetectors_unittest.cpp
        android/crashreport/HangDetector_unittest.cpp
    LINUX android/crashreport/CrashService_linux.cpp
    DARWIN android/crashreport/CrashService_darwin.cpp
    MSVC android/crashreport/CrashService_windows.cpp)
  target_link_libraries(
    emulator_crashreport_unittests PRIVATE android-emu libqemu2-glue
                                           breakpad_server gtest_main)
  target_include_directories(emulator_crashreport_unittests PRIVATE .)
  # Windows-msvc specific dependencies. Need these for posix support.
  android_target_link_libraries(emulator_crashreport_unittests
                                windows_msvc-x86_64 PUBLIC dirent-win32)

endif()
