set(android-webrtc-net_src emulator/net/JsonProtocol.cpp emulator/net/SocketTransport.cpp)

# A library that is shared between the video bridge & android-webrtc
android_add_library(android-webrtc-net)
target_include_directories(android-webrtc-net PUBLIC .)
target_link_libraries(android-webrtc-net PUBLIC nlohmann_json::nlohmann_json)

