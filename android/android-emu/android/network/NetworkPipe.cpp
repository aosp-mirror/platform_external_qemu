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

#include "NetworkPipe.h"

#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidPipe.h"

#include <atomic>
#include <ctime>
#include <unordered_set>
#include <vector>

namespace {

static const char kNetworkPipeName[] = "qemud:network";

class NetworkPipe : public android::AndroidPipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service() : android::AndroidPipe::Service(kNetworkPipeName) {
        }

        AndroidPipe* create(void* hwPipe, const char* /*args*/) override {
            auto pipe = new NetworkPipe(hwPipe, this);
            onPipeOpen(pipe);
            return pipe;
        }

        void onPipeOpen(NetworkPipe* pipe) {
            mPipes.insert(pipe);
        }

        void onPipeClose(NetworkPipe* pipe) {
            mPipes.erase(pipe);
        }

        NetworkPipe* getPipe() {
            if (!mPipes.empty()) {
                return *mPipes.begin();
            }
            return nullptr;
        }
    private:
        std::unordered_set<NetworkPipe*> mPipes;
    };

    NetworkPipe(void* hwPipe, Service* service)
        : android::AndroidPipe(hwPipe, service),
          mNetworkPipeService(service),
          mDataAvailable(false) {
    }


    int send(const char* data, size_t size) {
        if (size == 0) {
            return 0;
        }
        {
            android::base::AutoLock lock(mLock);
            mTxBuffer.insert(mTxBuffer.end(), data, data + size);
        }
        mDataAvailable = true;
        signalWake(PIPE_WAKE_READ);
        return size;
    }

    void onGuestClose(PipeCloseReason reason) override {
        mNetworkPipeService->onPipeClose(this);
    }

    unsigned onGuestPoll() const override {
        if (mDataAvailable) {
            return PIPE_POLL_OUT | PIPE_POLL_IN;
        }
        return PIPE_POLL_OUT;
    }

    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        if (!mDataAvailable) {
            return PIPE_ERROR_AGAIN;
        }
        android::base::AutoLock lock(mLock);
        size_t pos = 0;
        for (int i = 0; i < numBuffers && pos < mTxBuffer.size(); ++i) {
            if (buffers[i].size == 0) {
                continue;
            }
            size_t bytesToTransfer = std::min(mTxBuffer.size() - pos,
                                              buffers[i].size);
            if (bytesToTransfer > static_cast<size_t>(std::numeric_limits<int>::max()) ||
                static_cast<size_t>(std::numeric_limits<int>::max()) - bytesToTransfer < pos) {
                // Transferring this amount of data would overflow an integer
                // used as the return value to indicate how much data was
                // transferred. Stop here, the device will have to make another
                // read.
                break;
            }

            memcpy(buffers[i].data, &mTxBuffer[pos], bytesToTransfer);
            pos += bytesToTransfer;
        }
        if (pos > 0) {
            mTxBuffer.erase(mTxBuffer.begin(), mTxBuffer.begin() + pos);
            if (mTxBuffer.empty()) {
                mDataAvailable = false;
            }
        }
        return pos;
    }

    int onGuestSend(const AndroidPipeBuffer* buffers,
                    int numBuffers,
                    void** newPipePtr) override {
        int transferred = 0;
        for (int i = 0; i < numBuffers; ++i) {
            transferred += buffers[i].size;
        }

        return transferred;
    }

    void onGuestWantWakeOn(int flags) override {
    }

private:
    Service* mNetworkPipeService;
    std::atomic<bool> mDataAvailable;
    std::vector<char> mTxBuffer;
    android::base::Lock mLock;
    time_t mLastRead = 0;
};


}  // namespace

namespace android {
namespace network {

static NetworkPipe::Service* sNetworkPipeService = nullptr;

void registerNetworkPipeService() {
    if (sNetworkPipeService == nullptr) {
        sNetworkPipeService = new NetworkPipe::Service;
        AndroidPipe::Service::add(
            std::unique_ptr<NetworkPipe::Service>(sNetworkPipeService));
    }
}

int sendNetworkCommand(const char* command, size_t size) {
    if (sNetworkPipeService) {
        NetworkPipe* pipe = sNetworkPipeService->getPipe();
        if (pipe) {
            return pipe->send(command, size);
        }
    }
    return -1;
}


}  // namespace network
}  // namespace android

