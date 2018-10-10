# This file defines emulator crash service
prebuilt(BREAKPAD)
prebuilt(QT5)
prebuilt(CURL)
set(CRASH_WINDOWS_ICON ../images/emulator_icon.rc)
set(emulator-crash-service_src
    android/crashreport/main-crash-service.cpp 
    android/crashreport/CrashService_common.cpp 
    android/crashreport/ui/ConfirmDialog.cpp 
    android/resource.c 
    android/skin/resource.c
#QT Files, these are automatically picked up    
    android/crashreport/ui/ConfirmDialog.ui 
    android/crashreport/ui/ConfirmDialog.h 
# The icon
    ${CRASH_WINDOWS_ICON}
# Make sure we are depending on HW_CONFIG
    ${ANDROID_HW_CONFIG_H}
    )
set(emulator-crash-service_linux-x86_64_src android/crashreport/CrashService_linux.cpp)
set(emulator-crash-service_darwin-x86_64_src android/crashreport/CrashService_darwin.cpp)
set(emulator-crash-service_windows-x86_64_src android/crashreport/CrashService_windows.cpp)
android_add_executable(emulator-crash-service)
set_target_properties(emulator-crash-service PROPERTIES OUTPUT_NAME "emulator64-crash-service")
target_compile_definitions(emulator-crash-service PRIVATE -DCONFIG_QT -DCRASHUPLOAD=${OPTION_CRASHUPLOAD})
target_include_directories(emulator-crash-service PRIVATE ${QT5_INCLUDE_DIRS} ${BREAKPAD_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_libraries(emulator-crash-service PRIVATE android-emu emulator-libui ${BREAKPAD_LIBRARIES} ${CURL_LIBRARIES})

set(emulator64_test_crasher_src android/crashreport/testing/main-test-crasher.cpp)
android_add_executable(emulator64_test_crasher)
target_link_libraries(emulator64_test_crasher PRIVATE android-emu ${BREAKPAD_LIBRARIES})

set(emulator_crashreport_unittests_src
    android/crashreport/CrashService_common.cpp 
    android/crashreport/CrashService_unittest.cpp 
    android/crashreport/CrashSystem_unittest.cpp 
    android/crashreport/HangDetector_unittest.cpp 
)
set(emulator_crashreport_unittests_linux-x86_64_src android/crashreport/CrashService_linux.cpp)
set(emulator_crashreport_unittests_darwin-x86_64_src android/crashreport/CrashService_darwin.cpp)
set(emulator_crashreport_unittests_windows-x86_64_src android/crashreport/CrashService_windows.cpp)

android_add_test(emulator_crashreport_unittests)
target_include_directories(emulator_crashreport_unittests PRIVATE ${BREAKPAD_INCLUDE_DIRS})
target_link_libraries(emulator_crashreport_unittests PRIVATE  android-emu ${BREAKPAD_LIBRARIES} gtest_main)
