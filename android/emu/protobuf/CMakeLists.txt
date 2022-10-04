android_add_library(
  TARGET android-emu-protobuf
  # SHARED Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/protobuf/LoadSave.cpp android/protobuf/ProtobufLogging.cpp
  DEPS android-emu-base android-emu-base-headers protobuf::libprotobuf)
target_include_directories(android-emu-protobuf PRIVATE src PUBLIC include)
android_target_link_libraries(android-emu-protobuf windows
                              PRIVATE emulator-libmman-win32)
