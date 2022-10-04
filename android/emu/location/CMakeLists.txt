android_add_library(
  TARGET android-emu-location
  # SHARED Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR  src
  SRC android/location/MapsKey.cpp android/location/MapsKeyFileParser.cpp
      android/location/Point.cpp android/location/Route.cpp
      android/location/StudioMapsKey.cpp
  DEPS android-emu-curl android-emu-base android-emu-base-headers android-emu-protobuf
       protobuf::libprotobuf)
target_link_libraries(android-emu-location PUBLIC android-emu-protos)
target_include_directories(android-emu-location PRIVATE src PUBLIC include)

android_add_test(
  TARGET android-emu-location_unittests
  SOURCE_DIR test
  SRC android/location/MapsKey_unittest.cpp
      android/location/MapsKeyFileParser_unittest.cpp
      android/location/Point_unittest.cpp android/location/Route_unittest.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-location gtest gtest_main
       protobuf::libprotobuf)
target_include_directories(android-emu-location_unittests PRIVATE src)
