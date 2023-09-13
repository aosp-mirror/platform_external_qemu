# A library that contains harware related things
prebuilt(LIBXML2)
android_add_library(
  TARGET android-emu-update-check
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/update-check/UpdateChecker.cpp
      android/update-check/VersionExtractor.cpp
  DEPS android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-curl
       android-emu-files
       android-emu-studio-config
       android-emu-utils
      #  android-hw-config
       LibXml2::LibXml2)
target_include_directories(
  android-emu-update-check
  PRIVATE
    # TODO(jansene): Move to an interface lib.  For access to current
    # config-host.
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG} src
  PUBLIC include)
target_compile_options(android-emu-update-check PRIVATE -Wno-extern-c-compat)


target_compile_definitions(
  android-emu-update-check
  PRIVATE "-DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}"
          "-DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}")

android_add_test(
  TARGET android-emu-update-check_unittests
  SOURCE_DIR test
  SRC android/update-check/UpdateChecker_unittest.cpp
      android/update-check/VersionExtractor_unittest.cpp
  DEPS android-emu-base
       android-emu-base-headers
       android-emu-hardware
       android-emu-studio-config
       android-emu-update-check
       android-emu-utils
       gmock
       gmock_main)
target_include_directories(android-emu-update-check_unittests PRIVATE src test)
