set(android-base-net_src emulator/net/AsyncSocketAdapter.cpp emulator/net/JsonProtocol.cpp emulator/net/SocketTransport.cpp
    emulator/net/waterfall/WaterfallProtocol.cpp)

# A library that is shared between the video bridge & android-webrtc & android-grpc It contains: - A set of socket
# protocols - An AsyncSocketAdapter that is a bridge between android sockets & webrtc sockets.
#
# THIS LIBRARY SHOULD NOT HAVE ANY DEPENDENCIES, AS THIS WILL LIKELY BREAK THE VIDEO BRIDGE.
android_add_library(android-base-net)
target_include_directories(android-base-net PUBLIC .)
target_link_libraries(android-base-net PUBLIC nlohmann_json::nlohmann_json)

set(android-base-net_unittests_src emulator/net/JsonProtocol_unittest.cpp
    emulator/net/waterfall/WaterfallProtocol_unittest.cpp)
android_add_test(android-base-net_unittests)
# Needed for TestAsyncSocket.
target_include_directories(android-base-net_unittests PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android/android-emu)
target_link_libraries(android-base-net_unittests PRIVATE android-base-net gmock_main)
