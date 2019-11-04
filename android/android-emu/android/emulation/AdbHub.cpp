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
    // TODO
    setAllowHijackJdwp(true);
}

void AdbHub::insertProxy(AdbProxy *proxy) {
    mProxies.emplace(proxy->guestId(), proxy);
}

void AdbHub::doneInsertProxy() {
    if (mLoadRecvProxyId >= 0) {
        mCurrentRecvProxy = mProxies.find(mLoadRecvProxyId);
        mLoadRecvProxyId = -1;
    }
    if (mLoadSendProxyId >= 0) {
        mCurrentSendProxy = mProxies.find(mLoadSendProxyId);
        mLoadSendProxyId = -1;
    }
}

void AdbHub::checkRemoveProxy(std::unordered_map<int, AdbProxy *>::iterator proxy) {
    if (proxy != mProxies.end() && proxy->second->shouldClose()) {
        jdwp::JdwpProxy* jdwpProxy = (jdwp::JdwpProxy*)proxy->second;
        mJdwpProxies.erase(jdwpProxy->mGuestPid);
        mProxies.erase(proxy);
    }
}

int AdbHub::onGuestSendData(const AndroidPipeBuffer* buffers,
                                int numBuffers,
                                OnNewConnection _onNewConnection) {
    if (mShouldBlock) {
        D("Blocking send");
        // TODO: reduce the spin
        return PIPE_ERROR_AGAIN;
    }
    const int kMaxBufferSize = 2000000000; // ~2^31 max buffer size
    int actualSentBytesI = 0;
    std::vector<uint8_t> translatedBuffer;
    //size_t translatedBufferTail = 0;
    if (mNeedTranslate) {
        for (int i = 0; i < numBuffers; i++) {
            actualSentBytesI += buffers[i].size;
        }
        actualSentBytesI = std::min(actualSentBytesI, kMaxBufferSize);
        translatedBuffer.reserve(actualSentBytesI);
        DD("Sending %d buffer %p", actualSentBytesI, translatedBuffer.data());
    } else {
        actualSentBytesI = sendToHost(buffers, numBuffers);
        if (actualSentBytesI <= 0) {
            return actualSentBytesI;
        }
    }
    size_t bytesToParse = (size_t)actualSentBytesI;
    using amessage = emulation::AdbMessageSniffer::amessage;
    assert(numBuffers != 0 && bytesToParse != 0);
    const size_t kHeaderSize = sizeof(amessage);
    int currentBuffer = 0;
    size_t currentPst = 0;
    while (currentBuffer < numBuffers && bytesToParse > 0) {
        if (mGuestSendBuffer.mBytesToSkip) {
            std::vector<uint8_t>* clonedBuffer = mShouldForwardSendMessage? &translatedBuffer : nullptr;
            mGuestSendBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                        &currentPst, &bytesToParse,
                                        mGuestSendBuffer.mBytesToSkip, true,
                                        clonedBuffer);
        } else if (mGuestSendBuffer.mBufferTail < kHeaderSize) {
            mShouldForwardSendMessage = true;
            size_t readSize = 0;
            mGuestSendBuffer.readBytes(
                    buffers, numBuffers, &currentBuffer, &currentPst,
                    &bytesToParse,
                    kHeaderSize - mGuestSendBuffer.mBufferTail, false,
                    nullptr);
            if (mGuestSendBuffer.mBufferTail == kHeaderSize) {
                // check header
                amessage* mesg;
                mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
                if (mesg->command == ADB_CNXN) {
                    translatedBuffer.insert(translatedBuffer.end(),
                            (uint8_t*)mGuestSendBuffer.mBuffer.data(),
                            (uint8_t*)mGuestSendBuffer.mBuffer.data() + kHeaderSize);
                } else if (mIgnoreHostIds.count(mesg->arg1)) {
                    mShouldForwardSendMessage = false;
                    if (mesg->command == ADB_CLSE) {
                        mIgnoreHostIds.erase(mesg->arg1);
                    }
                    mGuestSendBuffer.mBytesToSkip = mesg->data_length;
                    mGuestSendBuffer.mBufferTail = 0;
                    mCurrentSendProxy = mProxies.end();
                } else {
                    auto pendingConnection = mPendingConnections.find(mesg->arg1);
                    if (mesg->command == ADB_OKAY && pendingConnection != mPendingConnections.end()) {
                        CHECK(mesg->data_length == 0);
                        apacket replyPacket;
                        replyPacket.mesg = *mesg;
                        onNewConnection(pendingConnection->second, replyPacket);
                        mPendingConnections.erase(pendingConnection);
                        mGuestSendBuffer.mBufferTail = 0;
                        mCurrentSendProxy = mProxies.end();
                    } else {
                        auto proxyIte = mProxies.find(mesg->arg0);
                        if (proxyIte != mProxies.end()) {
                            mCurrentSendProxy = proxyIte;
                            int32_t originHostId = mCurrentSendProxy->second->originHostId();
                            int32_t currentHostId = mCurrentSendProxy->second->currentHostId();
                            if (originHostId != currentHostId && originHostId == mesg->arg1) {
                                DD("update host id %d -> %d", originHostId, currentHostId);
                                mesg->arg1 = currentHostId;
                            }
                        }
                        if (proxyIte == mProxies.end() ||
                            !proxyIte->second->needProxySendData(mesg) ||
                            mesg->data_length == 0) {
                            mGuestSendBuffer.mBytesToSkip = mesg->data_length;
                            mGuestSendBuffer.mBufferTail = 0;
                            mCurrentSendProxy = mProxies.end();
                        }
                        checkRemoveProxy(proxyIte);
                    }
                    translatedBuffer.insert(translatedBuffer.end(),
                            (uint8_t*)mGuestSendBuffer.mBuffer.data(),
                            (uint8_t*)mGuestSendBuffer.mBuffer.data() + kHeaderSize);
                }
            }
        } else {
            std::vector<uint8_t>* clonedBuffer = mShouldForwardSendMessage? &translatedBuffer : nullptr;
            size_t readSize = 0;
            amessage* mesg;
            mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
            mGuestSendBuffer.readBytes(buffers, numBuffers, &currentBuffer,
                                       &currentPst, &bytesToParse,
                                       kHeaderSize + mesg->data_length -
                                               mGuestSendBuffer.mBufferTail,
                                       false,
                                       clonedBuffer);
            // could have been resized
            mesg = (amessage*)mGuestSendBuffer.mBuffer.data();
            if (kHeaderSize + mesg->data_length == mGuestSendBuffer.mBufferTail) {
                if (mesg->command == ADB_CNXN) {
                    D("Recorded ADB_CNXN message");
                    mCnxnPacket.mesg = *mesg;
                    mCnxnPacket.data.assign(mGuestSendBuffer.mBuffer.begin() + kHeaderSize,
                        mGuestSendBuffer.mBuffer.end());
                } else {
                    CHECK(mCurrentSendProxy != mProxies.end());
                    mCurrentSendProxy->second->onGuestSendData(mGuestSendBuffer.mBuffer.data(),
                            mGuestSendBuffer.mBuffer.size());
                    checkRemoveProxy(mCurrentSendProxy);
                }
                mCurrentSendProxy = mProxies.end();
                mGuestSendBuffer.mBufferTail = 0;
            }
        }
    }
    if (mNeedTranslate) {
        int senderResult = pushToSenderQueue(std::move(translatedBuffer));
        if (senderResult < 0) {
            return senderResult;
        }
    }
    return actualSentBytesI;
}

// This function track whether a connection is jdwp or not. It tracks all
// packages until it found a connection package.
int AdbHub::onGuestRecvData(AndroidPipeBuffer* buffers,
                                int numBuffers,
                                OnNewConnection _onNewConnection) {
    if (mShouldBlock) {
        D("Blocking recv");
        // TODO: reduce the spin
        return PIPE_ERROR_AGAIN;
    }
    if (!mNeedTranslate) {
        return recvFromHost(buffers, numBuffers);
    }

    using amessage = emulation::AdbMessageSniffer::amessage;
    const size_t kHeaderSize = sizeof(amessage);
    int currentBuffer = 0;
    size_t currentPst = 0;
    int actualRecvBytes = 0;
    while (currentBuffer < numBuffers) {
        //DD("Enter loop");
        if (mCurrentRecvPacketPst == mCurrentRecvPacket.data.size() + kHeaderSize) {
            //DD("Acquiring new buffer");
            base::AutoLock lock(mRecvFromHostLock);
            if (mRecvFromHostQueue.empty()) {
                break;
            }
            mCurrentRecvPacket = std::move(mRecvFromHostQueue.front());
            mRecvFromHostQueue.pop();
            mCurrentRecvPacketPst = 0;
            //DD("Acquired new buffer");
        }
        size_t remainingPacketSize = mCurrentRecvPacketPst >= kHeaderSize
                            ? mCurrentRecvPacket.data.size() + kHeaderSize - mCurrentRecvPacketPst
                            : kHeaderSize - mCurrentRecvPacketPst;
        size_t remainingBufferSize = buffers[currentBuffer].size - currentPst;
        size_t currentReadSize = std::min(remainingPacketSize, remainingBufferSize);
        uint8_t* readPtr = mCurrentRecvPacketPst >= kHeaderSize
                            ? ((uint8_t*)mCurrentRecvPacket.data.data()) + mCurrentRecvPacketPst - kHeaderSize
                            : ((uint8_t*)&mCurrentRecvPacket.mesg) + mCurrentRecvPacketPst;
        memcpy(buffers[currentBuffer].data + currentPst, readPtr, currentReadSize);
        currentPst += currentReadSize;
        if (currentPst == buffers[currentBuffer].size) {
            currentPst = 0;
            currentBuffer ++;
        }
        mCurrentRecvPacketPst += currentReadSize;
        actualRecvBytes += currentReadSize;
    }
    if (mCurrentRecvPacketPst == mCurrentRecvPacket.data.size() + kHeaderSize) {
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

int AdbHub::pushToSenderQueue(std::vector<uint8_t> &&buffer) {
    mSendToHostLock.lock();
    if (!mSocket || !mSocket->valid()) {
        D("Socket invalidated");
        mSendToHostLock.unlock();
        return PIPE_ERROR_IO;
    } else {
        DD("Pushed send buffer");
        //actualSentBytesI = translatedBuffer.size();
        //if (actualSentBytesI == 0) {
        //    actualSentBytesI = PIPE_ERROR_AGAIN;
        //    D("Ignore all guest send buffers");
        //} else {
        if (buffer.size() == 0) {
            D("Ignore all guest send buffers");
            mSendToHostLock.unlock();
        } else {
            mSendToHostQueue.push(std::move(buffer));
            mSendToHostCV.signalAndUnlock(&mSendToHostLock);
        }
        return buffer.size();
    }
}
void AdbHub::setSocket(CrossSessionSocket *socket) {
    int fd = socket && socket->valid() ? socket->fd() : 0;
    mSocket = socket;
    // TODO
    mNeedTranslate = mSocket && mSocket->valid();
    mSendToHostLock.lock();
    if (mSendToHostThread) {
        mSendToHostThreadShouldKill = true;
        mSendToHostCV.signalAndUnlock(&mSendToHostLock);
        D("Ready to kill sender thread");
        mSendToHostThread->wait();
        D("Killed sender thread");
        mSendToHostLock.lock();
    }
    clearSendBufferLocked();
    mSendToHostThreadShouldKill = false;
    mSendToHostLock.unlock();

    mRecvFromHostLock.lock();
    if (mRecvFromHostThread) {
        mRecvFromHostThreadShouldKill = true;
        mRecvFromHostCV.signalAndUnlock(&mRecvFromHostLock);
        D("Ready to kill sender thread");
        mRecvFromHostThread->wait();
        D("Killed sender thread");
        mRecvFromHostLock.lock();
    }
    clearRecvBufferLocked();
    mRecvFromHostThreadShouldKill = false;
    mRecvFromHostLock.unlock();


    if (mNeedTranslate && mSocket && mSocket->valid()) {
        base::socketSetBlocking(mSocket->fd());
        mSendToHostThread.reset(new base::FunctorThread([this, fd]() {
            D("AdbHub sender thread started");
            while (true) {
                mSendToHostLock.lock();
                if (mSendToHostThreadShouldKill) {
                    break;
                }
                if (mSendToHostQueue.empty()) {
                    mSendToHostCV.wait(&mSendToHostLock);
                    if (mSendToHostThreadShouldKill) {
                        mSendToHostLock.unlock();
                        break;
                    }
                    CHECK(!mSendToHostQueue.empty());
                }
                DD("AdbHub sender thread received data");
                std::vector<uint8_t> buffer = std::move(mSendToHostQueue.front());
                mSendToHostQueue.pop();
                mSendToHostLock.unlock();
                int length = 0;
                while (length < buffer.size()) {
                    int ret = base::socketSend(fd, buffer.data() + length, buffer.size() - length);
                    if (mSendToHostThreadShouldKill) {
                        return;
                    }
                    if (ret < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // TODO: use the fd watcher
                            sleep(1);
                        } else {
                            DD("AdbHub socket closed during send");
                            return;
                        }
                    }
                    length += ret;
                }
            }
            D("AdbHub sender thread terminated");
        }));
        mRecvFromHostThread.reset(new base::FunctorThread([this, fd]() {
            D("AdbHub receiver thread started");
            const size_t kHeaderSize = sizeof(AdbMessageSniffer::amessage);
            while (!mRecvFromHostThreadShouldKill) {
                apacket packet;
                if (!recvPacket(fd, &packet)) {
                    break;
                }
                DD("Recv packet length %d", packet.mesg.data_length);
                if (packet.mesg.command == ADB_CNXN && mShouldReconnect) {
                    D("Reusing connection");
                    std::vector<uint8_t> senderBuffer(kHeaderSize + mCnxnPacket.data.size());
                    memcpy(senderBuffer.data(), &mCnxnPacket.mesg, kHeaderSize);
                    memcpy(senderBuffer.data() + kHeaderSize, mCnxnPacket.data.data(), mCnxnPacket.data.size());
                    //lock.unlock();
                    // TODO: make sure send between commands
                    int senderResult = pushToSenderQueue(std::move(senderBuffer));
                    if (senderResult < 0) {
                        break;
                    }
                } else if (packet.mesg.command == ADB_OPEN) {
                    mPendingConnections[packet.mesg.arg0] = packet;
                    D("Open command %s", packet.data.data());
                    pushToRecvQueue(std::move(packet));
                } else {
                    auto proxyIte = mProxies.find(packet.mesg.arg1);
                    if (proxyIte != mProxies.end()) {
                        if (proxyIte->second->currentHostId() != proxyIte->second->originHostId() &&
                                proxyIte->second->currentHostId() == packet.mesg.arg0) {
                            packet.mesg.arg0 = proxyIte->second->originHostId();
                        }
                        if (proxyIte->second->onRecvDataHeader(&packet.mesg)) {
                            proxyIte->second->onGuestRecvData(&packet.mesg, packet.data.data());
                        }
                        checkRemoveProxy(proxyIte);
                    }
                    pushToRecvQueue(std::move(packet));
                }
            }
            D("AdbHub receiver thread terminated");
        }));
        mSendToHostThread->start();
        mRecvFromHostThread->start();
    }
}

void AdbHub::pushToRecvQueue(apacket &&packet) {
    base::AutoLock lock(mRecvFromHostLock);
    mRecvFromHostQueue.push(std::move(packet));
    if (mRecvFromHostQueue.size() == 1) {
        DD("Wake pipe read");
        mWantRecv = true;
        mWakePipe(PIPE_WAKE_READ);
    }
}

void AdbHub::clearSendBufferLocked() {
    /*while (!mSendToHostQueue.empty()) {
        const AndroidPipeBuffer& buffer = mSendToHostQueue.front();
        delete [] buffer.data;
        mSendToHostQueue.pop();
    }*/
    std::queue<std::vector<uint8_t>> empty;
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
    //} else if (mAllowHijackJdwp) {
    } else if (false) { // TODO: remove this code
        CHECK(mReconnectingProxy == nullptr);
        mReconnectingProxy = mJdwpProxies[jdwpId].get();
        mReconnectingProxy->mCurrentHostId = requestPacket.mesg.arg0;
        // TODO: Send adb close to the guest for the newly created connection
        //mReconnectingProxy->mClientState = jdwp::JdwpProxy::ConnectOk;
        mShouldBlock = true;
        D("Reconnecting jdwp proxy");
        mReconnectingProxy->fakeServerAsync(mSocket->fd(), [this]() {
            mShouldBlock = false;
            //mWakePipe();
        });
        return mReconnectingProxy;
    } else {
        fprintf(stderr, "WARNING: reconnecting to jdwp %d\n", jdwpId);
        return nullptr;
    }
}

bool AdbHub::shouldReuseConnection(const apacket &packet) {
    int jdwpId = 0;
    int jdwpConnect = sscanf((const char*)packet.data.data(),
            "jdwp:%d", &jdwpId);
    if (jdwpConnect == 0) {
        return false;
    }
    auto jdwp = mJdwpProxies.find(jdwpId);
    if (jdwp == mJdwpProxies.end()) {
        return false;
    }
    if (jdwp->second->mCurrentHostId > 0) {
        return false;
    }
    D("Reusing connection for %s", packet.data.data());
    CHECK(mReconnectingProxy == nullptr);
    mReconnectingProxy = jdwp->second.get();
    mReconnectingProxy->mCurrentHostId = packet.mesg.arg0;
    mReconnectingProxy->mClientState = jdwp::JdwpProxy::Connect;
    mShouldBlock = true;
    D("Reconnecting jdwp proxy");
    mReconnectingProxy->fakeServerAsync(mSocket->fd(), [this]() {
        mShouldBlock = false;
        //mWakePipe();
    });
    return true;
}

int AdbHub::getProxyCount() {
    return mProxies.size();
}

void AdbHub::setAllowHijackJdwp(bool allowHijackJdwp) {
    mAllowHijackJdwp = allowHijackJdwp;
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
