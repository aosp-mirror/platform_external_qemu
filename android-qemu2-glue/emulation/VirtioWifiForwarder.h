/* Copyright 2020 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#pragma once

#include <memory>

#include "aemu/base/Compiler.h"
#include "aemu/base/Optional.h"
#include "aemu/base/async/Looper.h"
#include "aemu/base/async/RecurrentTask.h"
#include "aemu/base/sockets/ScopedSocket.h"
#include "aemu/base/synchronization/Lock.h"
#include "android-qemu2-glue/emulation/WifiService.h"
#include "android/emulation/HostapdController.h"
#include "android/network/GenericNetlinkMessage.h"
#include "android/network/Ieee80211Frame.h"
#include "android/network/WifiForwardPeer.h"

typedef struct NetClientState NetClientState;
typedef struct NICState NICState;
typedef struct NICConf NICConf;
typedef struct Slirp Slirp;

namespace android {
namespace qemu2 {

class VirtioWifiForwarder : public WifiService {
public:
    VirtioWifiForwarder(
            const uint8_t* bssid,
            WifiService::OnReceiveCallback cb,
            WifiService::OnLinkStatusChangedCallback onLinkStatusChanged,
            WifiService::CanReceiveCallback canReceive,
            WifiService::OnSentCallback onFrameSentCallback = {},
            NICConf* conf = nullptr,
            Slirp* slirp = nullptr,
            uint16_t serverPort = 0,
            uint16_t clientPort = 0);
    ~VirtioWifiForwarder();
    bool init() override;
    int send(const android::base::IOVector& iov) override;
    int recv(android::base::IOVector& iov) override;
    void stop() override;
    NICState* getNic() override { return mNic; }
#ifndef LIBSLIRP
    android::network::MacAddress getStaMacAddr(const char* ssid) override;
#endif
    ssize_t onRxPacketAvailable(const uint8_t* buf, size_t size);

    static const uint32_t kWifiForwardMagic = 0xD6C4B3A2;
    static const uint8_t kWifiForwardVersion = 0x02;
private:
#ifdef LIBSLIRP
    static void eloopSocketHandler(int sock, void* eloop_ctx, void* sock_ctx);
    static void eloopTimeoutHandler(void* eloop_ctx, void* user_ctx);
#else
    static VirtioWifiForwarder* getInstance(NetClientState* nc);
    // Wrapper functions for passing C-sytle func ptr to struct NetClientInfo
    // defined in net/net.h
    static ssize_t onNICFrameAvailable(NetClientState* nc,
                                       const uint8_t* buf,
                                       size_t size);
    static int canReceive(NetClientState* nc);
    static void onLinkStatusChanged(NetClientState* nc);
    static void onFrameSentCallback(NetClientState*, ssize_t);
#endif
    // Wrapper function for pass C-style func ptr to hostapd socket
    static void onHostApd(void* opaque, int fd, unsigned events);
    static const char* const kNicModel;
    static const char* const kNicName;
    ssize_t sendToGuest(
            std::unique_ptr<android::network::Ieee80211Frame> frame);
    void registerBeaconTask();
    size_t onRemoteData(const uint8_t* data, size_t size);
    void sendToRemoteVM(std::unique_ptr<android::network::Ieee80211Frame> frame,
                        android::network::FrameType type);
    int sendToNIC(std::unique_ptr<android::network::Ieee80211Frame> frame);
    void ackLocalFrame(const android::network::Ieee80211Frame* frame);
    std::unique_ptr<android::network::Ieee80211Frame> parseHwsimCmdFrame(
            const android::network::GenericNetlinkMessage& msg);
    android::network::MacAddress mBssID;
    WifiService::OnReceiveCallback mOnFrameAvailableCallback;
    WifiService::OnLinkStatusChangedCallback mOnLinkStatusChanged;
    WifiService::OnSentCallback mOnFrameSentCallback;
    WifiService::CanReceiveCallback mCanReceive;
    android::base::Looper* mLooper = nullptr;
    // Scoped sockets holding the socket pair.
    android::base::ScopedSocket mVirtIOSock;
    android::base::ScopedSocket mHostapdSock;
    uint16_t mServerPort = 0;
    uint16_t mClientPort = 0;

    Slirp* mSlirp = nullptr;
    NICState* mNic = nullptr;
    std::unique_ptr<android::network::WifiForwardPeer> mRemotePeer;
    android::base::Looper::FdWatch* mFdWatch = nullptr;
    android::emulation::HostapdController* mHostapd = nullptr;
    android::base::Optional<android::base::RecurrentTask> mBeaconTask;
    android::base::Looper::Duration mBeaconIntMs = 1024;
    std::unique_ptr<android::network::Ieee80211Frame> mBeaconFrame;
    std::atomic<bool> mHostapdSockInitSuccess{false};
    NICConf* mNicConf = nullptr;
    android::network::FrameInfo mFrameInfo;
    android::base::Lock mLock;

    DISALLOW_COPY_AND_ASSIGN(VirtioWifiForwarder);
};

}  // namespace qemu2
}  // namespace android
