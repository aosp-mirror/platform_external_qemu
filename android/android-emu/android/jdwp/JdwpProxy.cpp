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

JdwpProxy::JdwpProxy(int hostId, int guestPid) {
    mHostId = hostId;
    mGuestPid = guestPid;
    mProxyState = Connect;
    mClientState = Connect;
}

JdwpProxy::State JdwpProxy::nextState(State state) {
    D("State done %d", state);
    if (state < Proxying) {
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

bool JdwpProxy::needProxyRecvData(const android::emulation::AdbMessageSniffer::amessage *mesg) {
    switch (mProxyState) {
        case ConnectOk:
            if (mesg->command != ADB_WRTE) {
                mShouldClose = true;
                return false;
            }
            mGuestId = mesg->arg0;
            return true;
        case HandshakeReplied:
            if (mesg->command != ADB_OKAY) {
                mShouldClose = true;
            }
            mClientState = mProxyState = nextState(mProxyState);
            return false;
        case Proxying:
            return false;
        default:
            mShouldClose = true;
            return false;
    }
}

void JdwpProxy::onGuestRecvData(const char *data, size_t len) {
    using amessage = android::emulation::AdbMessageSniffer::amessage;
    const amessage *mesg = (const amessage*)data;
    const size_t kHeaderSize = sizeof(amessage);
    if (mProxyState != ConnectOk
        || mesg->data_length != 14
        || 0 != memcmp("JDWP-Handshake",
                        data + kHeaderSize,
                        14)) {
        mShouldClose = true;
        return;
    }
    mClientState = mProxyState = nextState(mProxyState);
}

bool JdwpProxy::needProxySendData(const android::emulation::AdbMessageSniffer::amessage *mesg) {
    switch (mProxyState) {
        case Connect:
        case HandshakeSent:
            if (mesg->command != ADB_OKAY) {
                mShouldClose = true;
            }
            mClientState = mProxyState = nextState(mProxyState);
            return false;
        case HandshakeSentOk:
            if (mesg->command != ADB_WRTE) {
                mShouldClose = true;
                return false;
            }
            return true;
        case Proxying:
            return false;
        default:
            mShouldClose = true;
            return false;
    }
}

void JdwpProxy::onGuestSendData(const char *data, size_t len) {
    using amessage = android::emulation::AdbMessageSniffer::amessage;
    const amessage *mesg = (const amessage*)data;
    const size_t kHeaderSize = sizeof(amessage);
    if (mProxyState != HandshakeSentOk
        || mesg->data_length != 14
        || 0 != memcmp("JDWP-Handshake",
                        data + kHeaderSize,
                        14)) {
        mShouldClose = true;
        return;
    }
    mClientState = mProxyState = nextState(mProxyState);
}

bool JdwpProxy::shouldClose() const {
    return mShouldClose;
}

//int JdwpProxy::currentHostId() const {
//    return mCurrentHostId;
//}

void JdwpProxy::fakeServerAsync(int socket,
        std::function<void()> serverDoneCallback) {
    if (mProxyState == mClientState) {
        mFakeServerThread.reset();
    } else {
        mFakeServerThread.reset(new base::FunctorThread(
                [this, socket, serverDoneCallback]() -> intptr_t {
            base::socketSetBlocking(socket);
            while (mProxyState != mClientState) {
                assert(mProxyState > mClientState);
                emulation::apacket toHost;
                toHost.mesg.arg0 = mGuestId;
                toHost.mesg.arg1 = mHostId;
                toHost.mesg.data_check = 0;
                toHost.mesg.data_length = 0;
                switch (mClientState) {
                    case Connect:
                    case HandshakeSent:
                        toHost.mesg.command = ADB_OKAY;
                        toHost.mesg.magic = toHost.mesg.command ^ 0xffffffff;
                        if (!emulation::sendPacket(socket, &toHost)) {
                            _PRINT_LN
                            return intptr_t();
                        }
                        break;
                    case ConnectOk: {
                        emulation::apacket toGuest;
                        if (!emulation::recvPacket(socket, &toGuest)) {
                            _PRINT_LN
                            return intptr_t();
                        }
                        if (toGuest.mesg.command != ADB_WRTE || toGuest.mesg.data_length != 14
                                || memcmp(toGuest.data.data(), "JDWP-Handshake", 14) != 0) {
                            // TODO: do we need to close the socket?
                            _PRINT_LN
                            return intptr_t();
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
                            return intptr_t();
                        }
                        break;
                    case HandshakeReplied:
                        if (!emulation::recvOkay(socket)) {
                            _PRINT_LN
                            return intptr_t();
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
            D("Fake JDWP server done, proxying to real jdwp");
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
