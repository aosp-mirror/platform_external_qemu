// Copyright 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/emulation/AddressSpaceService.h"

#include "android/base/ring_buffer.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/address_space_device.h"

#include <functional>
#include <vector>

namespace android {
namespace emulation {

class AddressSpaceGraphicsContext : public AddressSpaceDeviceContext {
public:
    // One control page,
    // two rings per context (guest to host, host to guest),
    // each of
    // AddressSpaceGraphicsContext::kMaxContexts contexts
    static const int kPageSize;
    static const int kRingInfoSize;
    static const int kRingSize;
    static const int kMaxContexts;
    static const int kContextAllocationSize;
    static const int kBackingSize;
    static const int kToHostRingInfoOffset;
    static const int kFromHostRingInfoOffset;
    static const int kToHostRingBufferOffset;
    static const int kFromHostRingBufferOffset;

    using OnUnavailableReadCallback =
        std::function<int()>;
    using ConsumerCreateCallback =
        std::function<void* (struct ring_buffer_with_view,
                             struct ring_buffer_with_view,
                             OnUnavailableReadCallback)>;
    using ConsumerDestroyCallback =
        std::function<void(void*)>;

    enum Command {
        AllocOrGetOffset = 0,
        GetSize = 1,
        GuestInitializedRings = 2,
        NotifyAvailable = 3,
        Echo = 4,
        EchoAsync = 5,
        EchoAsyncStart = 6,
        EchoAsyncStop = 7,
        EchoAsyncWithHangup = 8,
        CreateConsumer = 9,
        DestroyConsumer = 10,
    };

    enum ConsumerState {
        NeedNotify = 0,
        CanConsume = 1,
        CanConsumeButWillHangup = 2,
    };


    static void init(const address_space_device_control_ops *ops);
    static void clear();

    AddressSpaceGraphicsContext();
    ~AddressSpaceGraphicsContext();

    void perform(AddressSpaceDevicePingInfo *info) override;

    AddressSpaceDeviceType getDeviceType() const override;

    void setConsumer(
        ConsumerCreateCallback createFunc,
        ConsumerDestroyCallback destroyFunc);

    void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

private:
    enum ThreadCommand {
        Wakeup = 0,
        Sleep = 1,
        Exit = 2,
    };

    void echoAsync();
    ConsumerState consumeReadbackLoop(ConsumerState);
    void threadFunc();

    int onUnavailableRead();

    char* mBuffer = 0;
    size_t mReadPos = 0;
    std::vector<char> mReadBuffer;

    struct ring_buffer_with_view mToHost;
    struct ring_buffer_with_view mFromHost;

    uint32_t mRingVersion;
    base::FunctorThread mThread;
    base::MessageChannel<ThreadCommand, 4> mThreadMessages;
    base::MessageChannel<ThreadCommand, 4> mThreadReturnMessages;
    ConsumerState* mConsumerStatePtr;
    uint32_t mExiting = 0;

    // For general render threads
    void* mCurrentConsumer = 0;
    ConsumerCreateCallback mConsumerCreateFunc;
    ConsumerDestroyCallback mConsumerDestroyFunc;
    uint32_t mUnavailableReadCount = 0;

};

}  // namespace emulation
}  // namespace android
