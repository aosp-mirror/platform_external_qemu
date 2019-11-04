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
#include "android/base/sockets/SocketUtils.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/AdbMessageSniffer.h"

#define DEBUG 2

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define DD(...) (void)0
#endif

#define _PRINT_LN fprintf(stderr, "%s: %s %d\n", __func__, __FILE__, __LINE__);

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

bool JdwpProxy::shouldBlockGuest() const {
    return mProxyState != mClientState;
}

bool JdwpProxy::shouldRegister() const {
    return !mRegistered && mProxyState == ConnectOk;
}

bool JdwpProxy::shouldCheckRegistration() const {
    return !mRegistered && mClientState == Connect;
}

bool JdwpProxy::shouldUnregister() const {
    //return mRegistered && mIsJdwp != Yes;
    return false;
}

void JdwpProxy::setRegistered(bool registered) {
    mRegistered = registered;
}

bool JdwpProxy::onRecvDataHeader(const android::emulation::AdbMessageSniffer::amessage *mesg) {
    return true;
}

void JdwpProxy::onGuestRecvData(const android::emulation::AdbMessageSniffer::amessage* mesg, const uint8_t* data,
        bool* shouldForwardRecv, std::queue<emulation::apacket>* toSends) {
    bool clientProxyDifferent = (mClientState != mProxyState);
    switch (mClientState) {
        case ConnectOk:
            if (mesg->command != ADB_WRTE
                || mesg->data_length != 14
                || 0 != memcmp("JDWP-Handshake", data, 14)) {
                _PRINT_LN
                mShouldClose = true;
                return;
            }
            break;
        case HandshakeReplied:
            if (mesg->command != ADB_OKAY) {
                _PRINT_LN
                mShouldClose = true;
            }
            mClientState = mProxyState = nextState(mProxyState);
            break;
        case Proxying:
            break;
        default:
            _PRINT_LN
            mShouldClose = true;
            break;
    }
    if (clientProxyDifferent) {
        mClientState = nextState(mClientState);
    } else {
        mClientState = mProxyState = nextState(mProxyState);
    }
}

bool JdwpProxy::needProxySendData(const android::emulation::AdbMessageSniffer::amessage *mesg) {
    return true;
}

void JdwpProxy::onGuestSendData(const android::emulation::AdbMessageSniffer::amessage* mesg, const uint8_t* data) {
    switch (mProxyState) {
        case Connect:
            if (mesg->command != ADB_OKAY) {
                _PRINT_LN
                mShouldClose = true;
                return;
            }
            mGuestId = mesg->arg0;
            break;
        case HandshakeSent:
            if (mesg->command != ADB_OKAY) {
                _PRINT_LN
                mShouldClose = true;
                return;
            }
            break;
        case HandshakeSentOk:
            if (mesg->command != ADB_WRTE
                || mesg->data_length != 14
                || 0 != memcmp("JDWP-Handshake", data, 14)) {
                _PRINT_LN
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
            _PRINT_LN
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

int32_t JdwpProxy::originHostId() const {
    return mHostId;
}

int JdwpProxy::currentHostId() const {
    return mCurrentHostId;
}

void JdwpProxy::fakeServer(int socket) {
    if (mProxyState == mClientState) {
        return;
    }
    while (mProxyState != mClientState) {
        assert(mProxyState > mClientState);
        emulation::apacket toHost;
        toHost.mesg.arg0 = mGuestId;
        toHost.mesg.arg1 = mCurrentHostId;
        toHost.mesg.data_check = 0;
        toHost.mesg.data_length = 0;
        switch (mClientState) {
            case Connect:
            case HandshakeSent:
                toHost.mesg.command = ADB_OKAY;
                toHost.mesg.magic = toHost.mesg.command ^ 0xffffffff;
                if (!emulation::sendPacket(socket, &toHost)) {
                    _PRINT_LN
                    return;
                }
                break;
            case ConnectOk: {
                emulation::apacket toGuest;
                if (!emulation::recvPacket(socket, &toGuest)) {
                    _PRINT_LN
                    return;
                }
                if (toGuest.mesg.command != ADB_WRTE || toGuest.mesg.data_length != 14
                        || memcmp(toGuest.data.data(), "JDWP-Handshake", 14) != 0) {
                    // TODO: do we need to close the socket?
                    _PRINT_LN
                    return;
                }
                break;
            }
            case HandshakeSentOk:
                toHost.mesg.command = ADB_WRTE;
                toHost.mesg.magic = toHost.mesg.command ^ 0xffffffff;
                toHost.mesg.data_length = 14;
                toHost.data.resize(14);
                memcpy(toHost.data.data(), "JDWP-Handshake", 14);
                if (!emulation::sendPacket(socket, &toHost)) {
                    _PRINT_LN
                    return;
                }
                break;
            case HandshakeReplied:
                if (!emulation::recvOkay(socket)) {
                    _PRINT_LN
                    return;
                }
                break;
            case Uninitialized:
            case HandshakeRepliedOk:
                _PRINT_LN
                assert(false);
                break;
        }
        mClientState = nextState(mClientState);
    }
    D("Fake JDWP server done, host id %d, new host id %d, guest id %d, "
            "client state %d, proxy state %d, proxying to real jdwp",
            mHostId, mCurrentHostId, mGuestId, mClientState, mProxyState);
    //base::socketSetNonBlocking(socket);
    //serverDoneCallback();
    //return intptr_t();

}

void JdwpProxy::fakeServerAsync(int socket,
        std::function<void()> serverDoneCallback) {
    //mClientState = ConnectOk;
    if (mProxyState == mClientState) {
        mFakeServerThread.reset();
    } else {
        mFakeServerThread.reset(new base::FunctorThread(
                [this, socket, serverDoneCallback]() -> intptr_t {
            base::socketSetBlocking(socket);
            fakeServer(socket);
            base::socketSetNonBlocking(socket);
            serverDoneCallback();
            return intptr_t();
        }));
        mFakeServerThread->start();
    }
}

void JdwpProxy::resetServerThread() {
    mFakeServerThread.reset();
}

}  // namespace jdwp
}  // namespace android
