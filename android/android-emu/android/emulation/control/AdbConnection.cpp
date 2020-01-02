// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/AdbConnection.h"

#include <memory>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "android/android.h"
#include "android/base/async/AsyncSocket.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/QueueStreambuf.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/control/AdbAuthentication.h"
#include "android/emulation/control/AdbInterface.h"

/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("AdbConnection: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_PACKET(pkt)                                                        \
    do {                                                                      \
        const char* buf = reinterpret_cast<const char*>(&pkt);                \
        printf("AdbConnection: %s:%d %d, %d, %d = %lu\n", __func__, __LINE__, \
               pkt.msg.arg0, pkt.msg.arg1, pkt.msg.data_length,               \
               pkt.payload.size());                                           \
        for (int x = 0; x < sizeof(apacket); x++) {                           \
            if (isprint((int)buf[x]))                                         \
                printf("%c", buf[x]);                                         \
            else                                                              \
                printf("[0x%02x]", 0xff & (int)buf[x]);                       \
        }                                                                     \
        printf("\n");                                                         \
    } while (0)

#else
#define DD(...) (void)0
#define DD_PACKET(...) (void)0
#endif

namespace android {
namespace emulation {

using android::base::AsyncSocket;
using android::base::ConditionVariable;
using android::base::Lock;
using emulator::net::AsyncSocketAdapter;

constexpr size_t MAX_PAYLOAD_V1 = 4 * 1024;
constexpr size_t MAX_PAYLOAD = 1024 * 1024;
constexpr size_t RECV_BUFFER_SIZE = 512;

enum class AdbWireMessage {
    A_SYNC = 0x434e5953,
    A_CNXN = 0x4e584e43,
    A_AUTH = 0x48545541,
    A_OPEN = 0x4e45504f,
    A_OKAY = 0x59414b4f,
    A_CLSE = 0x45534c43,
    A_WRTE = 0x45545257,
};

// ADB protocol version.
// Version revision:
// 0x01000000: original
// 0x01000001: skip checksum (Dec 2017)
#define A_VERSION_MIN 0x01000000
#define A_VERSION_SKIP_CHECKSUM 0x01000001
#define A_VERSION 0x01000001

#define ADB_AUTH_TOKEN 1
#define ADB_AUTH_SIGNATURE 2
#define ADB_AUTH_RSAPUBLICKEY 3

struct amessage {
    uint32_t command;     /* command identifier constant      */
    uint32_t arg0;        /* first argument                   */
    uint32_t arg1;        /* second argument                  */
    uint32_t data_length; /* length of payload (0 is allowed) */
    uint32_t data_check;  /* checksum of data payload         */
    uint32_t magic;       /* command ^ 0xffffffff             */
} __attribute__((packed));

struct apacket {
    amessage msg;
    std::vector<char> payload;
};

//  Human readable logging.
template <typename tstream>
tstream& operator<<(tstream& out, const AdbState value) {
    const char* s = 0;
#define STATE(p)        \
    case (AdbState::p): \
        s = #p;         \
        break;
    switch (value) {
        STATE(disconnected);
        STATE(connecting);
        STATE(authorizing);
        STATE(connected);
        STATE(failed);
    }
#undef STATE
    return out << s;
}

static int s_adb_port = 0;

//  Human readable logging.
template <typename tstream>
tstream& operator<<(tstream& out, const AdbWireMessage value) {
    const char* s = 0;
#define STATE(p)              \
    case (AdbWireMessage::p): \
        s = #p;               \
        break;
    switch (value) {
        STATE(A_SYNC);
        STATE(A_CNXN);
        STATE(A_AUTH);
        STATE(A_OPEN);
        STATE(A_OKAY);
        STATE(A_CLSE);
        STATE(A_WRTE);
    };
#undef STATE
    return out << s;
}

class AdbConnectionImpl;

// A streambuf that drives the adb stream.
// Drives the read/write protocol for standard streams.
//
// See
// https://android.googlesource.com/platform/system/core/+/master/adb/protocol.txt
// for details or https://github.com/cstyan/adbDocumentation for more in depth
// details. Currently this stream does not support pull/push.
class AdbStreambuf : public std::streambuf {
public:
    AdbStreambuf(int clientId, AdbConnectionImpl* con);
    ~AdbStreambuf();

    // Closes down the stream properly
    void close();

    // Called when a ready message has been received.
    void unlock();

    // Opens a stream to the given service id, starting
    // the hand shake protocol
    void open(const std::string& id);

    // True if the connection is not closed
    bool isOpen() { return mOpen; }

    // Used to set the remote id to which this stream is connected.
    void setRemoteId(int id) { mRemoteId = id; };

    // Called when we received bytes from the remote id.
    void receive(const std::vector<char>& msg);

protected:
    // Overrides needed for working std::streambuf

    // Writes are basically forwarded to the AdbConnection
    std::streamsize xsputn(const char* s, std::streamsize n) override;

    // Reads are coming from the inner blocking queue, incoming WRTE messages
    // end up in this buffer.
    std::streamsize showmanyc() override { return mReceivingQueue.in_avail(); };
    std::streamsize xsgetn(char* s, std::streamsize n) override {
        return mReceivingQueue.sgetn(s, n);
    };

private:
    // Blocks and waits returning true when we can write.
    bool readyForWrite();

    int mClientId;  // Stream ID our side.
    int mRemoteId;  // Stream ID on the ADB side.
    bool mReady = false;
    bool mOpen = true;
    AdbConnectionImpl* mConnection;

    base::QueueStreambuf
            mReceivingQueue;  // Incoming bytes live in this stream.

    // You cannot write to adb until you have received an OKAY message.
    // These locks help us waiting for receipt of such a message.
    Lock mWriterLock;
    ConditionVariable mWriterCV;
};

// This is basically a std::iostream with using the adbstream buffer
class BasicAdbStream : public AdbStream {
public:
    BasicAdbStream(AdbStreambuf* buf) : AdbStream(buf) {}

    ~BasicAdbStream() {
        close();
        delete adbbuf();
    }

    void close() override { adbbuf()->close(); };

    AdbStreambuf* adbbuf() { return reinterpret_cast<AdbStreambuf*>(rdbuf()); }
};

// This class manages the stream multiplexing and auth to the adbd deamon.
// This uses async i/o an ThreadLooper thread.
class AdbConnectionImpl : public AdbConnection,
                          public emulator::net::AsyncSocketEventListener {
public:
    AdbConnectionImpl() {
        base::Looper* looper = android::base::ThreadLooper::get();
        mSocket = std::unique_ptr<AsyncSocket>(
                new AsyncSocket(looper, s_adb_port));
        mSocket->setSocketEventListener(this);
        mSocket->connect();
    }

    ~AdbConnectionImpl() {
        DD("~AdbConnectionImpl");
        closeStreams();
        mSocket->close();
    }

    // Maximum package size we can send to adbd
    uint32_t getMaxPayload() { return mMaxPayloadSize; }

    AdbState state() const override { return mState; }

    // Calculates the packet checksum
    uint32_t chksum(const apacket& p) {
        uint32_t sum = 0;
        for (size_t i = 0; i < p.msg.data_length; ++i) {
            sum += static_cast<uint8_t>(p.payload[i]);
        }
        return sum;
    }

    // Finalizes and sends out the packet. Setting field information
    // as needed.
    void send(apacket& packet) {
        packet.msg.data_length = packet.payload.size();
        packet.msg.magic = packet.msg.command ^ 0xffffffff;

        if (mProtocolVersion >= A_VERSION_SKIP_CHECKSUM) {
            packet.msg.data_check = 0;  // Newer version don't need it.
        } else {
            packet.msg.data_check = chksum(packet);
        }
        DD_PACKET(packet);
        mSocket->send(reinterpret_cast<char*>(&packet.msg), sizeof(packet.msg));
        if (packet.msg.data_length > 0) {
            mSocket->send(packet.payload.data(), packet.payload.size());
        }
    }

    // Opens up an adb stream to the given service..
    std::shared_ptr<AdbStream> open(const std::string& id) override {
        android::base::AutoLock lock(mActiveStreamsLock);
        mNextClientId++;

        // TODO(jansene): Do we need push/pull support? I
        auto buf = new AdbStreambuf(mNextClientId, this);
        auto stream = std::make_shared<BasicAdbStream>(buf);
        mActiveStreams[mNextClientId] = stream;
        buf->open(id);
        return stream;
    }

    void handlePacket(const apacket& msg) {
        DD_PACKET(msg);
        AdbWireMessage cmd = static_cast<AdbWireMessage>(msg.msg.command);
        switch (cmd) {
            case AdbWireMessage::A_CNXN:
                handleConnect(msg);
                break;
            case AdbWireMessage::A_AUTH:
                sendAuth(msg);
                break;
            case AdbWireMessage::A_OKAY:
                handleOkay(msg);
                break;
            case AdbWireMessage::A_CLSE:
                handleClose(msg);
                break;
            case AdbWireMessage::A_WRTE:
                handleWrite(msg);
                break;
            default:
                LOG(WARNING) << "Don't know how to handle packet of type: "
                             << msg.msg.command;
        }
    }

    void handleConnect(const apacket& msg) {
        std::string banner =
                std::string(msg.payload.begin(), msg.payload.end());
        LOG(INFO) << "Connected: version: " << msg.msg.arg0
                  << ", msg size: " << msg.msg.arg1 << ", banner: " << banner;

        setState(AdbState::connected);
        mProtocolVersion = msg.msg.arg0;
        mMaxPayloadSize = msg.msg.arg1;

        // Now split out the connection string.
        // "device::ro.product.name=x;ro.product.model=y;ro.product.device=z;features="
        // we only care about the feature set, if any.
        mFeatures.clear();
        const std::string features = "features=";
        size_t start = banner.find("features");
        if (start != std::string::npos) {
            // Features are , separated..
            start += features.size();
            size_t end = banner.find(',', start);
            size_t endFeatureList = banner.find(';', start);
            while (end != std::string::npos &&
                   (endFeatureList == std::string::npos ||
                    end < endFeatureList)) {
                mFeatures.emplace(banner.substr(start, end - start));
                start = end + 1;
                end = banner.find(',', start);
            }
            // And the last one..
            endFeatureList = banner.find(';', start);
            mFeatures.emplace(banner.substr(start, endFeatureList));
        }
    }

    void setState(const AdbState newState) {
        LOG(VERBOSE) << "Adb transition " << mState << " -> " << newState;
        mState = newState;
    }

    bool hasFeature(const std::string& feature) const override {
        return mFeatures.count(feature) == 1;
    }

    // Redirect write to the proper stream.
    void handleWrite(const apacket& msg) {
        base::AutoLock lock(mActiveStreamsLock);
        auto clientId = msg.msg.arg1;
        if (mActiveStreams.count(clientId) == 0) {
            DD("Stream %d no longer active", clientId);
            return;
        }
        auto stream = mActiveStreams[clientId]->adbbuf();
        stream->receive(msg.payload);
    }

    void handleClose(const apacket& msg) {
        base::AutoLock lock(mActiveStreamsLock);
        auto clientId = msg.msg.arg1;
        if (mActiveStreams.count(clientId) == 0) {
            DD("Stream %d no longer active", clientId);
            return;
        }

        auto stream = mActiveStreams[clientId];
        mActiveStreams.erase(clientId);
        stream->close();
    }

    void handleOkay(const apacket& msg) {
        base::AutoLock lock(mActiveStreamsLock);
        auto clientId = msg.msg.arg1;
        if (mActiveStreams.count(clientId) == 0) {
            DD("Stream %d no longer active", msg.msg.arg0);
            return;
        }

        // Unblock and set matching remote.
        auto stream = mActiveStreams[clientId]->adbbuf();
        stream->setRemoteId(msg.msg.arg0);
        stream->unlock();
    }

    void sendPacket(AdbWireMessage command,
                    uint32_t arg0,
                    uint32_t arg1,
                    std::vector<char>&& payload) {
        apacket pack{.msg = {.command = (uint32_t)command,
                             .arg0 = arg0,
                             .arg1 = arg1},
                     .payload = payload};
        send(pack);
    }

    void sendClose(int clientId, int remoteId) {
        sendPacket(AdbWireMessage::A_CLSE, clientId, remoteId, {});
    }

    void sendOkay(int clientId, int remoteId) {
        sendPacket(AdbWireMessage::A_OKAY, clientId, remoteId, {});
    }

    void sendCnxn() {
        setState(AdbState::connecting);
        sendPacket(AdbWireMessage::A_CNXN, A_VERSION, MAX_PAYLOAD, {});
    }

    void sendAuth(const apacket& recv) {
        // Bail out if we tried a few times to auth and it failed..
        if (mAuthAttempts == 0) {
            setState(AdbState::failed);
            mSocket->close();
            return;
        }

        setState(AdbState::authorizing);
        mAuthAttempts--;
        int sigLen = 256;
        std::vector<char> signed_token{};
        signed_token.resize(256);
        if (!sign_auth_token((const char*)recv.payload.data(),
                             recv.msg.data_length, signed_token.data(),
                             sigLen)) {
            LOG(ERROR) << "Unable to sign token.";
        }
        signed_token.resize(sigLen);
        sendPacket(AdbWireMessage::A_AUTH, ADB_AUTH_SIGNATURE, 0,
                   std::move(signed_token));
    }

    void onRead(AsyncSocketAdapter* socket) override {
        // Read the amessage header..
        while (mPacketOffset < sizeof(amessage)) {
            char* pack = reinterpret_cast<char*>(&mIncoming.msg);
            pack += mPacketOffset;
            int want = sizeof(amessage) - mPacketOffset;
            int rd = socket->recv(pack, sizeof(amessage) - mPacketOffset);
            if (rd <= 0) {
                // closed stream or errno..
                return;
            };
            mPacketOffset += rd;
            if (mPacketOffset == sizeof(amessage)) {
                mIncoming.payload.resize(mIncoming.msg.data_length);
            }
        }

        // Read the payload..
        int dataOffset = mPacketOffset - sizeof(amessage);
        while (dataOffset < mIncoming.msg.data_length) {
            int needed = mIncoming.msg.data_length - dataOffset;
            char* data = mIncoming.payload.data() + dataOffset;
            int rd = socket->recv(data, needed);
            if (rd <= 0) {
                // closed stream or errno.. (like EAGAIN)
                return;
            };
            mPacketOffset += rd;
            dataOffset += rd;
        }

        mPacketOffset = 0;
        handlePacket(mIncoming);
    }

    // Called when this socket (re-)establishes a connection
    // it kicks of the auth thread. The emulator makes adb
    // available early on, but it might take a while before
    // adbd is ready in the emulator.
    void onConnected(AsyncSocketAdapter* socket) override {
        DD("Sock open");
        mAuthAttempts = 8;
        mOpenerThread.reset(new base::FunctorThread([this] {
            // Try until we reach the failure state..
            // This relies on ADBD coming up inside the emulator.
            while (mState == AdbState::disconnected) {
                this->sendCnxn();
                base::Thread::sleepMs(500);
            }
        }));
        mOpenerThread->start();
    };

    void closeStreams() {
        base::AutoLock lock(mActiveStreamsLock);
        for (auto& entry : mActiveStreams) {
            entry.second->close();
        }
        mActiveStreams.clear();
    }

    // Called when this socket is closed.
    void onClose(AsyncSocketAdapter* socket, int err) override {
        closeStreams();
        setState(AdbState::disconnected);
    };

    void close() override {
        closeStreams();
        mSocket->close();
    }

private:
    std::unique_ptr<AsyncSocket> mSocket;          // Socket to adbd in emulator
    AdbState mState{AdbState::disconnected};       // Current connection state
    apacket mIncoming{.msg = {0}, .payload = {}};  // Incoming packet
    size_t mPacketOffset{0};  // Received bytes of packet so far

    std::unique_ptr<base::FunctorThread>
            mOpenerThread;                      // Initial connector thread
    std::unordered_set<std::string> mFeatures;  // available features

    // Stream management
    std::unordered_map<int32_t, std::shared_ptr<BasicAdbStream>> mActiveStreams;
    Lock mActiveStreamsLock;

    uint32_t mNextClientId{1};  // Client id generator.
    uint32_t mAuthAttempts{8};

    uint32_t mMaxPayloadSize{MAX_PAYLOAD};
    uint32_t mProtocolVersion{A_VERSION_MIN};
};

static base::LazyInstance<AdbConnectionImpl> gAdbConnection =
        LAZY_INSTANCE_INIT;

AdbConnection* AdbConnection::connection() {
    if (s_adb_port == 0)
        return nullptr;
    return gAdbConnection.ptr();
}

void AdbConnection::setAdbPort(int port) {
    s_adb_port = port;
}

// AdbStreambuf...
void AdbStreambuf::receive(const std::vector<char>& msg) {
    mReceivingQueue.sputn(msg.data(), msg.size());
    DD("Received: %lu, available: %lu", msg.size(), mReceivingQueue.in_avail());
    mConnection->sendOkay(mClientId, mRemoteId);
}

AdbStreambuf::AdbStreambuf(int idx, AdbConnectionImpl* con)
    : mClientId(idx), mConnection(con) {}

void AdbStreambuf::open(const std::string& id) {
    LOG(INFO) << "Opening adb service: " << id;
    std::vector<char> service{id.begin(), id.end()};
    service.emplace_back(0);
    mConnection->sendPacket(AdbWireMessage::A_OPEN, mClientId, 0,
                            std::move(service));
}

std::streamsize AdbStreambuf::xsputn(const char* s, std::streamsize n) {
    apacket packet{0};
    if (!mOpen)
        return 0;

    int written = 0;
    const char* w = s;
    auto maxPayload = mConnection->getMaxPayload();

    // Every write is followed by an A_OKAY, so we need to wait..
    while (n > 0 && readyForWrite()) {
        mReady = false;
        int write = std::min<std::streamsize>(maxPayload, n);
        mConnection->sendPacket(AdbWireMessage::A_WRTE, mClientId, mRemoteId,
                                std::vector<char>(w, w + write));
        w += write;
        n -= write;
    }
    return w - s;
}

AdbStreambuf::~AdbStreambuf() {
    close();
}

// Returns true if the OKAY packet has been received.
// and the connection is still alive.
bool AdbStreambuf::readyForWrite() {
    base::AutoLock lock(mWriterLock);
    while (!mReady && mOpen) {
        mWriterCV.wait(&mWriterLock);
    }
    return mReady && mOpen;
}

void AdbStreambuf::unlock() {
    mReady = true;
    mWriterCV.signal();
};

void AdbStreambuf::close() {
    if (mOpen) {
        mConnection->sendClose(mClientId, mRemoteId);
    }
    mOpen = false;
    mReceivingQueue.close();
    unlock();
}

}  // namespace emulation
}  // namespace android
