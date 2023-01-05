# A library that contains harware related things for controlling the gps
prebuilt(LIBXML2)
android_add_library(
  TARGET android-emu-gps
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/gps.cpp android/gps/GpxParser.cpp android/gps/KmlParser.cpp
  android/gps/PassiveGpsUpdater.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-hardware LibXml2::LibXml2)
target_include_directories(android-emu-gps PRIVATE src PUBLIC include)

android_add_test(
  TARGET android-emu-gps_unittests
  SOURCE_DIR test
  SRC android/gps/GpxParser_unittest.cpp android/gps/KmlParser_unittest.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-gps android-emu-hardware gmock gmock_main
  LibXml2::LibXml2)
target_include_directories(android-emu-gps_unittests PRIVATE src test)
