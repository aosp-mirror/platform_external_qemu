android_add_library(
  TARGET android-emu-net LICENSE Apache-2.0
  SRC # cmake-format: sortable
	    emulation/HostapdController.cpp
      network/GenericNetlinkMessage.cpp
      network/Ieee80211Frame.cpp
      network/NetworkPipe.cpp
      network/WifiForwardClient.cpp
      network/WifiForwardPeer.cpp
      network/WifiForwardPipe.cpp
      network/WifiForwardServer.cpp
)

target_link_libraries(android-emu-net PRIVATE
      android-emu-base-headers
      android-emu-base
      android-emu-utils
      hostapd)

target_include_directories(android-emu-net PUBLIC ..)
