
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

//#define WEBRTC_ANDROID 1
//#define WEBRTC_IOS 1
#define WEBRTC_LINUX 1
// #define WEBRTC_MAC 1
#define WEBRTC_POSIX 1
//#define WEBRTC_WIN 1

#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/api/test/fakeconstraints.h>
#include <webrtc/base/flags.h>
#include <webrtc/base/physicalsocketserver.h>
#include <webrtc/base/ssladapter.h>
#include <webrtc/base/thread.h>


rtc::Thread* thread;
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
webrtc::PeerConnectionInterface::RTCConfiguration configuration;
rtc::PhysicalSocketServer socket_server;

void webrtc_start_ice_start() {
  std::cout << std::this_thread::get_id() << ":"
            << "RTC thread" << std::endl;
  peer_connection_factory = webrtc::CreatePeerConnectionFactory();
  if (peer_connection_factory.get() == nullptr) {
    std::cout << "Error on CreatePeerConnectionFactory." << std::endl;
    return;
  }

  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = "stun:stun.l.google.com:19302";
  configuration.servers.push_back(ice_server);
}

