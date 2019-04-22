set(android-webrtc_src android/emulation/control/WebRtcBridge.cpp)

# Let's add in the library
android_add_library(android-webrtc)
target_include_directories(android-webrtc PUBLIC .)
target_compile_definitions(android-webrtc PUBLIC -DANDROID_WEBRTC)
target_link_libraries(android-webrtc PRIVATE android-emu-base nlohmann_json::nlohmann_json)

