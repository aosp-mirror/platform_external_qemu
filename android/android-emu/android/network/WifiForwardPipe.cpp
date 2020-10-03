// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "WifiForwardPipe.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/synchronization/Lock.h"
#include "android/network/WifiForwardClient.h"
#include "android/network/WifiForwardPeer.h"
#include "android/network/WifiForwardServer.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/emulation/AndroidPipe.h"

#include <memory>
#include <thread>

using android::network::WifiForwardPeer;
using android::network::WifiForwardClient;
using android::network::WifiForwardMode;
using android::network::WifiForwardServer;

namespace {

static const size_t kReceiveBufferSize = 131072;

static const char kWifiForwardPipeName[] = "qemud:wififorward";

template<typename T>
static const T addAndWrap(T left, T right, T upperLimit) {
    T result = left + right;
    if (result >= upperLimit) {
        return 0;
    }
    return result;
}

class WifiForwardPipe : public android::AndroidPipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service(WifiForwardMode mode, uint16_t port) :
            android::AndroidPipe::Service(kWifiForwardPipeName),
            mPipe(nullptr) {
            auto onData = [this](const uint8_t* data, size_t size) {
                return this->onDataAvailable(data, size);
            };

            switch (mode) {
                case WifiForwardMode::Client:
                    mPeer.reset(new WifiForwardClient(port, onData));
                    break;
                case WifiForwardMode::Server:
                    mPeer.reset(new WifiForwardServer(port, onData));
                    break;
            }
            mPeer->init();
            mPeer->run();
        }

        ~Service() {
            if (mPeer) {
                mPeer->stop();
            }
        }

        AndroidPipe* create(void* hwPipe, const char* /*args*/) override {
            if (mPipe) {
                // Only one pipe can exist at a time, otherwise different pipes
                // will compete for reading data from another emulator and will
                // not receive a complete packet stream.
                return nullptr;
            }
            mPipe = new WifiForwardPipe(hwPipe, this);
            return mPipe;
        }

        void onClose(WifiForwardPipe* pipe) {
            if (mPipe == pipe) {
                // The pipe is closing, forget about it so that a new one can
                // be created if the guest opens another pipe.
                mPipe = nullptr;
            }
        }

        size_t send(const uint8_t* data, size_t size) {
            return mPeer->queue(data, size);
        }

    private:
        size_t onDataAvailable(const uint8_t* data, size_t size) {
            if (mPipe) {
                return mPipe->onDataAvailable(data, size);
            } else {
                return 0;
            }
        }

        WifiForwardPipe* mPipe;
        std::unique_ptr<WifiForwardPeer> mPeer;
    };

    WifiForwardPipe(void* hwPipe, Service* service)
        : android::AndroidPipe(hwPipe, service),
          mService(service),
          mDataAvailable(false),
          mReceiveBuffer(kReceiveBufferSize),
          mReceivePos(0),
          mForwardPos(0) {
    }

    void onGuestClose(PipeCloseReason /*reason*/) override {
        mService->onClose(this);
    }

    unsigned onGuestPoll() const override {
        if (mDataAvailable) {
            return PIPE_POLL_OUT | PIPE_POLL_IN;
        }
        return PIPE_POLL_OUT;
    }

    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        size_t totalRead = 0;

        android::base::AutoLock lock(mReceiveBufferLock);
        if (mReceivePos == mForwardPos) {
            // There is nothing to read
            return PIPE_ERROR_AGAIN;
        }

        for (size_t i = 0; i < numBuffers && mForwardPos != mReceivePos; ++i) {
            uint8_t* dest = buffers[i].data;
            size_t bytes = buffers[i].size;
            if (mForwardPos > mReceivePos) {
                // Send from the end of the receive buffer
                bytes = std::min(bytes, mReceiveBuffer.size() - mForwardPos);
                memcpy(dest, &mReceiveBuffer[mForwardPos], bytes);
                totalRead += bytes;
                mForwardPos = addAndWrap(mForwardPos,
                                         bytes,
                                         mReceiveBuffer.size());
                if (bytes >= buffers[i].size) {
                    // We filled this entire buffer, keep going with a new one
                    continue;
                }
                // There's still room left in this buffer, update destination
                // and remaining bytes.
                dest += bytes;
                bytes = buffers[i].size - bytes;
            }
            // This conditional might be true after the previous one was true
            // as well in case the buffer wrapped around. This is intentional.
            if (mForwardPos < mReceivePos) {
                // Forward from the beginning of the buffer
                bytes = std::min(bytes, mReceivePos - mForwardPos);
                memcpy(dest, &mReceiveBuffer[mForwardPos], bytes);
                totalRead += bytes;
                mForwardPos = addAndWrap(mForwardPos,
                                         bytes,
                                         mReceiveBuffer.size());
                // At this point either the forward position has reached the
                // receive position which ends the loop or we filled this buffer
                // and the loop moves on to the next buffer.
            }
        }
        mDataAvailable = mForwardPos != mReceivePos;
        return static_cast<int>(totalRead);
    }

    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers,
                    void** newPipePtr) override {
        int transferred = 0;
        for (int i = 0; i < numBuffers; ++i) {
            size_t sent = mService->send(buffers[i].data, buffers[i].size);
            if (sent == 0) {
                // It's currently not possible to send more data, the socket
                // buffers might be full or the peer might have disconnected.
                break;
            }
            transferred += sent;
        }
        return transferred;
    }

    void onGuestWantWakeOn(int flags) override {
    }

    size_t onDataAvailable(const uint8_t* data, size_t size) {
        android::base::AutoLock lock(mReceiveBufferLock);
        size_t ret = 0;
        if (mReceivePos >= mForwardPos) {
            // We're receiving at the end of the buffer and possibly at the
            // beginning after that.
            size_t bufferSpace = mReceiveBuffer.size() - mReceivePos;
            if (mForwardPos == 0) {
                // If the forward position is at the beginning we need to leave
                // an empty byte at the end to avoid indicating an empty buffer.
                --bufferSpace;
            }
            size_t bytes = std::min(size, bufferSpace);
            if (bytes > 0) {
                memcpy(&mReceiveBuffer[mReceivePos], data, bytes);
                mReceivePos = addAndWrap(mReceivePos,
                                         bytes,
                                         mReceiveBuffer.size());
                // Update source data info for copying to front of buffer
                data += bytes;
                size -= bytes;
                ret += bytes;
            }
        }
        // This conditional might be true after the previous one was true as
        // well in case the buffer wrapped around. This is intentional.
        if (mReceivePos < mForwardPos) {
            // We're receiving at the beginning of the buffer. If needed leave a
            // byte between receive and forward position to distinguish between
            // a full and empty buffer.
            size_t bytes = std::min(size, mForwardPos - mReceivePos - 1);
            if (bytes > 0) {
                memcpy(&mReceiveBuffer[mReceivePos], data, bytes);
                mReceivePos = addAndWrap(mReceivePos,
                                         bytes,
                                         mReceiveBuffer.size());
                ret += bytes;
            }
        }
        mDataAvailable = mForwardPos != mReceivePos;
        lock.unlock();
        signalWake(PIPE_WAKE_READ);
        return ret;
    }

private:
    Service* mService;
    std::atomic<bool> mDataAvailable;
    std::vector<uint8_t> mReceiveBuffer;
    size_t mReceivePos;
    size_t mForwardPos;
    android::base::Lock mReceiveBufferLock;
};

}

namespace android {
namespace network {

static WifiForwardPipe::Service* sWifiForwardPipeService = nullptr;

void registerWifiForwardPipeService(WifiForwardMode mode, uint16_t port) {
    if (sWifiForwardPipeService == nullptr) {
        sWifiForwardPipeService = new WifiForwardPipe::Service(mode, port);
        android::AndroidPipe::Service::add(
            std::unique_ptr<WifiForwardPipe::Service>(sWifiForwardPipeService));
    }
}

}  // namespace network
}  // namespace android

