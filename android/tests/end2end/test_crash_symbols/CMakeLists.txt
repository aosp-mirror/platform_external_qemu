# Only schedule the test if we are not cross compiling and have symbols enabled.
# currently disabled due to not working on GCE
# if(ANDROID_EXTRACT_SYMBOLS
#    AND ANDROID_TARGET_TAG STREQUAL ANDROID_HOST_TAG
#    AND (NOT WINDOWS_MSVC_X86_64) # b/126964544
#    AND (NOT OPTION_CRASHUPLOAD STREQUAL "NONE"))
#
#   get_filename_component(PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/testdata"
#                          ABSOLUTE)
#
#   # Copy test dependencies.
#   set(ANDROID_SDK_ROOT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testdata/test-sdk)
#   set(ANDROID_AVD_HOME ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testdata/avd-home)
#
#   file(COPY ${PREBUILT_ROOT}/test-sdk/ DESTINATION ${ANDROID_SDK_ROOT})
#   file(COPY ${PREBUILT_ROOT}/dot-android/avd/armeabi-v7a.ini DESTINATION ${ANDROID_AVD_HOME}/avd)
#
#   configure_file(${PREBUILT_ROOT}/dot-android/avd/armeabi-v7a.avd/config.ini
#                  ${ANDROID_AVD_HOME}/avd/armeabi-v7a.avd/config.ini)
#
#   add_test(NAME proper_symbols_test
#            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_crash_symbols.py
#                    --emulator $<TARGET_FILE:emulator>
#                    --stackwalker $<TARGET_FILE:minidump_stackwalk>
#                    --sdk_root ${ANDROID_SDK_ROOT}
#                    --avd_root ${ANDROID_AVD_HOME}
#                    --symbol_src ${ANDROID_SYMBOL_DIR}
#                    --avd armeabi-v7a
#                    --args "-no-window,-no-accel,-no-snapshot,-verbose,-show-kernel,-qemu,-no-reboot,-append,kernel.panic=-1"
#             TIMEOUT 60)
#   set_tests_properties(proper_symbols_test PROPERTIES DEPENDS emulator
#                                                       DEPENDS minidump_stackwalk)
# endif()
