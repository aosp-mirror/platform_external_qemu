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

#include "android/base/sockets/SocketUtils.h"
#include "android/base/threads/FunctorThread.h"
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
#else
#define DD(...) (void)0
#endif

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
    D("Loading JdwpProxy host id %d guest id %d\n", mHostId, mGuestId);
}

void JdwpProxy::onSave(android::base::Stream* stream) {
    D("Saving JdwpProxy host id %d guest id %d\n", mHostId, mGuestId);
    stream->putBe32(mHostId);
    stream->putBe32(mGuestId);
    stream->putBe32(mGuestPid);
    stream->putBe32(mProxyState);
    stream->putBe32(mShouldClose);
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
                0 != memcmp(sJdwpHandshake, data, sJdwpHandshakeSize)) {
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
                    memcpy(packet.data.data(), sJdwpHandshake,
                           sJdwpHandshakeSize);
                    toSends->push(packet);
                    mClientState = HandshakeReplied;
                }
            }
            break;
        case HandshakeReplied:
            if (mesg->command != ADB_OKAY) {
                mShouldClose = true;
                return;
            }
            if (clientProxyDifferent) {
                // Leak the last ADB_OKAY to the guest, to make it consistent
                // with icebox behavior.
                *shouldForwardRecv = true;
                mClientState = Proxying;
            }
            break;
        case Proxying:
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
                                const uint8_t* data) {
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
                0 != memcmp(sJdwpHandshake, data, sJdwpHandshakeSize)) {
                mShouldClose = true;
                return;
            }
            break;
        case Proxying:
            if (mesg->command == ADB_CLSE) {
                mShouldClose = true;
                return;
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
