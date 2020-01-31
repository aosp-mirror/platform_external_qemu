// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/jdwp/JdwpProxy.h"

#include "android/emulation/apacket_utils.h"
#include "android/jdwp/Jdwp.h"

#define DEBUG 0

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2

#define DD(...) D(__VA_ARGS__)
static void debugPrintJdwp(const char* prefix, const uint8_t* data) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    android::jdwp::JdwpCommandHeader jdwpCmd;
    jdwpCmd.parseFrom(data);
    fprintf(stderr,
            "%s (%ld) jdwp id %d flags %d cmd_set %d cmd %d length %d\n",
            prefix, ms, (int)jdwpCmd.id, (int)jdwpCmd.flags,
            (int)jdwpCmd.command_set, (int)jdwpCmd.command,
            (int)jdwpCmd.length);
    uint32_t printLength = jdwpCmd.length > 100 ? 100 : jdwpCmd.length;
    fprintf(stderr, "%sjdwp data string: %s\n", prefix, data + 11);
    fprintf(stderr, "%sjdwp data binary: ", prefix);
    for (uint32_t i = 11; i < printLength; i++) {
        fprintf(stderr, "%02x", data[i]);
    }
    fprintf(stderr, "\n");
}

#else  // DEBUG >= 2
#define DD(...) (void)0
#endif  // DEBUG >= 2

namespace android {
namespace jdwp {

JdwpProxy::JdwpProxy(int hostId, int guestId, int guestPid) {
    D("New JdwpProxy host id %d guest id %d\n", hostId, guestId);
    mHostId = hostId;
    mCurrentHostId = mHostId;
    mGuestId = guestId;
    mGuestPid = guestPid;
    mProxyState = ConnectOk;
    mClientState = ConnectOk;
}

JdwpProxy::JdwpProxy(android::base::Stream* stream) {
    mHostId = stream->getBe32();
    mGuestId = stream->getBe32();
    mGuestPid = stream->getBe32();
    mProxyState = (State)stream->getBe32();
    mClientState = Uninitialized;
    mShouldClose = stream->getBe32();
    mCurrentHostId = -1;
    mShouldSendCachePacket = stream->getByte();
    if (mShouldSendCachePacket) {
        mloadedCachedPacket.reset(new emulation::apacket);
        stream->read(&(mloadedCachedPacket->mesg),
                     sizeof(mloadedCachedPacket->mesg));
        mloadedCachedPacket->data.resize(mloadedCachedPacket->mesg.data_length);
        stream->read(mloadedCachedPacket->data.data(),
                     mloadedCachedPacket->mesg.data_length);
    }
    D("Loading JdwpProxy host id %d guest id %d\n", mHostId, mGuestId);
}

void JdwpProxy::onSave(android::base::Stream* stream) {
    D("Saving JdwpProxy host id %d guest id %d\n", mHostId, mGuestId);
    stream->putBe32(mHostId);
    stream->putBe32(mGuestId);
    stream->putBe32(mGuestPid);
    stream->putBe32(mProxyState);
    stream->putBe32(mShouldClose);
    if (mloadedCachedPacket || mCachedPacket) {
        emulation::apacket* packetToSave = mloadedCachedPacket
                                                   ? mloadedCachedPacket.get()
                                                   : mCachedPacket.get();
        stream->putByte(1);
        stream->write(&(packetToSave->mesg), sizeof(packetToSave->mesg));
        stream->write(packetToSave->data.data(),
                      packetToSave->mesg.data_length);
    } else {
        stream->putByte(0);
    }
}

JdwpProxy::State JdwpProxy::nextState(State state) {
    if (state < Proxying) {
        D("State done %d", state);
        return State((int)state + 1);
    }
    return Proxying;
}

void JdwpProxy::onGuestRecvData(const android::emulation::amessage* mesg,
                                const uint8_t* data,
                                bool* shouldForwardRecv,
                                std::queue<emulation::apacket>* toSends) {
    if (mesg->command == ADB_CLSE) {
        mShouldClose = true;
    }
    *shouldForwardRecv = true;
    bool clientProxyDifferent = (mClientState != mProxyState);
    switch (mClientState) {
        case Uninitialized:
            if (!clientProxyDifferent || mesg->command != ADB_OPEN) {
                mShouldClose = true;
                return;
            }
            // Connect
            {
                CHECK(mProxyState > Connect);
                *shouldForwardRecv = false;
                emulation::apacket packet;
                packet.mesg = buildReplyMesg();
                packet.mesg.command = ADB_OKAY;
                packet.mesg.magic = packet.mesg.command ^ 0xffffffff;
                toSends->push(packet);
                mClientState = ConnectOk;
            }
            break;
        case ConnectOk:
            if (mesg->command != ADB_WRTE || mesg->data_length != 14 ||
                0 != memcmp(kJdwpHandshake, data, kJdwpHandshakeSize)) {
                mShouldClose = true;
                return;
            }
            if (clientProxyDifferent) {
                // HandshakeSent
                *shouldForwardRecv = false;
                emulation::apacket packet;
                packet.mesg = buildReplyMesg();
                packet.mesg.command = ADB_OKAY;
                packet.mesg.magic = packet.mesg.command ^ 0xffffffff;
                toSends->push(packet);
                if (mProxyState > HandshakeSentOk) {
                    // HandshakeSentOk
                    packet.mesg.command = ADB_WRTE;
                    packet.mesg.magic = packet.mesg.command ^ 0xffffffff;
                    packet.mesg.data_length = 14;
                    packet.data.resize(14);
                    memcpy(packet.data.data(), kJdwpHandshake,
                           kJdwpHandshakeSize);
                    toSends->push(packet);
                    mClientState = HandshakeReplied;
                }
            }
            break;
        case HandshakeReplied:
            // No-op when connecting to jdwp the first time
            // It should receive an ADB_OKAY, but Android Studio will mix it
            // with other commands.
            if (clientProxyDifferent) {
                // Leak the last ADB_OKAY to the guest, to make it consistent
                // with icebox behavior.
                *shouldForwardRecv = true;
                mClientState = Proxying;
            }
            break;
        case Proxying:
#if DEBUG >= 2
            if (mesg->command == ADB_WRTE) {
                debugPrintJdwp("guest recv ", data);
            }
#endif  // DEBUG >= 2
            if (mesg->command == ADB_WRTE && mShouldSendCachePacket) {
                JdwpCommandHeader jdwpCmd;
                jdwpCmd.parseFrom(data);
                if ((jdwpCmd.flags & kJdwpReplyFlag) == 0 &&
                    jdwpCmd.command_set < ExtensionBegin) {
                    if (jdwpCmd.command_set == EventRequest &&
                        jdwpCmd.command == Set &&
                        data[kJdwpHeaderSize] == Exception &&
                        data[kJdwpHeaderSize + 1] != SuspendPolicy::None) {
                        mBreakpointRequestId = jdwpCmd.id;
                    }
                    mPendingGuestReplyCommandIds.insert(jdwpCmd.id);
                    DD("new cmd id %d, total %d\n", jdwpCmd.id,
                       (int)mPendingGuestReplyCommandIds.size());
                }
            }
            break;
        default:
            mShouldClose = true;
            return;
    }
    if (!clientProxyDifferent) {
        mClientState = mProxyState = nextState(mProxyState);
    }
}

void JdwpProxy::onGuestSendData(const android::emulation::amessage* mesg,
                                const uint8_t* data,
                                bool* shouldForwardSend,
                                std::queue<emulation::apacket>* extraSends) {
    *shouldForwardSend = true;
    if (mesg->command == ADB_CLSE) {
        mShouldClose = true;
    }
    switch (mProxyState) {
        case Connect:
            if (mesg->command != ADB_OKAY) {
                mShouldClose = true;
                return;
            }
            mGuestId = mesg->arg0;
            break;
        case HandshakeSent:
            if (mesg->command != ADB_OKAY) {
                mShouldClose = true;
                return;
            }
            break;
        case HandshakeSentOk:
            if (mesg->command != ADB_WRTE || mesg->data_length != 14 ||
                0 != memcmp(kJdwpHandshake, data, kJdwpHandshakeSize)) {
                mShouldClose = true;
                return;
            }
            break;
        case Proxying:
#if DEBUG >= 2
            if (mesg->command == ADB_WRTE) {
                debugPrintJdwp("guest send ", data);
            }
#endif  // DEBUG >= 2
            if (mesg->command == ADB_CLSE) {
                mShouldClose = true;
                return;
            }
            if (mesg->command == ADB_WRTE) {
                // Cache the last message
                mCachedPacket.reset(new emulation::apacket);
                mCachedPacket->mesg = *mesg;
                if (mesg->data_length) {
                    mCachedPacket->data.assign(data, data + mesg->data_length);
                }
                if (mShouldSendCachePacket) {
                    JdwpCommandHeader jdwpCmd;
                    jdwpCmd.parseFrom(data);
                    if (jdwpCmd.flags & kJdwpReplyFlag &&
                        jdwpCmd.command_set < ExtensionBegin) {
                        mPendingGuestReplyCommandIds.erase(jdwpCmd.id);
                        // Collect the event ID.
                        if (jdwpCmd.id == mBreakpointRequestId) {
                            memcpy(&mBreakpointEventId, data + kJdwpHeaderSize,
                                   4);
                        }
                        DD("recv reply id %d, remaining %d\n", jdwpCmd.id,
                           (int)mPendingGuestReplyCommandIds.size());
                        // If it doesn't have pending commands and the event ID
                        // is set, send out the loaded pause message
                        if (mPendingGuestReplyCommandIds.empty() &&
                            mBreakpointEventId) {
                            *shouldForwardSend = false;
                            extraSends->push(emulation::apacket());
                            emulation::apacket& packet0 = extraSends->back();
                            packet0.mesg = *mesg;
                            packet0.data.assign(data, data + mesg->data_length);
                            extraSends->push(*mloadedCachedPacket.get());
                            emulation::apacket& packet1 = extraSends->back();
                            packet1.mesg.arg1 = mCurrentHostId;
                            // The message looks like (Hexadecimal):
                            // 02 (suspend policy: all) 000000001 (number of
                            // events) 02 (event kind: break point) xxxxxxxx
                            // (request id) ... We need to override the request
                            // id to match those known to JDI
                            memcpy(packet1.data.data() + kJdwpHeaderSize + 6,
                                   &mBreakpointEventId, 4);
                            mShouldSendCachePacket = false;
                            mDebuggerActivated = false;
                            mloadedCachedPacket.reset();
                            D("Pause message injected\n");
                        }
                    }
                }
            }
            break;
        default:
            mShouldClose = true;
            return;
    }
    CHECK(mClientState == mProxyState);
    mClientState = mProxyState = nextState(mProxyState);
}

bool JdwpProxy::shouldClose() const {
    return mShouldClose;
}

int32_t JdwpProxy::guestId() const {
    return mGuestId;
}

int32_t JdwpProxy::guestPid() const {
    return mGuestPid;
}

int32_t JdwpProxy::originHostId() const {
    return mHostId;
}

int JdwpProxy::currentHostId() const {
    return mCurrentHostId;
}

void JdwpProxy::setCurrentHostId(int32_t currentHostId) {
    mCurrentHostId = currentHostId;
}

emulation::amessage JdwpProxy::buildReplyMesg() {
    emulation::amessage mesg;
    mesg.arg0 = (unsigned)mGuestId;
    mesg.arg1 = (unsigned)mCurrentHostId;
    mesg.magic = 0xffffffff;
    return mesg;
}

}  // namespace jdwp
}  // namespace android
