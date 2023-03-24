android_add_library(
  TARGET android-emu-studio-config

  # SHARED # Testing only..
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/EmulatorAdvertisement.cpp
  android/metrics/MetricsPaths.cpp android/metrics/StudioConfig.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-files android-emu-utils android-emu-base-process
  LibXml2::LibXml2)
target_include_directories(android-emu-studio-config PUBLIC include PRIVATE src)

# target_compile_definitions(android-emu-studio-config PRIVATE
# AEMU_STUDIO_CONFIG_EXPORTS PUBLIC AEMU_STUDIO_CONFIG_AS_DLL)
android_add_test(
  TARGET android-emu-studio-config_unittests
  SOURCE_DIR test
  SRC android/emulation/control/EmulatorAdvertisement_unittest.cpp
  android/metrics/StudioConfig_unittest.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-files android-emu-studio-config gmock
  gmock_main)
target_include_directories(android-emu-studio-config_unittests PRIVATE src test)

android_add_executable(
  TARGET studio_discovery_tester NODISTRIBUTE SOURCE_DIR test
  SRC android/emulation/control/StudioDiscoveryTester.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-studio-config)
target_include_directories(studio_discovery_tester PRIVATE src test)
add_dependencies(android-emu-studio-config_unittests studio_discovery_tester)
