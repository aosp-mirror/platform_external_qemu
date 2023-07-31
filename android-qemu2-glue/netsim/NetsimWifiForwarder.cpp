/* Copyright 2023 The Android Open Source Project
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
#include "android-qemu2-glue/netsim/NetsimWifiForwarder.h"


#include "aemu/base/logging/CLog.h"
#include "android/emulation/HostapdController.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "backend/packet_streamer_client.h"
#include "netsim/packet_streamer.grpc.pb.h"
#include "netsim.h"

#include <assert.h>            // for assert
#include <grpcpp/grpcpp.h>     // for Clie...
#include <condition_variable>  // for cond...
#include <cstring>             // for NULL
#include <mutex>               // for cond...
#include <optional>            // for opti...

#include <random>
#include <string>  // for string
#include <vector>  // for vector

using namespace grpc;
using android::network::MacAddress;
using netsim::packet::PacketRequest;
using netsim::packet::PacketResponse;
using netsim::packet::PacketStreamer;

#define DEBUG 0
/* set  for very verbose debugging */
#if DEBUG < 1
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("Channel %s (%p):", __func__, this);     \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)

#endif

namespace android {
namespace qemu2 {

class NetsimWifiTransport;

// Default RX queue used by the virtio wifi driver.
static int kDefaultQueueIdx = 0;
static std::unique_ptr<NetsimWifiTransport> sTransport = nullptr;
static std::unique_ptr<PacketStreamer::Stub> sTransportStub = nullptr;

class NetsimWifiTransport
    : public SimpleClientBidiStream<PacketResponse, PacketRequest> {
public:
    NetsimWifiTransport(WifiService::OnReceiveCallback onRecv,
                        WifiService::CanReceiveCallback canReceive)
        : mOnRecv(onRecv), mCanReceive(canReceive) {}
    ~NetsimWifiTransport() {}
    void Read(const PacketResponse* received) override {
        DD("Forwarding %s -> %s ", mContext.peer().c_str(),
           received->ShortDebugString().c_str());
        if (received->has_packet()) {
            auto packet = received->packet();
            if (mCanReceive(kDefaultQueueIdx)) {
                mOnRecv(reinterpret_cast<const uint8_t*>(packet.c_str()),
                        packet.size());
            }
        } else {
            dwarning("Unexpected packet %s", received->DebugString().c_str());
        }
    }

    void OnDone(const grpc::Status& s) override {
        dwarning("Netsim Wifi %s is gone due to %s", mContext.peer().c_str(),
                 s.error_message().c_str());
    }

    // Blocks and waits until this connection has completed.
    grpc::Status await() {
        std::unique_lock<std::mutex> lock(mAwaitMutex);
        mAwaitCondition.wait(lock, [this] { return mDone; });
        return std::move(mStatus);
    }

    void cancel() { mContext.TryCancel(); }

    void startCall(PacketStreamer::Stub* stub) {
        stub->async()->StreamPackets(&mContext, this);
        StartCall();
        StartRead();
    };

protected:
    grpc::ClientContext mContext;

private:
    WifiService::OnReceiveCallback mOnRecv;
    WifiService::CanReceiveCallback mCanReceive;
    std::mutex mAwaitMutex;
    std::condition_variable mAwaitCondition;
    grpc::Status mStatus;
    bool mDone = false;
};

NetsimWifiForwarder::NetsimWifiForwarder(
        WifiService::OnReceiveCallback onRecv,
        WifiService::CanReceiveCallback canReceive)
    : mOnRecv(onRecv), mCanReceive(canReceive) {}

NetsimWifiForwarder::~NetsimWifiForwarder() {
    stop();
}

bool NetsimWifiForwarder::init() {
    auto channel = netsim::packet::CreateChannel("");
    if (!channel) {
        dwarning("Unable to establish gRPC connection with netsim.");
        return false;
    }
    sTransportStub = PacketStreamer::NewStub(channel);

    sTransport = std::make_unique<NetsimWifiTransport>(mOnRecv, mCanReceive);
    sTransport->startCall(sTransportStub.get());

    PacketRequest initial_request;
    initial_request.mutable_initial_info()->set_name(get_netsim_device_name());
    initial_request.mutable_initial_info()->mutable_chip()->set_kind(
        netsim::common::ChipKind::WIFI);
    sTransport->Write(initial_request);
    auto* hostapd = android::emulation::HostapdController::getInstance();
    if (hostapd)
        hostapd->terminate();
    dinfo("Successfully initialized netsim WiFi");
    return true;
}

int NetsimWifiForwarder::send(const android::base::IOVector& iov) {
    if (!sTransport) {
        return 0;
    }
    PacketRequest toSend;
    size_t size = iov.summedLength();
    std::string data(size, 0);
    iov.copyTo(data.data(), 0, size);
    toSend.set_packet(std::move(data));
    sTransport->Write(toSend);
    return size;
}

void NetsimWifiForwarder::stop() {
    if (sTransport != nullptr)
        sTransport->cancel();
}

int NetsimWifiForwarder::recv(android::base::IOVector& iov) {
    // Not implemented because virtio wifi driver uses recv callback
    return 0;
}

}  // namespace qemu2
}  // namespace android
