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

#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidPipe.h"

#include <functional>
#include <memory>
#include <vector>

namespace android {
namespace emulation {

// This is a special pipe for updating the contents of guest system's
// clipboard. The other side is connected to Android's clipboard service
// that has been patched to listen messages from it.
class ClipboardPipe final : public AndroidPipe {
public:
    using GuestClipboardCallback = std::function<void(const uint8_t*, size_t)>;
    using Ptr = std::shared_ptr<ClipboardPipe>;

    class Service final : public AndroidPipe::Service {
    public:
        Service();
        AndroidPipe* create(void* hwPipe, const char* args) override;

        static ClipboardPipe::Ptr getPipe();
        static void closePipe();
    };

    ClipboardPipe(void* hwPipe, Service* svc);

    void onGuestClose() override;
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
        std::vector<uint8_t> buffer;
        uint32_t requiredBytes = 0;
        uint32_t processedBytes = 0;
        bool sizeProcessed = false;
    };

    enum class OperationType { ReadFromGuestClipboard, WriteToGuestClipboard };

    int processOperation(OperationType type,
                         ReadWriteState* state,
                         const AndroidPipeBuffer* pipeBuffers,
                         int numPipeBuffers,
                         bool* opComplete);

    ReadWriteState mGuestClipboardReadState;
    ReadWriteState mGuestClipboardWriteState;
    bool mHostClipboardHasNewData = false;
    bool mWakeOnRead = false;
};

void registerClipboardPipeService();

}  // namespace emulation
}  // namespace android
