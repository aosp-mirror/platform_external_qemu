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

#include <vector>

namespace android {
namespace emulation {

class AddressSpaceGraphicsContext : public AddressSpaceDeviceContext {
public:
    enum Command {
        AllocOrGetOffset = 0,
        GetSize = 1,
        GuestInitializedRings = 2,
        NotifyAvailable = 3,
        Echo = 4,
        EchoAsync = 5,
        EchoAsyncStop = 6,
    };

    static void init(const address_space_device_control_ops *ops);
    static void clear();

    AddressSpaceGraphicsContext();
    ~AddressSpaceGraphicsContext();

    void perform(AddressSpaceDevicePingInfo *info) override;

    AddressSpaceDeviceType getDeviceType() const override;
    void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

    struct Ring {
        ring_buffer* ring;
        ring_buffer_view view;
    };

private:
    enum ThreadCommand {
        Wakeup = 0,
        Sleep = 1,
        Exit = 2,
    };

    enum ConsumerState {
        NeedNotify = 0,
        CanConsume = 1,
    };

    void echoAsync();
    void threadFunc();

    char* mBuffer = 0;
    size_t mReadPos = 0;
    std::vector<char> mReadBuffer;

    Ring mToHost;
    Ring mFromHost;

    uint32_t mRingVersion;
    base::FunctorThread mThread;
    base::MessageChannel<ThreadCommand, 4> mThreadMessages;
    uint32_t* mConsumerState;
    uint32_t mExiting = 0;
};

}  // namespace emulation
}  // namespace android
