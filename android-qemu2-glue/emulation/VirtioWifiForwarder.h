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

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/network/Ieee80211Frame.h"
#include "android/network/WifiForwardPeer.h"

typedef struct NetClientState NetClientState;
typedef struct NICState NICState;
typedef struct NICConf NICConf;

namespace android {
namespace qemu2 {

class VirtioWifiForwarder {
public:
    // The callback function will only provide the clietn with one and only one
    // IEEE80211 frame.
    // The client should return the number of bytes consumed.
    // So if the client is busy or encouter a partial frame (It could happen
    // when communicating with the remote VM, it should return zero.

    using OnFrameAvailableCallback =
            std::function<size_t(const uint8_t*, size_t, bool)>;
    // Used when the outbound frame has been transmitted.
    // It is possible that the network device is busy therefore the client needs
    // to handle retransmission.
    using OnFrameSentCallback = std::function<void(::NetClientState*, ssize_t)>;
    using CanReceive = std::function<int(::NetClientState*)>;
    using OnLinkStatusChanged = std::function<void(::NetClientState*)>;
    VirtioWifiForwarder(const uint8_t* bssid,
                        OnFrameAvailableCallback cb,
                        OnLinkStatusChanged onLinkStatusChanged,
                        CanReceive canReceive,
                        NICConf* conf,
                        OnFrameSentCallback onFrameSentCallback = {});
    bool init();
    int forwardFrame(android::base::IOVector iov, bool toRemoteVM = false);
    void stop();
    NICState* getNic() { return mNic; }
    static VirtioWifiForwarder* getInstance(NetClientState* nc);

private:
    // Wrapper functions for passing C-sytle func ptr to C API.
    static int canReceive(NetClientState* nc);
    static void onLinkStatusChanged(NetClientState* nc);
    static ssize_t onNICFrameAvailable(NetClientState* nc,
                                       const uint8_t* buf,
                                       size_t size);

    static void onFrameSentCallback(NetClientState*, ssize_t);
    static void onHostApd(void* opaque, int fd, unsigned events);

    static const uint32_t kWifiForwardMagic = 0xD6C4B3A2;
    static const uint8_t kWifiForwardVersion = 0x02;
    static const char* const kNicModel;
    static const char* const kNicName;
    size_t onRemoteData(const uint8_t* data, size_t size);
    void sendToRemoteVM(android::network::Ieee80211Frame& frame);
    int sendToNIC(android::network::Ieee80211Frame& frame);
    android::network::MacAddress mBssID;
    OnFrameAvailableCallback mOnFrameAvailableCallback;
    OnLinkStatusChanged mOnLinkStatusChanged;
    OnFrameSentCallback mOnFrameSentCallback;
    CanReceive mCanReceive;
    android::base::Looper* mLooper;
    android::base::ScopedSocket mHostApdSock;
    android::base::ScopedSocket mVirtIOSock;
    NICState* mNic = nullptr;
    std::unique_ptr<android::network::WifiForwardPeer> mRemotePeer;
    android::base::Looper::FdWatch* mFdWatch = nullptr;
    NICConf* mNicConf = nullptr;

    DISALLOW_COPY_AND_ASSIGN(VirtioWifiForwarder);
};

}  // namespace qemu2
}  // namespace android
