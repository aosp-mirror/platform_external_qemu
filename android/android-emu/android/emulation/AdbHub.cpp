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

#include "android/emulation/apacket_utils.h"
#include "android/base/async/Looper.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/AdbMessageSniffer.h"
#include "android/jdwp/JdwpProxy.h"

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

static const size_t kHeaderSize = sizeof(android::emulation::AdbMessageSniffer::amessage);

namespace android {
namespace emulation {
void AdbHub::Buffer::readBytes(const AndroidPipeBuffer* buffers,
                                  int numBuffers,
                                  int* bufferIdx,
                                  size_t* bufferPst,
                                  size_t* remainBytes,
                                  size_t bytesToRead,
                                  bool skipData,
                                  std::vector<uint8_t>* clonedData) {
    bytesToRead = std::min(*remainBytes, bytesToRead);
    while (bytesToRead > 0 && *bufferIdx < numBuffers) {
        size_t remainBufferSize = buffers[*bufferIdx].size - *bufferPst;
        size_t currentReadSize = std::min(bytesToRead, remainBufferSize);
        if (clonedData) {
            clonedData->insert(clonedData->end(), buffers[*bufferIdx].data + *bufferPst,
                    buffers[*bufferIdx].data + *bufferPst + currentReadSize);
        }
        if (skipData) {
            mBytesToSkip -= currentReadSize;
        } else {
            if (mBufferTail + currentReadSize > mBuffer.size()) {
                mBuffer.resize(mBufferTail + currentReadSize);
            }
            memcpy(mBuffer.data() + mBufferTail, buffers[*bufferIdx].data + *bufferPst,
                   currentReadSize);
            mBufferTail += currentReadSize;
        }
        *bufferPst += currentReadSize;
        bytesToRead -= currentReadSize;
        *remainBytes -= currentReadSize;
        if (*bufferPst == buffers[*bufferIdx].size) {
            (*bufferIdx)++;
            *bufferPst = 0;
        }
    }
}

void AdbHub::Buffer::onSave(android::base::Stream *stream) {
    stream->putBe64(mBufferTail);
    if (mBufferTail > 0) {
        stream->write(mBuffer.data(), mBufferTail);
    }
    stream->putBe64(mBytesToSkip);
}

void AdbHub::Buffer::onLoad(android::base::Stream *stream) {
    mBufferTail = stream->getBe64();
    if (mBufferTail > 0) {
        if (mBufferTail > mBuffer.size()) {
            mBuffer.resize(mBufferTail);
        }
        stream->read(mBuffer.data(), mBufferTail);
    }
    mBytesToSkip = stream->getBe64();
}

AdbHub::~AdbHub() {
    if (mSocket) {
        setSocket(nullptr);
    }
    D("~AdbHub");
}

void AdbHub::onSave(android::base::Stream *stream) {
    stream->putBe32(mJdwpProxies.size());
    for (const auto& proxy: mJdwpProxies) {
        proxy.second->onSave(stream);
    }
    if (mCurrentRecvProxy == mProxies.end()) {
        stream->putBe32(-1);
    } else {
        stream->putBe32(mCurrentRecvProxy->first);
    }
    if (mCurrentSendProxy == mProxies.end()) {
        stream->putBe32(-1);
    } else {
        stream->putBe32(mCurrentSendProxy->first);
    }
    stream->write(&mCnxnPacket.mesg, sizeof(mCnxnPacket.mesg));
    stream->putBe32(mCnxnPacket.data.size());
    stream->write(mCnxnPacket.data.data(), mCnxnPacket.data.size());
}

void AdbHub::onLoad(android::base::Stream *stream) {
    int proxyCount = stream->getBe32();
    for (int i = 0; i < proxyCount; i++) {
        jdwp::JdwpProxy* proxy = new jdwp::JdwpProxy(stream);
        mProxies.emplace(proxy->guestId(), proxy);
        mJdwpProxies.emplace(proxy->mGuestPid, proxy);
    }
    mLoadRecvProxyId = stream->getBe32();
    mLoadSendProxyId = stream->getBe32();
    mCurrentRecvProxy = mProxies.end();
    mCurrentSendProxy = mProxies.end();
    if (mLoadRecvProxyId >= 0) {
        mCurrentRecvProxy = mProxies.find(mLoadRecvProxyId);
        mLoadRecvProxyId = -1;
    }
    if (mLoadSendProxyId >= 0) {
        mCurrentSendProxy = mProxies.find(mLoadSendProxyId);
        mLoadSendProxyId = -1;
    }
    stream->read(&mCnxnPacket.mesg, sizeof(mCnxnPacket.mesg));
    int bufferSize = stream->getBe32();
    mCnxnPacket.data.resize(bufferSize);
    stream->read(mCnxnPacket.data.data(), mCnxnPacket.data.size());
    mShouldReconnect = true;
}

void AdbHub::checkRemoveProxy(std::unordered_map<int, AdbProxy *>::iterator proxy) {
    if (proxy != mProxies.end() && proxy->second->shouldClose()) {
        jdwp::JdwpProxy* jdwpProxy = (jdwp::JdwpProxy*)proxy->second;
        mJdwpProxies.erase(jdwpProxy->mGuestPid);
        mProxies.erase(proxy);
    }
}

int AdbHub::onGuestSendData(const AndroidPipeBuffer* buffers,
                                int numBuffers) {
    if (!mNeedTranslate) {
        return sendToHost(buffers, numBuffers);
    }

    using amessage = emulation::AdbMessageSniffer::amessage;
    int currentBuffer = 0;
    size_t currentPst = 0;
    int actualSendBytes = 0;
    while (currentBuffer < numBuffers) {
        //DD("Enter loop");
        size_t remainingPacketSize = mCurrentGuestSendPacketPst >= kHeaderSize
                            ? mCurrentGuestSendPacket.data.size() + kHeaderSize - mCurrentGuestSendPacketPst
                            : kHeaderSize - mCurrentGuestSendPacketPst;
        size_t remainingBufferSize = buffers[currentBuffer].size - currentPst;
        size_t currentReadSize = std::min(remainingPacketSize, remainingBufferSize);
        uint8_t* writePtr = mCurrentGuestSendPacketPst >= kHeaderSize
                            ? ((uint8_t*)mCurrentGuestSendPacket.data.data()) + mCurrentGuestSendPacketPst - kHeaderSize
                            : ((uint8_t*)&mCurrentGuestSendPacket.mesg) + mCurrentGuestSendPacketPst;
        memcpy(writePtr, buffers[currentBuffer].data + currentPst, currentReadSize);
        currentPst += currentReadSize;
        if (currentPst == buffers[currentBuffer].size) {
            currentPst = 0;
            currentBuffer ++;
        }
        mCurrentGuestSendPacketPst += currentReadSize;
        actualSendBytes += currentReadSize;
        if (mCurrentGuestSendPacketPst == kHeaderSize) {
            mCurrentGuestSendPacket.data.resize(mCurrentGuestSendPacket.mesg.data_length);
        }
        if (mCurrentGuestSendPacketPst >= kHeaderSize &&
            mCurrentGuestSendPacketPst == mCurrentGuestSendPacket.data.size() + kHeaderSize) {
            // handle message
            bool shouldPush = true;
            {
                base::AutoLock lock(mHandlePacketLock);
                amessage& mesg = mCurrentGuestSendPacket.mesg;
                if (mesg.command == ADB_CNXN) {
                    mCnxnPacket = mCurrentGuestSendPacket;
                } else {
                    auto pendingConnection = mPendingConnections.find(mesg.arg1);
                    if (mesg.command == ADB_OKAY && pendingConnection != mPendingConnections.end()) {
                        CHECK(mesg.data_length == 0);
                        onNewConnection(pendingConnection->second, mCurrentGuestSendPacket);
                        mPendingConnections.erase(pendingConnection);
                    } else {
                        auto proxyIte = mProxies.find(mesg.arg0);
                        if (proxyIte != mProxies.end()) {
                            mCurrentSendProxy = proxyIte;
                            int32_t originHostId = mCurrentSendProxy->second->originHostId();
                            int32_t currentHostId = mCurrentSendProxy->second->currentHostId();
                            if (originHostId != currentHostId && originHostId == mesg.arg1) {
                                DD("update host id %d -> %d", originHostId, currentHostId);
                                mesg.arg1 = currentHostId;
                            }
                        }
                        if (proxyIte != mProxies.end()) {
                            proxyIte->second->onGuestSendData(&mesg, mCurrentGuestSendPacket.data.data());
                        }
                        checkRemoveProxy(proxyIte);
                    }
                }
            }
            if (shouldPush) {
                pushToSendQueue(std::move(mCurrentGuestSendPacket));
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

// This function track whether a connection is jdwp or not. It tracks all
// packages until it found a connection package.
int AdbHub::onGuestRecvData(AndroidPipeBuffer* buffers,
                                int numBuffers) {
    if (!mNeedTranslate) {
        return recvFromHost(buffers, numBuffers);
    }

    using amessage = emulation::AdbMessageSniffer::amessage;
    int currentBuffer = 0;
    size_t currentPst = 0;
    int actualRecvBytes = 0;
    while (currentBuffer < numBuffers) {
        //DD("Enter loop");
        if (mCurrentGuestRecvPacketPst == -1 || mCurrentGuestRecvPacketPst == packetSize(mCurrentGuestRecvPacket)) {
            //DD("Acquiring new buffer");
            base::AutoLock lock(mRecvFromHostLock);
            if (mRecvFromHostQueue.empty()) {
                break;
            }
            mCurrentGuestRecvPacket = std::move(mRecvFromHostQueue.front());
            mRecvFromHostQueue.pop();
            mCurrentGuestRecvPacketPst = 0;
            //DD("Acquired new buffer");
        }
        size_t remainingPacketSize = mCurrentGuestRecvPacketPst >= kHeaderSize
                            ? mCurrentGuestRecvPacket.data.size() + kHeaderSize - mCurrentGuestRecvPacketPst
                            : kHeaderSize - mCurrentGuestRecvPacketPst;
        size_t remainingBufferSize = buffers[currentBuffer].size - currentPst;
        size_t currentReadSize = std::min(remainingPacketSize, remainingBufferSize);
        uint8_t* readPtr = mCurrentGuestRecvPacketPst >= kHeaderSize
                            ? ((uint8_t*)mCurrentGuestRecvPacket.data.data()) + mCurrentGuestRecvPacketPst - kHeaderSize
                            : ((uint8_t*)&mCurrentGuestRecvPacket.mesg) + mCurrentGuestRecvPacketPst;
        memcpy(buffers[currentBuffer].data + currentPst, readPtr, currentReadSize);
        currentPst += currentReadSize;
        if (currentPst == buffers[currentBuffer].size) {
            currentPst = 0;
            currentBuffer ++;
        }
        mCurrentGuestRecvPacketPst += currentReadSize;
        actualRecvBytes += currentReadSize;
    }
    if (mCurrentGuestRecvPacketPst == mCurrentGuestRecvPacket.data.size() + kHeaderSize) {
        base::AutoLock lock(mRecvFromHostLock);
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

int AdbHub::sendToHost(const AndroidPipeBuffer *buffers,
                       int numBuffers) {
    if (!mSocket) {
        return PIPE_ERROR_IO;
    }
    int result = 0;
    while (numBuffers > 0) {
        const uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            ssize_t len;
            {
                // Possible that the host socket has been reset.
                if (mSocket && mSocket->valid()) {
                    len = android::base::socketSend(
                            mSocket->fd(), data, dataSize);
                } else {
                    fprintf(stderr, "WARNING: AdbGuestPipe socket closed in the middle of send\n");
                    return PIPE_ERROR_IO;
                }
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // sent less than requested -> no more room.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    //mFdWatcher->dontWantWrite();
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    //mHostSocket.reset();
                    //mState = State::ClosedByHost;
                    //DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            return result;
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

int AdbHub::recvFromHost(AndroidPipeBuffer* buffers, int numBuffers) {
    if (!mSocket) {
        return PIPE_ERROR_IO;
    }
    int result = 0;
    while (numBuffers > 0) {
        uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        DD("%s: [%p] dataSize=%d", __func__, this, (int)dataSize);
        while (dataSize > 0) {
            ssize_t len;
            {
                // Possible that the host socket has been reset.
                if (mSocket->hasStaleData()) {
                    len = mSocket->readStaleData(data, dataSize);
                    DD("%s: [%p] loaded %d data from buffer", __func__, this,
                       (int)len);
                } else if (mSocket->valid()) {
                    len = android::base::socketRecv(
                            mSocket->fd(), data, dataSize);
                } else {
                    fprintf(stderr, "WARNING: AdbGuestPipe socket closed in the middle of recv\n");
                    len = -1;
                }
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // got less than requested -> no more data.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    //mFdWatcher->dontWantRead();
                    //DD("%s: [%p] done try again", __func__, this);
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    //mAdbHub.setSocket(nullptr);
                    //mHostSocket.reset();
                    //mState = State::ClosedByHost;
                    //DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            DD("%s: [%p] done %d", __func__, this, result);
            return result;
        }
        buffers++;
        numBuffers--;
    }
    DD("%s: [%p] done %d", __func__, this, result);
    return result;
}

bool AdbHub::needTranslate() const {
    return mNeedTranslate;
}

void AdbHub::onHostSocketEvent(int fd, unsigned events, std::function<void()> onSocketClose) {
    CHECK(needTranslate());
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
    while ((mCurrentHostSendPacketPst >= 0 && mCurrentHostSendPacketPst < packetSize(mCurrentHostSendPacket))
        || !mSendToHostQueue.empty()) {
        if (mCurrentHostSendPacketPst < 0 || mCurrentHostSendPacketPst == packetSize(mCurrentHostSendPacket)) {
            mCurrentHostSendPacketPst = 0;
            mCurrentHostSendPacket = std::move(mSendToHostQueue.front());
            mSendToHostQueue.pop();
            DD("AdbHub writeSocket new packet size %d", (int)packetSize(mCurrentHostSendPacket));
        }
        uint8_t* data = nullptr;
        size_t bytesToSend = 0;
        if (mCurrentHostSendPacketPst < kHeaderSize) {
            bytesToSend = kHeaderSize - mCurrentHostSendPacketPst;
            data = ((uint8_t*)&mCurrentHostSendPacket.mesg) + mCurrentHostSendPacketPst;
        } else {
            bytesToSend = packetSize(mCurrentHostSendPacket) - mCurrentHostSendPacketPst;
            data = mCurrentHostSendPacket.data.data() + mCurrentHostSendPacketPst - kHeaderSize;
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
            data = ((uint8_t*)&mCurrentHostRecvPacket.mesg) + mCurrentHostRecvPacketPst;
        } else {
            bytesToRecv = packetSize(mCurrentHostRecvPacket) - mCurrentHostRecvPacketPst;
            data = mCurrentHostRecvPacket.data.data() + mCurrentHostRecvPacketPst - kHeaderSize;
        }
        ssize_t len = base::socketRecv(fd, data, bytesToRecv);
        if (len > 0) {
            mCurrentHostRecvPacketPst += len;
            if (mCurrentHostRecvPacketPst == kHeaderSize) {
                mCurrentHostRecvPacket.data.resize(mCurrentHostRecvPacket.mesg.data_length);
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
            base::AutoLock lock(mHandlePacketLock);
            if (packet.mesg.command == ADB_CNXN && mShouldReconnect) {
                D("Reusing connection");
                apacket sendPacket = mCnxnPacket;
                lock.unlock();
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
                        if (proxyIte->second->currentHostId() != proxyIte->second->originHostId() &&
                                proxyIte->second->currentHostId() == packet.mesg.arg0) {
                            packet.mesg.arg0 = proxyIte->second->originHostId();
                        }
                        proxyIte->second->onGuestRecvData(&packet.mesg, packet.data.data(), &shouldForward, &proxySends);
                        checkRemoveProxy(proxyIte);
                    }
                }
                lock.unlock();
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

void AdbHub::setSocket(CrossSessionSocket *socket) {
    int fd = socket && socket->valid() ? socket->fd() : 0;
    mSocket = socket;
    mNeedTranslate = mSocket && mSocket->valid();
    return;
}

void AdbHub::pushToSendQueue(apacket&& packet) {
    mSendToHostLock.lock();
    mSendToHostQueue.push(std::move(packet));
    mSendToHostCV.signalAndUnlock(&mSendToHostLock);
}

void AdbHub::pushToRecvQueue(apacket &&packet) {
    base::AutoLock lock(mRecvFromHostLock);
    mRecvFromHostQueue.push(std::move(packet));
    DD("Wake pipe read");
    mWantRecv = true;
    mWakePipe(PIPE_WAKE_READ);
}

void AdbHub::clearSendBufferLocked() {
    /*while (!mSendToHostQueue.empty()) {
        const AndroidPipeBuffer& buffer = mSendToHostQueue.front();
        delete [] buffer.data;
        mSendToHostQueue.pop();
    }*/
    std::queue<apacket> empty;
    mSendToHostQueue.swap(empty);
}

void AdbHub::clearRecvBufferLocked() {
    std::queue<apacket> empty;
    mRecvFromHostQueue.swap(empty);
}

AdbProxy* AdbHub::onNewConnection(const apacket &requestPacket, const apacket &replyPacket) {
    if (requestPacket.mesg.command != ADB_OPEN
            || replyPacket.mesg.command != ADB_OKAY) {
        fprintf(stderr, "WARNING: onNewConnection expect requestPacket command ADB_OPEN (%d), "
                "actual command %d\n", ADB_OPEN, requestPacket.mesg.command);
        fprintf(stderr, "WARNING: onNewConnection expect replyPacket command ADB_OKAY (%d), "
                "actual command %d\n", ADB_OKAY, replyPacket.mesg.command);
        return nullptr;
    }
    CHECK(requestPacket.mesg.arg0 == replyPacket.mesg.arg1);
    int jdwpId = 0;
    int jdwpConnect = sscanf((const char*)requestPacket.data.data(),
            "jdwp:%d", &jdwpId);
    if (jdwpConnect == 0) {
        return nullptr;
    }
    if (mJdwpProxies.count(jdwpId) == 0) {
        jdwp::JdwpProxy* proxy = new jdwp::JdwpProxy(
            requestPacket.mesg.arg0, replyPacket.mesg.arg0, jdwpId
        );
        mJdwpProxies[jdwpId].reset(proxy);
        mProxies.emplace(proxy->guestId(), proxy);
        return proxy;
    } else {
        fprintf(stderr, "WARNING: reconnecting to jdwp %d\n", jdwpId);
        return nullptr;
    }
}

AdbProxy* AdbHub::tryReuseConnection(const apacket &packet) {
    int jdwpId = 0;
    int jdwpConnect = sscanf((const char*)packet.data.data(),
            "jdwp:%d", &jdwpId);
    if (jdwpConnect == 0) {
        return nullptr;
    }
    auto jdwp = mJdwpProxies.find(jdwpId);
    if (jdwp == mJdwpProxies.end()) {
        return nullptr;
    }
    if (jdwp->second->mCurrentHostId > 0) {
        return nullptr;
    }
    jdwp->second->mCurrentHostId = packet.mesg.arg0;
    CHECK(jdwp->second->mClientState == jdwp::JdwpProxy::Uninitialized);
    D("Reusing connection for %s", packet.data.data());
    return jdwp->second.get();
}

int AdbHub::getProxyCount() {
    return mProxies.size();
}

void AdbHub::setWakePipeFunc(WakePipeFunc wakePipe) {
    mWakePipe = wakePipe;
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
    return !mSendToHostQueue.empty() || (mCurrentHostSendPacketPst >= 0
        && mCurrentHostSendPacketPst < packetSize(mCurrentHostSendPacket));
}

size_t AdbHub::printToBuffers(
                        AndroidPipeBuffer* dstBuffers,
                        int dstNumBuffers,
                        int* dstBufferIdx,       // input / output
                        size_t* dstBufferPst,    // input / output
                        uint8_t* srcBuffer,
                        size_t srcBufferSize) {
    size_t bytesWritten = 0;    
    while (srcBufferSize > 0 && *dstBufferIdx < dstNumBuffers) {
        size_t remainBufferSize = dstBuffers[*dstBufferIdx].size - *dstBufferPst;
        size_t currentWriteSize = std::min(srcBufferSize, remainBufferSize);
        memcpy(dstBuffers[*dstBufferIdx].data + *dstBufferPst, srcBuffer, currentWriteSize);
        srcBuffer += currentWriteSize;
        srcBufferSize -= currentWriteSize;
        bytesWritten += currentWriteSize;
        *dstBufferPst += currentWriteSize;
        if (*dstBufferPst == dstBuffers[*dstBufferIdx].size) {
            (*dstBufferIdx) ++;
            *dstBufferPst = 0;
        }
    }
    return bytesWritten;
}

}  // namespace emulation
}  // namespace android
