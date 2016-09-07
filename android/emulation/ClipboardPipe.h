// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/AndroidPipe.h"

#include <cassert>
#include <functional>
#include <vector>

namespace android {
namespace emulation {

// This is a special pipe for updating the contents of guest system's
// clipboard. The other side is connected to Android's clipboard service
// that has been patched to listen messages from it.
class ClipboardPipe : public AndroidPipe {
public:
    using GuestClipboardCallback = std::function<void(const uint8_t*, size_t)>;

    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("clipboard") {}
        AndroidPipe* create(void* hwPipe, const char* args) override {
            assert(!mPipeInstance);
            ClipboardPipe* pipe = new ClipboardPipe(hwPipe, this);
            mPipeInstance = pipe;
            return pipe;
        }

        static ClipboardPipe* getPipeInstance() { return mPipeInstance; }

    private:
        static ClipboardPipe* mPipeInstance;
    };

    ClipboardPipe(void* hwPipe, Service* svc);

    void onGuestClose() override {}
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {
        mWakeOnRead = (flags & PIPE_WAKE_READ) != 0;
    }

    static void setGuestClipboardCallback(GuestClipboardCallback cb);
    void setGuestClipboardContents(const uint8_t* buf, size_t len);

private:
    struct ReadWriteState {
        bool sizeProcessed;
        uint32_t requiredBytes;
        uint32_t processedBytes;
        std::vector<uint8_t> buffer;
    };

    enum class OperationType {
        ReadFromGuestClipboard,
        WriteToGuestClipboard
    };
    int processOperation(OperationType type,
                         ReadWriteState* state,
                         const AndroidPipeBuffer* pipeBuffers,
                         int numPipeBuffers,
                         bool* opComplete);

    static GuestClipboardCallback mGuestClipboardCallback;

    ReadWriteState mGuestClipboardReadState = {false, 0, 0};
    ReadWriteState mGuestClipboardWriteState = {false, 0, 0};
    bool mHostClipboardHasNewData = false;
    bool mWakeOnRead = false;
};

void registerClipboardPipeService();
}
}

