set(android-net_src emulator/net/JsonProtocol.cpp emulator/net/SocketTransport.cpp)

# A library that is shared between the video bridge & android-webrtc
android_add_library(android-net)
target_include_directories(android-net PUBLIC .)
target_link_libraries(android-net PUBLIC nlohmann_json::nlohmann_json)

