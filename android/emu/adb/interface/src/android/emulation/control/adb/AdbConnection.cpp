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
#include "android/emulation/control/adb/AdbConnection.h"

#include <assert.h>  // for assert
#include <ctype.h>   // for isprint
#include <stdio.h>   // for fprintf

#include <algorithm>      // for min
#include <atomic>         // for atomic
#include <memory>         // for shared_ptr
#include <type_traits>    // for enable_i...
#include <unordered_map>  // for unordere...
#include <unordered_set>  // for unordere...
#include <utility>        // for move, pair
#include <vector>         // for vector

#include "android/base/ArraySize.h"                          // for stringLi...
#include "android/base/Log.h"                                // for LogStream
#include "android/base/async/AsyncSocket.h"                  // for AsyncSocket
#include "android/base/async/ThreadLooper.h"                 // for ThreadLo...
#include "android/base/files/PathUtils.h"                    // for pj
#include "android/base/files/QueueStreambuf.h"               // for QueueStr...
#include "android/base/synchronization/ConditionVariable.h"  // for Conditio...
#include "android/base/synchronization/Lock.h"               // for Lock
#include "android/base/system/System.h"                      // for System
#include "android/base/threads/FunctorThread.h"              // for FunctorT...
#include "android/emulation/control/adb/adbkey.h"            // for adb_auth...
#include "emulator/net/AsyncSocketAdapter.h"                 // for AsyncSoc...

namespace android {
namespace base {
class AsyncThreadWithLooper;
}  // namespace base

namespace emulation {
struct amessage;
struct apacket;
}  // namespace emulation
}  // namespace android

/* set >0 for very verbose debugging */
#define DEBUG 0

#define D(...) (void)0
#define DD(...) (void)0
#define DD_PACKET(...) (void)0
#if DEBUG >= 1
#undef D
#define D(fmt, ...)                                                        \
    fprintf(stderr, "AdbConnection: %s:%d| " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__)
#endif
#if DEBUG >= 2
#undef DD
#undef DD_PACKET
#define DD(fmt, ...)                                                       \
    fprintf(stderr, "AdbConnection: %s:%d| " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__)
#define DD_PACKET(pkt)                                                       \
    do {                                                                     \
        const char* buf = reinterpret_cast<const char*>(&pkt);               \
        fprintf(stderr, "AdbConnection: %s:%d %d, %d, %d = %lu\n", __func__, \
                __LINE__, pkt.msg.arg0, pkt.msg.arg1, pkt.msg.data_length,   \
                pkt.payload.size());                                         \
        for (int x = 0; x < sizeof(apacket); x++) {                          \
            if (isprint((int)buf[x]))                                        \
                fprintf(stderr, "%c", buf[x]);                               \
            else                                                             \
                fprintf(stderr, "[0x%02x]", 0xff & (int)buf[x]);             \
        }                                                                    \
        fprintf(stderr, "\n");                                               \
    } while (0)

#endif

namespace android {
namespace emulation {

using android::base::AsyncSocket;
using android::base::ConditionVariable;
using android::base::Lock;
using android::base::System;
using emulator::net::AsyncSocketAdapter;

constexpr size_t MAX_PAYLOAD_V1 = 4 * 1024;
constexpr size_t MAX_PAYLOAD = 1024 * 1024;
constexpr size_t RECV_BUFFER_SIZE = 512;

// True if we disable to the internal bridge.
static bool sAdbInternalDisabled = false;

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
        STATE(socket);
        STATE(connecting);
        STATE(authorizing);
        STATE(connected);
        STATE(failed);
        STATE(offer_key);
    }
#undef STATE
    return out << s;
}

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

    void setWriteTimeout(uint64_t timeoutMs) { mWriteTimeout = timeoutMs; }

    // Closes down the stream properly
    void close();

    // Called when a ready message has been received.
    void unlock();

    // Opens a stream to the given service id, starting
    // the hand shake protocol
    void open(const std::string& id);

    // True if the connection is not closed
    bool isOpen() { return mRemoteId != -1; }

    // Used to set the remote id to which this stream is connected.
    void setRemoteId(int id) {
        // Can be set only once.
        assert(id == mRemoteId || mRemoteId == -1);
        if (mRemoteId == -1)
            mRemoteId = id;
    };

    // Called when we received bytes from the remote id.
    void receive(const std::vector<char>& msg);

    // Blocks and waits returning true when we unlock, or false for timeout.
    bool waitForUnlock(
            uint64_t timeoutMs = std::numeric_limits<uint64_t>::max());

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
    // Blocks and waits returning true when we can write, and are connected.
    bool readyForWrite();

    int32_t mClientId;      // Stream ID our side.
    int64_t mRemoteId{-1};  // Stream ID on the ADB side.
    bool mReady = false;
    AdbConnectionImpl* mConnection;

    base::QueueStreambuf
            mReceivingQueue;  // Incoming bytes live in this stream.

    // You cannot write to adb until you have received an OKAY message.
    // These locks help us waiting for receipt of such a message.
    Lock mWriterLock;
    ConditionVariable mWriterCV;
    uint64_t mWriteTimeout{std::numeric_limits<uint64_t>::max()};
};

// This is basically a std::iostream with using the adbstream buffer
class BasicAdbStream : public AdbStream {
public:
    BasicAdbStream(AdbStreambuf* buf) : AdbStream(buf) {
        if (buf == nullptr) {
            setstate(failbit);
        }
    }

    ~BasicAdbStream() {
        close();
        delete adbbuf();
    }

    void close() override {
        if (adbbuf())
            adbbuf()->close();
    };

    void setWriteTimeout(uint64_t timeoutMs) override {
        if (adbbuf() != nullptr)
            adbbuf()->setWriteTimeout(timeoutMs);
    }

    AdbStreambuf* adbbuf() { return reinterpret_cast<AdbStreambuf*>(rdbuf()); }
};

constexpr const char hdr[] =
        "host::features=remount_shell,abb_exec,fixed_push_symlink_"
        "timestamp,abb,stat_v2,apex,shell_v2,fixed_push_mkdir,cmd";

// This class manages the stream multiplexing and auth to the adbd deamon.
// This uses async i/o an ThreadLooper thread.
// Note that ADBD can be very flaky during system image boot. The kernel
// can decide to Kill ADBD whenever it sees fit for example.
class AdbConnectionImpl : public AdbConnection,
                          public emulator::net::AsyncSocketEventListener {
public:
    AdbConnectionImpl(AsyncSocketAdapter* socket) : mSocket(socket) {
        mSocket->setSocketEventListener(this);
        if (mSocket->connected()) {
            mState = AdbState::socket;
        }
    }

    ~AdbConnectionImpl() {
        DD("~AdbConnectionImpl");
        closeStreams();
        mSocket->dispose();
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

    // This will try to establish a connection to the emulator when none exists:
    // it will:
    // 1. Connect the socket if closed.
    // 2. Do the handshake if needed.
    // 3. Wait at most timeoutMs.
    // Returns true if we are connected, or false otherwise.
    bool connectToEmulator(int timeoutMs) {
        base::AutoLock connectLock(mConnectLock);
        if (!mSocket->connectSync(timeoutMs)) {
            DD("Failed to connect to socket.");
            return false;
        }

        bool sendConnect = false;
        auto waitUntil = System::get()->getUnixTimeUs() + (timeoutMs * 1000);
        {
            base::AutoLock lock(mStateLock);
            // Only send out a connect request if we are disconnected.
            // We never want to send more than one on an open connection,
            // otherwise things will go haywire
            if (state() == AdbState::socket) {
                mState = AdbState::connecting;
                sendConnect = true;
                sendCnxn();
            }

            while (state() != AdbState::connected &&
                   System::get()->getUnixTimeUs() < waitUntil) {
                auto why = mStateCv.timedWait(&mStateLock, waitUntil);
            }
        }
        if (sendConnect && state() == AdbState::connecting) {
            // Assume that our message went into the void..
            DD("No timely response, closing socket.");
            mSocket->close();
        }
        return state() == AdbState::connected;
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
    std::shared_ptr<AdbStream> open(const std::string& id,
                                    uint32_t timeoutMs) override {
        D("Open %s, :%d", id.c_str(), timeoutMs);
        if (mState == AdbState::failed) {
            LOG(INFO) << "No proper keys installed, refusing to "
                         "connect, adb direct is disabled.";
            return std::make_shared<BasicAdbStream>(nullptr);
        }
        // Hand out "bad" streams on closed connections that we are not yet
        // establishing.
        if (!connectToEmulator(timeoutMs)) {
            D("Open %s, not yet connected", id.c_str());
            return std::make_shared<BasicAdbStream>(nullptr);
        }
        auto buf = new AdbStreambuf(mNextClientId, this);
        auto myId = mNextClientId++;
        std::shared_ptr<BasicAdbStream> stream;
        {
            android::base::AutoLock lock(mActiveStreamsLock);
            // TODO(jansene): Do we need push/pull support?
            stream = std::make_shared<BasicAdbStream>(buf);
            mActiveStreams[myId] = stream;
            buf->open(id);
        }

        if (!buf->waitForUnlock(timeoutMs)) {
            // Okay, we failed.. We should close our socket and redo the
            // handshake if we are not showing a ui..
            if (state() != AdbState::offer_key) {
                D("Not showing offer key dialog, closing connection.");
                mSocket->close();
            }
            D("Open %s, timeout", id.c_str());
            return std::make_shared<BasicAdbStream>(nullptr);
        }

        D("Open %s, connected", id.c_str());
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

        mProtocolVersion = msg.msg.arg0;
        mMaxPayloadSize = msg.msg.arg1;

        // Now split out the connection string.
        // "device::ro.product.name=x;ro.product.model=y;ro.product.device=z;features="
        // we only care about the feature set, if any.
        mFeatures.clear();
        const std::string features = "features=";
        size_t start = banner.find(features);
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

        // Finally switch state, this will wake up sleepers, we are ready to go!
        setState(AdbState::connected);
    }

    void setState(const AdbState newState) {
        base::AutoLock lock(mStateLock);
        if (mState == AdbState::failed) {
            LOG(VERBOSE) << "We are in a failed state.. We give up.";
            sAdbInternalDisabled = true;
            return;
        }

        LOG(VERBOSE) << "Adb transition " << mState << " -> " << newState;
        mState = newState;
        mStateChange = System::get()->getUnixTimeUs();

        // Wake up everyone, as we might be in a fully connected state.
        mStateCv.broadcast();
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
        mAuthAttempts = 2;
        sendPacket(AdbWireMessage::A_CNXN, A_VERSION, MAX_PAYLOAD,
                   {hdr, hdr + base::stringLiteralLength(hdr)});
    }

    void sendPublicKeyToDevice() {
        // So we have a mismatch between our private key and the pub. key inside
        // the emulator.
        // Currently that means we get into a fight with potential other ADB
        // connections b/150160590.. For now we will just completely disable
        // ourselves and give up the fight.
        setState(AdbState::failed);
        mSocket->close();
        LOG(ERROR) << "We are not offering to install a public key due to "
                      "b/150160590, direct bridge disabled.\n"
                      "Depending on your system image you might experience ADB "
                      "connection problems. If you do restart the image with "
                      "-wipe-data flag.";
        return;

        auto privkey = getPrivateAdbKeyPath();
        if (privkey.empty()) {
            LOG(WARNING) << "No private key exists, we will have to "
                            "generate one.";
            privkey = base::pj(System::get()->getHomeDirectory(), ".android",
                               "adbkey");
            if (!adb_auth_keygen(privkey.c_str())) {
                LOG(ERROR) << "Cannot create private key: " << privkey;
                return;
            }
        }

        std::string key = "";
        // Welp! We didn't get a key..
        if (!pubkey_from_privkey(privkey, &key)) {
            LOG(ERROR) << "Failure to create public key.";
            mSocket->close();
            return;
        }

        // This will show a message to the user, if the user accepts a
        // connection message will arrive..
        setState(AdbState::offer_key);
        sendPacket(AdbWireMessage::A_AUTH, ADB_AUTH_RSAPUBLICKEY, 0,
                   {key.c_str(), key.c_str() + key.size()});
    }

    void sendAuth(const apacket& recv) {
        mAuthAttempts--;
        // Bail out if we tried a few times to auth and it failed..
        if (mAuthAttempts < 0) {
            mSocket->close();
        }
        if (mAuthAttempts == 0) {
            sendPublicKeyToDevice();
            return;
        }

        int sigLen = 256;
        std::vector<char> signed_token{};
        signed_token.resize(sigLen);
        if (!sign_auth_token((uint8_t*)recv.payload.data(),
                             recv.msg.data_length,
                             (uint8_t*)signed_token.data(), sigLen)) {
            // Oh, oh.. We are likely missing our private key..
            sendPublicKeyToDevice();
            return;
        }
        signed_token.resize(sigLen);
        setState(AdbState::authorizing);
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
        onRead(socket);
    }

    // Called when this socket (re-)establishes a connection
    // it kicks of the auth thread. The emulator makes adb
    // available early on, but it might take a while before
    // adbd is ready in the emulator.
    void onConnected(AsyncSocketAdapter* socket) override {
        setState(AdbState::socket);
    };

    // Mark the connection as closed.
    void closeStreams() {
        base::AutoLock lock(mActiveStreamsLock);
        for (auto& entry : mActiveStreams) {
            entry.second->close();
        }
        mFeatures.clear();
        mActiveStreams.clear();
        setState(AdbState::disconnected);
    }

    // Called when this socket is closed.
    void onClose(AsyncSocketAdapter* socket, int err) override {
        LOG(VERBOSE) << ("Disconnected");
        closeStreams();
    };

    void close() override { mSocket->close(); }

private:
    std::unique_ptr<AsyncSocketAdapter> mSocket;   // Socket to adbd in emulator
    apacket mIncoming{.msg = {0}, .payload = {}};  // Incoming packet
    size_t mPacketOffset{0};  // Received bytes of packet so far

    std::unique_ptr<base::FunctorThread>
            mOpenerThread;                      // Initial connector thread
    std::unordered_set<std::string> mFeatures;  // available features

    Lock mStateLock;
    Lock mConnectLock;
    ConditionVariable mStateCv;
    std::atomic<AdbState> mState{
            AdbState::disconnected};        // Current connection state
    std::atomic<uint64_t> mStateChange{0};  // Timestamp of this state change.

    // Stream management
    Lock mActiveStreamsLock;
    std::unordered_map<int32_t, std::shared_ptr<BasicAdbStream>> mActiveStreams;

    uint32_t mNextClientId{1};  // Client id generator.
    int8_t mAuthAttempts{8};

    uint32_t mMaxPayloadSize{MAX_PAYLOAD};
    uint32_t mProtocolVersion{A_VERSION_MIN};

};  // namespace emulation

static std::shared_ptr<AdbConnectionImpl> gAdbConnection;

std::shared_ptr<AdbConnection> AdbConnection::connection(int timeoutMs) {
    if (timeoutMs > 0 && !gAdbConnection->connectToEmulator(timeoutMs)) {
        LOG(VERBOSE) << "Timeout:" << timeoutMs
                     << "Connection state: " << gAdbConnection->state();
    }
    return gAdbConnection;
}

bool AdbConnection::failed() {
    return sAdbInternalDisabled;
}

void AdbConnection::setAdbPort(int adbPort) {
    auto looper = android::base::ThreadLooper::get();
    setAdbSocket(new AsyncSocket(looper, adbPort));
}

void AdbConnection::setAdbSocket(AsyncSocketAdapter* socket) {
    if (socket == nullptr) {
        gAdbConnection.reset();
    } else {
        gAdbConnection = std::make_shared<AdbConnectionImpl>(socket);
    }
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
    std::vector<char> service{id.begin(), id.end()};
    service.emplace_back(0);
    mConnection->sendPacket(AdbWireMessage::A_OPEN, mClientId, 0,
                            std::move(service));
}

std::streamsize AdbStreambuf::xsputn(const char* s, std::streamsize n) {
    apacket packet{0};
    if (!isOpen())
        return 0;

    int written = 0;
    const char* w = s;
    auto maxPayload = mConnection->getMaxPayload();

    // Every write is followed by an A_OKAY, so we need to wait..
    base::AutoLock lock(mWriterLock);
    while (n > 0 && readyForWrite()) {
        mReady = false;
        int write = std::min<std::streamsize>(maxPayload, n);
        mConnection->sendPacket(AdbWireMessage::A_WRTE, mClientId, mRemoteId,
                                {w, w + write});
        w += write;
        n -= write;
    }
    return w - s;
}

AdbStreambuf::~AdbStreambuf() {
    D("~AdbStreambuf");
    close();
}

bool AdbStreambuf::waitForUnlock(uint64_t timeoutms) {
    base::AutoLock lock(mWriterLock);
    auto waitUntil = base::System::get()->getUnixTimeUs() + timeoutms * 1000;
    while (!mReady && System::get()->getUnixTimeUs() < waitUntil) {
        mWriterCV.timedWait(&mWriterLock, waitUntil);
    }
    return mReady;
}

// Returns true if the OKAY packet has been received.
// and the connection is still alive. Returns false in case
// of timeout or closed stream..
bool AdbStreambuf::readyForWrite() {
    auto waitUntilUs = System::get()->getUnixTimeUs() + mWriteTimeout * 1000;
    while (!mReady && isOpen() &&
           System::get()->getUnixTimeUs() < waitUntilUs) {
        mWriterCV.timedWait(&mWriterLock, waitUntilUs);
    }
    return mReady && isOpen();
}

void AdbStreambuf::unlock() {
    base::AutoLock lock(mWriterLock);
    mReady = true;
    mWriterCV.signal();
};

void AdbStreambuf::close() {
    if (isOpen()) {
        mConnection->sendClose(mClientId, mRemoteId);
    }
    mRemoteId = -1;
    mReceivingQueue.close();
    unlock();
}

}  // namespace emulation
}  // namespace android
