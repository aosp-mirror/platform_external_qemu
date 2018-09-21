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
#pragma once

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidPipe.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <array>
#include <unordered_map>

// This is a utility class that can help implement message-based remote calls
// between the host and guest, with optional out-of-band responses.
//
// Usage:
// 1. Inherit AndroidMessagePipe, and implement OnMessage to receive callbacks
//    when messages are received.
// 2. Call Send(data) to send a response to the client.
//
// VERY IMPORTANT: this function runs on a QEMU's CPU thread while BQL is
// locked. This means the OnMessage must be quick, if any complex operation must
// be done do it on another thread and call send(data) from that thread.
//
// 3. Register the service
// void registerMyXYZService() {
//    android::AndroidPipe::Service::add(new
//        android::AndroidAsyncMessagePipe::Service<YourPipe>("MyXYZService");
//}

namespace android {

struct AsyncMessagePipeHandle {
    int id = -1;

    bool isValid() const { return id >= 0; }
};

class AndroidAsyncMessagePipe : public AndroidPipe {
public:
    struct PipeArgs {
        AsyncMessagePipeHandle handle;
        void* hwPipe;
        const char* args;
        base::Stream* loadStream;
        std::function<void(AsyncMessagePipeHandle)> deleter;
    };

    //
    // Pipe Service
    //

    template <typename PipeType>
    class Service : public AndroidPipe::Service {
    public:
        Service(const char* serviceName)
            : AndroidPipe::Service(serviceName) {}

        bool canLoad() const override { return true; }

        virtual AndroidPipe* load(void* hwPipe,
                                  const char* args,
                                  base::Stream* stream) final {
            base::AutoLock lock(mLock);
            AsyncMessagePipeHandle handle = nextPipeHandle();
            PipeArgs pipeArgs = {
                    handle,
                    hwPipe,
                    args,
                    nullptr,
                    std::bind(&Service::destroyPipe, this, std::placeholders::_1)};
            return new PipeType(this, std::move(pipeArgs));
        }

        virtual AndroidPipe* create(void* hwPipe, const char* args) final {
            base::AutoLock lock(mLock);
            AsyncMessagePipeHandle handle = nextPipeHandle();
            PipeArgs pipeArgs = {handle, hwPipe, args, nullptr, std::bind(&Service::destroyPipe, this, std::placeholders::_1)};
            return new PipeType(this, std::move(pipeArgs));
        }

        void destroyPipe(AsyncMessagePipeHandle handle) {
            base::AutoLock lock(mLock);
            const auto it = mPipes.find(handle.id);
            if (it == mPipes.end()) {
                LOG(WARNING) << "Could not find pipe id %d" << handle.id;
                return;
            }

            mPipes.erase(it);
        }

        struct PipeRef {
            PipeType& pipe;
            base::AutoLock lock;
        };

        base::Optional<PipeRef> getPipe(AsyncMessagePipeHandle handle) {
            base::AutoLock lock(mLock);
            const auto it = mPipes.find(handle.id);
            if (it == mPipes.end()) {
                DLOG(VERBOSE) << "Could not find pipe id %d" << handle.id;
                return {};
            }

            return PipeRef{it->get(), std::move(lock)};
        }

    private:
        AsyncMessagePipeHandle nextPipeHandle() {
            return AsyncMessagePipeHandle{mNextId++};
        }

        base::Lock mLock;
        int mNextId = 0;
        std::unordered_map<int, std::unique_ptr<PipeType>> mPipes;
    };

    AndroidAsyncMessagePipe(AndroidPipe::Service* service, PipeArgs&& args);

    void send(const std::vector<uint8_t>& data);
    virtual void onMessage(const std::vector<uint8_t>& data) = 0;

    void onSave(base::Stream* stream) override;
    virtual void onLoad(base::Stream* stream);

protected:
    AsyncMessagePipeHandle getHandle() { return mHandle; }

private:
    bool allowRead() const;

    int readBuffers(const AndroidPipeBuffer* buffers, int numBuffers);
    int writeBuffers(AndroidPipeBuffer* buffers, int numBuffers);

    void onGuestClose(PipeCloseReason reason) final { mDeleter(mHandle); }
    unsigned onGuestPoll() const final;
    void onGuestWantWakeOn(int flags) final {}

    // Rename onGuestRecv/onGuestSend to be in the context of the host.
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) final {
        return writeBuffers(buffers, numBuffers);
    }

    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) final {
        return readBuffers(buffers, numBuffers);
    }

    struct OutgoingPacket {
        size_t offset = 0;
        std::array<uint8_t, 4> messageLength = {};
        std::vector<uint8_t> data;

        OutgoingPacket(std::vector<uint8_t>&& data);

        size_t length() const { return messageLength.size() + data.size(); }
        bool complete() const;

        // Copy the packet contents to the AndroidPipeBuffer, updating
        // writeOffset based on how much was written.
        void copyToBuffer(AndroidPipeBuffer& buffer, size_t* writeOffset);
    };

    struct IncomingPacket {
        size_t headerBytesRead = 0;
        std::array<uint8_t, 4> messageLength = {};
        std::vector<uint8_t> data;

        bool lengthKnown() const;
        base::Optional<size_t> bytesRemaining() const;
        bool complete() const;

        // Read the packet data from an AndroidPipeBuffer, updating the
        // readOffset based on how much was written.
        //
        // Returns the number of bytes read.
        size_t copyFromBuffer(const AndroidPipeBuffer& buffer, size_t* readOffset);
    };

    mutable base::Lock mLock;
    const AsyncMessagePipeHandle mHandle;
    const std::function<void(AsyncMessagePipeHandle)> mDeleter;

    base::Optional<IncomingPacket> mIncomingPacket;
    std::list<OutgoingPacket> mOutgoingPackets;
};

}  // namespace android
