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

#include "android/emulation/AdbHub.h"

#include "android/base/async/Looper.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/apacket_utils.h"
#include "android/jdwp/JdwpProxy.h"

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

static const size_t kHeaderSize = sizeof(android::emulation::amessage);

namespace android {
namespace emulation {
void AdbHub::onSave(android::base::Stream* stream) {
    stream->putBe32(mJdwpProxies.size());
    for (const auto& proxy : mJdwpProxies) {
        proxy.second->onSave(stream);
    }
    stream->write(&mCnxnPacket.mesg, sizeof(mCnxnPacket.mesg));
    stream->putBe32(mCnxnPacket.data.size());
    stream->write(mCnxnPacket.data.data(), mCnxnPacket.data.size());
}

void AdbHub::onLoad(android::base::Stream* stream) {
    int proxyCount = stream->getBe32();
    for (int i = 0; i < proxyCount; i++) {
        jdwp::JdwpProxy* proxy = new jdwp::JdwpProxy(stream);
        mProxies.emplace(proxy->guestId(), proxy);
        mJdwpProxies.emplace(proxy->guestPid(),
                std::unique_ptr<jdwp::JdwpProxy>(proxy));
    }
    stream->read(&mCnxnPacket.mesg, sizeof(mCnxnPacket.mesg));
    int bufferSize = stream->getBe32();
    mCnxnPacket.data.resize(bufferSize);
    stream->read(mCnxnPacket.data.data(), mCnxnPacket.data.size());
    mShouldReconnect = true;
}

void AdbHub::checkRemoveProxy(
        std::unordered_map<int, AdbProxy*>::iterator proxy) {
    if (proxy != mProxies.end() && proxy->second->shouldClose()) {
        jdwp::JdwpProxy* jdwpProxy = (jdwp::JdwpProxy*)proxy->second;
        mJdwpProxies.erase(jdwpProxy->guestPid());
        mProxies.erase(proxy);
    }
}

int AdbHub::onGuestSendData(const AndroidPipeBuffer* buffers, int numBuffers) {
    int currentBuffer = 0;
    size_t currentPst = 0;
    int actualSendBytes = 0;
    while (currentBuffer < numBuffers) {
        size_t remainingPacketSize =
                mCurrentGuestSendPacketPst >= kHeaderSize
                        ? mCurrentGuestSendPacket.data.size() + kHeaderSize -
                                  mCurrentGuestSendPacketPst
                        : kHeaderSize - mCurrentGuestSendPacketPst;
        size_t remainingBufferSize = buffers[currentBuffer].size - currentPst;
        size_t currentReadSize =
                std::min(remainingPacketSize, remainingBufferSize);
        uint8_t* writePtr =
                mCurrentGuestSendPacketPst >= kHeaderSize
                        ? ((uint8_t*)mCurrentGuestSendPacket.data.data()) +
                                  mCurrentGuestSendPacketPst - kHeaderSize
                        : ((uint8_t*)&mCurrentGuestSendPacket.mesg) +
                                  mCurrentGuestSendPacketPst;
        memcpy(writePtr, buffers[currentBuffer].data + currentPst,
               currentReadSize);
        currentPst += currentReadSize;
        if (currentPst == buffers[currentBuffer].size) {
            currentPst = 0;
            currentBuffer++;
        }
        mCurrentGuestSendPacketPst += currentReadSize;
        actualSendBytes += currentReadSize;
        if (mCurrentGuestSendPacketPst == kHeaderSize) {
            mCurrentGuestSendPacket.data.resize(
                    mCurrentGuestSendPacket.mesg.data_length);
        }
        if (mCurrentGuestSendPacketPst >= kHeaderSize &&
            mCurrentGuestSendPacketPst ==
                    mCurrentGuestSendPacket.data.size() + kHeaderSize) {
            // handle message
            bool shouldPush = true;
            std::queue<emulation::apacket> sendDataQueue;
            amessage& mesg = mCurrentGuestSendPacket.mesg;
            if (mesg.command == ADB_CNXN) {
                mCnxnPacket = mCurrentGuestSendPacket;
            } else {
                auto pendingConnection = mPendingConnections.find(mesg.arg1);
                if (mesg.command == ADB_OKAY &&
                    pendingConnection != mPendingConnections.end()) {
                    CHECK(mesg.data_length == 0);
                    onNewConnection(pendingConnection->second,
                                    mCurrentGuestSendPacket);
                    mPendingConnections.erase(pendingConnection);
                } else {
                    auto proxyIte = mProxies.find(mesg.arg0);
                    if (proxyIte != mProxies.end()) {
                        int32_t originHostId = proxyIte->second->originHostId();
                        int32_t currentHostId =
                                proxyIte->second->currentHostId();
                        if (originHostId != currentHostId &&
                            originHostId == mesg.arg1) {
                            DD("update host id %d -> %d", originHostId,
                               currentHostId);
                            mesg.arg1 = currentHostId;
                        }
                        proxyIte->second->onGuestSendData(
                                &mesg, mCurrentGuestSendPacket.data.data(),
                                &shouldPush, &sendDataQueue);
                    }
                    checkRemoveProxy(proxyIte);
                }
            }
            if (shouldPush) {
                pushToSendQueue(std::move(mCurrentGuestSendPacket));
            }
            while (sendDataQueue.size()) {
                pushToSendQueue(std::move(sendDataQueue.front()));
                sendDataQueue.pop();
            }
            mCurrentGuestSendPacket.mesg.data_length = 0;
            mCurrentGuestSendPacket.data.resize(0);
            mCurrentGuestSendPacketPst = 0;
        }
    }
    if (actualSendBytes == 0) {
        return PIPE_ERROR_AGAIN;
    } else {
        return actualSendBytes;
    }
}

int AdbHub::onGuestRecvData(AndroidPipeBuffer* buffers, int numBuffers) {
    int currentBuffer = 0;
    size_t currentPst = 0;
    int actualRecvBytes = 0;
    while (currentBuffer < numBuffers) {
        if (mCurrentGuestRecvPacketPst == -1 ||
            mCurrentGuestRecvPacketPst == packetSize(mCurrentGuestRecvPacket)) {
            if (mRecvFromHostQueue.empty()) {
                break;
            }
            mCurrentGuestRecvPacket = std::move(mRecvFromHostQueue.front());
            mRecvFromHostQueue.pop();
            mCurrentGuestRecvPacketPst = 0;
        }
        size_t remainingPacketSize =
                mCurrentGuestRecvPacketPst >= kHeaderSize
                        ? mCurrentGuestRecvPacket.data.size() + kHeaderSize -
                                  mCurrentGuestRecvPacketPst
                        : kHeaderSize - mCurrentGuestRecvPacketPst;
        size_t remainingBufferSize = buffers[currentBuffer].size - currentPst;
        size_t currentReadSize =
                std::min(remainingPacketSize, remainingBufferSize);
        uint8_t* readPtr =
                mCurrentGuestRecvPacketPst >= kHeaderSize
                        ? ((uint8_t*)mCurrentGuestRecvPacket.data.data()) +
                                  mCurrentGuestRecvPacketPst - kHeaderSize
                        : ((uint8_t*)&mCurrentGuestRecvPacket.mesg) +
                                  mCurrentGuestRecvPacketPst;
        memcpy(buffers[currentBuffer].data + currentPst, readPtr,
               currentReadSize);
        currentPst += currentReadSize;
        if (currentPst == buffers[currentBuffer].size) {
            currentPst = 0;
            currentBuffer++;
        }
        mCurrentGuestRecvPacketPst += currentReadSize;
        actualRecvBytes += currentReadSize;
    }
    if (mCurrentGuestRecvPacketPst ==
        mCurrentGuestRecvPacket.data.size() + kHeaderSize) {
        if (mRecvFromHostQueue.empty()) {
            mWantRecv = false;
        }
    }
    DD("Finish recv buffer");
    if (actualRecvBytes) {
        return actualRecvBytes;
    } else {
        return PIPE_ERROR_AGAIN;
    }
}

void AdbHub::onHostSocketEvent(int fd,
                               unsigned events,
                               std::function<void()> onSocketClose) {
    DD("%s: %s", __FILE__, __func__);
    if ((events & base::Looper::FdWatch::kEventRead) != 0) {
        if (readSocket(fd) == PIPE_ERROR_IO) {
            onSocketClose();
            return;
        }
    }
    if ((events & base::Looper::FdWatch::kEventWrite) != 0) {
        if (writeSocket(fd) == PIPE_ERROR_IO) {
            onSocketClose();
            return;
        }
    }
}

int AdbHub::writeSocket(int fd) {
    D("AdbHub writeSocket started");
    while ((mCurrentHostSendPacketPst >= 0 &&
            mCurrentHostSendPacketPst < packetSize(mCurrentHostSendPacket)) ||
           !mSendToHostQueue.empty()) {
        if (mCurrentHostSendPacketPst < 0 ||
            mCurrentHostSendPacketPst == packetSize(mCurrentHostSendPacket)) {
            mCurrentHostSendPacketPst = 0;
            mCurrentHostSendPacket = std::move(mSendToHostQueue.front());
            mSendToHostQueue.pop();
            DD("AdbHub writeSocket new packet size %d",
               (int)packetSize(mCurrentHostSendPacket));
        }
        uint8_t* data = nullptr;
        size_t bytesToSend = 0;
        if (mCurrentHostSendPacketPst < kHeaderSize) {
            bytesToSend = kHeaderSize - mCurrentHostSendPacketPst;
            data = ((uint8_t*)&mCurrentHostSendPacket.mesg) +
                   mCurrentHostSendPacketPst;
        } else {
            bytesToSend = packetSize(mCurrentHostSendPacket) -
                          mCurrentHostSendPacketPst;
            data = mCurrentHostSendPacket.data.data() +
                   mCurrentHostSendPacketPst - kHeaderSize;
        }
        ssize_t len = base::socketSend(fd, data, bytesToSend);
        DD("AdbHub writeSocket sent %d", (int)len);
        if (len > 0) {
            mCurrentHostSendPacketPst += len;
            if (len < bytesToSend) {
                return 0;
            }
        } else if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return PIPE_ERROR_AGAIN;
        } else {
            return PIPE_ERROR_IO;
        }
    }
    D("AdbHub writeSocket done");
    return 0;
}

int AdbHub::readSocket(int fd) {
    D("AdbHub readSocket started");
    while (true) {
        uint8_t* data = nullptr;
        size_t bytesToRecv = 0;
        if (mCurrentHostRecvPacketPst < kHeaderSize) {
            bytesToRecv = kHeaderSize - mCurrentHostRecvPacketPst;
            data = ((uint8_t*)&mCurrentHostRecvPacket.mesg) +
                   mCurrentHostRecvPacketPst;
        } else {
            bytesToRecv = packetSize(mCurrentHostRecvPacket) -
                          mCurrentHostRecvPacketPst;
            data = mCurrentHostRecvPacket.data.data() +
                   mCurrentHostRecvPacketPst - kHeaderSize;
        }
        ssize_t len = base::socketRecv(fd, data, bytesToRecv);
        if (len > 0) {
            mCurrentHostRecvPacketPst += len;
            if (mCurrentHostRecvPacketPst == kHeaderSize) {
                mCurrentHostRecvPacket.data.resize(
                        mCurrentHostRecvPacket.mesg.data_length);
            }
            if (len < bytesToRecv) {
                return 0;
            }
        } else if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return PIPE_ERROR_AGAIN;
        } else {
            return PIPE_ERROR_IO;
        }

        if (mCurrentHostRecvPacketPst == packetSize(mCurrentHostRecvPacket)) {
            DD("Recv new packet");
            apacket& packet = mCurrentHostRecvPacket;
            if (packet.mesg.command == ADB_CNXN && mShouldReconnect) {
                D("Reusing connection");
                apacket sendPacket = mCnxnPacket;
                pushToSendQueue(std::move(sendPacket));
            } else {
                bool shouldForward = true;
                std::queue<apacket> proxySends;
                if (packet.mesg.command == ADB_OPEN) {
                    AdbProxy* proxy = tryReuseConnection(packet);
                    if (proxy) {
                        proxy->onGuestRecvData(&packet.mesg, packet.data.data(),
                                               &shouldForward, &proxySends);
                    } else {
                        mPendingConnections[packet.mesg.arg0] = packet;
                        D("Open command %s", packet.data.data());
                    }
                } else {
                    auto proxyIte = mProxies.find(packet.mesg.arg1);
                    if (proxyIte != mProxies.end()) {
                        if (proxyIte->second->currentHostId() !=
                                    proxyIte->second->originHostId() &&
                            proxyIte->second->currentHostId() ==
                                    packet.mesg.arg0) {
                            packet.mesg.arg0 = proxyIte->second->originHostId();
                        }
                        proxyIte->second->onGuestRecvData(
                                &packet.mesg, packet.data.data(),
                                &shouldForward, &proxySends);
                        checkRemoveProxy(proxyIte);
                    }
                }
                if (shouldForward) {
                    pushToRecvQueue(std::move(packet));
                }
                while (!proxySends.empty()) {
                    pushToSendQueue(std::move(proxySends.front()));
                    proxySends.pop();
                }
            }
            mCurrentHostRecvPacketPst = 0;
        }
    }
    return 0;
}

void AdbHub::pushToSendQueue(apacket&& packet) {
    mSendToHostQueue.push(std::move(packet));
}

void AdbHub::pushToRecvQueue(apacket&& packet) {
    mRecvFromHostQueue.push(std::move(packet));
    DD("Wake pipe read");
    mWantRecv = true;
}

AdbProxy* AdbHub::onNewConnection(const apacket& requestPacket,
                                  const apacket& replyPacket) {
    if (requestPacket.mesg.command != ADB_OPEN ||
        requestPacket.mesg.data_length == 0 ||
        replyPacket.mesg.command != ADB_OKAY) {
        LOG(WARNING) << "WARNING: onNewConnection expect requestPacket command "
                        "ADB_OPEN, actual command "
                     << requestPacket.mesg.command;
        LOG(WARNING) << "WARNING: onNewConnection expect replyPacket command "
                        "ADB_OKAY "
                        "actual command "
                     << replyPacket.mesg.command;
        return nullptr;
    }
    CHECK(requestPacket.mesg.arg0 == replyPacket.mesg.arg1);
    int jdwpId = 0;
    int jdwpConnect =
            sscanf((const char*)requestPacket.data.data(), "jdwp:%d", &jdwpId);
    if (jdwpConnect == 0) {
        return nullptr;
    }
    if (mJdwpProxies.count(jdwpId) == 0) {
        jdwp::JdwpProxy* proxy = new jdwp::JdwpProxy(
                requestPacket.mesg.arg0, replyPacket.mesg.arg0, jdwpId);
        mJdwpProxies[jdwpId].reset(proxy);
        mProxies.emplace(proxy->guestId(), proxy);
        return proxy;
    } else {
        fprintf(stderr, "WARNING: multiple jdwp connection to %d\n", jdwpId);
        return nullptr;
    }
}

AdbProxy* AdbHub::tryReuseConnection(const apacket& packet) {
    int jdwpId = 0;
    int jdwpConnect =
            sscanf((const char*)packet.data.data(), "jdwp:%d", &jdwpId);
    if (jdwpConnect == 0) {
        return nullptr;
    }
    auto jdwp = mJdwpProxies.find(jdwpId);
    if (jdwp == mJdwpProxies.end()) {
        return nullptr;
    }
    if (jdwp->second->currentHostId() > 0) {
        return nullptr;
    }
    jdwp->second->setCurrentHostId(packet.mesg.arg0);
    D("Reusing connection for %s", packet.data.data());
    return jdwp->second.get();
}

int AdbHub::getProxyCount() {
    return mProxies.size();
}

unsigned AdbHub::onGuestPoll() const {
    unsigned ret = PIPE_POLL_OUT;
    if (mWantRecv) {
        ret |= PIPE_POLL_IN;
    }
    return ret;
}

bool AdbHub::socketWantRead() {
    return true;
}

bool AdbHub::socketWantWrite() {
    return !mSendToHostQueue.empty() ||
           (mCurrentHostSendPacketPst >= 0 &&
            mCurrentHostSendPacketPst < packetSize(mCurrentHostSendPacket));
}

int AdbHub::pipeWakeFlags() {
    int wakeFlags = PIPE_WAKE_WRITE;
    if (!mRecvFromHostQueue.empty() ||
        (mCurrentGuestRecvPacketPst >= 0 &&
            mCurrentGuestRecvPacketPst < packetSize(mCurrentGuestRecvPacket))) {
        wakeFlags |= PIPE_WAKE_READ;
    }
    return wakeFlags;
}

}  // namespace emulation
}  // namespace android
