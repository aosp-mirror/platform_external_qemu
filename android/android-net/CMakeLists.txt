# A library that is shared between the video bridge & android-webrtc
android_add_library(
  TARGET android-net LICENSE Apache-2.0
  SRC # cmake-format: sortable
      emulator/net/JsonProtocol.cpp emulator/net/SocketTransport.cpp
  DEPS android-emu-base-headers)

target_include_directories(android-net PUBLIC .)
target_link_libraries(android-net PUBLIC nlohmann_json::nlohmann_json)
add_subdirectory(android)
