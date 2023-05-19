// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "aemu/base/Compiler.h"
#include "aemu/base/IOVector.h"
#include "android-qemu2-glue/emulation/export.h"
#include "android/network/MacAddress.h"

#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif

#include <functional>
#include <memory>
#include <cstring>
#include <string>


typedef struct NetClientState NetClientState;
typedef struct NICState NICState;
typedef struct NICConf NICConf;

namespace android {
namespace qemu2 {

struct SlirpOptions {
    bool disabled;
    bool ipv4 = true;
    bool restricted = false;
    std::string vnet;
    std::string vhost;
    std::string vmask;
    bool ipv6 = true;
    std::string vprefix6;
    uint8_t vprefixLen;
    std::string vhost6;
    std::string vhostname;
    std::string tftpath;
    std::string bootfile;
    std::string dhcpstart;
    std::string dns;
    std::string dns6;
};

struct HostapdOptions {
    bool disabled;
    std::string ssid;
    std::string passwd;
};

/**
 * Wifi Service provisions network connectivity without necessarily
 * having a real WiFi network card on host OS. The underlying network
 * stack is supported by SLIRP library in QEMU and the virtual router
 * is supported by hostapd.
 * When redirect-to-netsim option is provided, Wifi Service will
 * establish a bi-directional gRPC connection to the external netsim
 * process and forward WiFi packets via the gRPC connection.
 *
 * For netsim process, to provision connectivity in its code:
 * auto wifiSvc = WifiService::Builder().build;
 * wifiSvc.init();
 */

class WifiService {
public:
    class Builder;
    using OnReceiveCallback =
            std::function<ssize_t(const uint8_t*, size_t)>;
    // Used when the outbound frame has been transmitted.
    // It is possible that the network device is busy therefore the client needs
    // to handle retransmission.
    using OnSentCallback = std::function<void(size_t)>;
    using CanReceiveCallback =
            std::function<int(size_t)>;
    using OnLinkStatusChangedCallback =
            std::function<void(::NetClientState*)>;
    virtual ~WifiService() = default;
    AEMU_WIFI_API virtual bool init() = 0;
    AEMU_WIFI_API virtual int send(const android::base::IOVector& iov) = 0;
    AEMU_WIFI_API virtual int recv(android::base::IOVector& iov) = 0;
    AEMU_WIFI_API virtual void stop() = 0;
    // Specific details for virtio wifi driver. No necessary for netsim.
    AEMU_WIFI_API virtual NICState* getNic() = 0;
    AEMU_WIFI_API virtual android::network::MacAddress getStaMacAddr(
            const char* ssid) = 0;
};

class WifiService::Builder {
public:
    AEMU_WIFI_API Builder();
    // Set to true if the network traffic is redirected to the external netsim
    // process.
    // Default value false.
    AEMU_WIFI_API Builder& withRedirectToNetsim(bool enable);
    // WiFi Service should launch hostapd thread if disabled field is false
    // In default emulator setup, Hostapd thread is launched in QT
    // main loop thread.
    AEMU_WIFI_API Builder& withHostapd(HostapdOptions options);
    // WiFi service should initialize user-mode network if disabled field is
    // false stack SLIPR. In default emulator setup, SLIRP is initialized in
    // Qemu main loop thread.
    AEMU_WIFI_API Builder& withSlirp(SlirpOptions options);
    // Set the bssid of the virtual router. The default value is
    // 00:13:10:85:fe:01 which is defined in hostapd.conf configuration file.
    AEMU_WIFI_API Builder& withBssid(std::vector<uint8_t> bssid);
    // Used by emulator's virtio implementation only.
    AEMU_WIFI_API Builder& withNicConf(NICConf* conf);
    AEMU_WIFI_API Builder& withVerboseLogging(bool verbose);
    // The server port and client port are used in WiFi Direct mode
    // when two AVDs are communication via a TCP loopback server.
    AEMU_WIFI_API Builder& withServerPort(uint16_t port);
    AEMU_WIFI_API Builder& withClientPort(uint16_t port);
    // The callback functions are only used by emulator's virtio
    // implementation because they contain QEMU specific details.
    AEMU_WIFI_API Builder& withOnReceiveCallback(
            WifiService::OnReceiveCallback cb);
    AEMU_WIFI_API Builder& withOnSentCallback(WifiService::OnSentCallback cb);
    AEMU_WIFI_API Builder& withCanReceiveCallback(
            WifiService::CanReceiveCallback cb);
    AEMU_WIFI_API Builder& withOnLinkStatusChangedCallback(
            WifiService::OnLinkStatusChangedCallback cb);
    // Returns the fully configured and running service, or nullptr if
    // construction failed.
    std::shared_ptr<WifiService> build();

private:
    bool mRedirectToNetsim = false;
    HostapdOptions mHostapdOpts;
    SlirpOptions mSlirpOpts;
    bool mVerbose = false;
    std::vector<uint8_t> mBssID;
    NICConf* mNicConf = nullptr;
    uint16_t mServerPort = 0;
    uint16_t mClientPort = 0;
    WifiService::OnLinkStatusChangedCallback mOnLinkStatusChanged;
    WifiService::OnSentCallback mOnSentCallback;
    WifiService::CanReceiveCallback mCanReceive;
};

}  // namespace qemu2
}  // namespace android