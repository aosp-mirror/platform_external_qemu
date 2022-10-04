android_add_library(
  TARGET android-emu-avd
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC ${ANDROID_HW_CONFIG_H}
      android/avd/generate.cpp
      android/avd/hw-config.c
      android/avd/info.c
      android/avd/scanner.c
      android/avd/util.c
      android/avd/util_wrapper.cpp
      android/hw-lcd.c
      android/multi-instance.cpp
      android/user-config.cpp
  DEPS android-emu-agents android-emu-base android-emu-base-headers android-emu-cmdline
       android-emu-files android-emu-utils)

# You usually want ot read the hardware configuration, so lets make it public.
target_link_libraries(android-emu-avd PUBLIC android-hw-config)
target_include_directories(
  android-emu-avd PRIVATE src PUBLIC include
                                     ${CMAKE_CURRENT_BINARY_DIR}/avd_config)

target_compile_options(android-emu-avd PRIVATE -Wno-extern-c-compat)

# A minimal avd lib that cannot do everything.
android_add_library(
  TARGET android-emu-min-avd
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC ${ANDROID_HW_CONFIG_H}
      android/avd/hw-config.c
      android/avd/info.c
      android/avd/scanner.c
      android/avd/util.c
      android/avd/util_wrapper.cpp
      android/hw-lcd.c
  DEPS android-emu-agents-headers android-emu-base android-emu-base-headers android-emu-cmdline
       android-emu-files android-emu-utils android-hw-config)
target_include_directories(
  android-emu-min-avd PRIVATE src PUBLIC include
                                         ${CMAKE_CURRENT_BINARY_DIR}/avd_config)
target_compile_definitions(android-emu-min-avd PRIVATE AEMU_MIN AEMU_LAUNCHER)

android_add_test(
  TARGET android-emu-avd_unittests
  SOURCE_DIR test
  SRC android/avd/util_unittest.cpp android/avd/util_wrapper_unittest.cpp
      android/hw-lcd_unittest.cpp
  DEPS android-emu-avd android-emu-base android-emu-base-headers android-emu-files gmock_main)
