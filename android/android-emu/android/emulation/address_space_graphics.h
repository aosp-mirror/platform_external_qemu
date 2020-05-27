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
#include "android/emulation/address_space_graphics_types.h"

#include <functional>
#include <vector>

namespace android {
namespace emulation {
namespace asg {

struct Allocation {
    char* buffer = nullptr;
    size_t blockIndex = 0;
    uint64_t offsetIntoPhys = 0;
    uint64_t size = 0;
    bool dedicated = false;
    uint64_t hostmemId = 0;
    bool isView = false;
};

class AddressSpaceGraphicsContext : public AddressSpaceDeviceContext {
public:
    AddressSpaceGraphicsContext(bool isVirtio = false);
    ~AddressSpaceGraphicsContext();

    static void setConsumer(ConsumerInterface);
    static void init(const address_space_device_control_ops *ops);
    static void clear();

    void perform(AddressSpaceDevicePingInfo *info) override;
    AddressSpaceDeviceType getDeviceType() const override;

    void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

private:
    // For consumer communication
    enum ConsumerCommand {
        Wakeup = 0,
        Sleep = 1,
        Exit = 2,
    };

    // For ConsumerCallbacks
    int onUnavailableRead();

    // Data layout
    uint32_t mVersion = 1;
    Allocation mRingAllocation;
    Allocation mBufferAllocation;
    Allocation mCombinedAllocation;
    struct asg_context mHostContext = {};

    // Consumer storage
    ConsumerCallbacks mConsumerCallbacks;
    ConsumerInterface mConsumerInterface;
    void* mCurrentConsumer = 0;

    // Communication with consumer
    base::MessageChannel<ConsumerCommand, 4> mConsumerMessages;
    uint32_t mExiting = 0;
    // For onUnavailableRead
    uint32_t mUnavailableReadCount = 0;

    bool mIsVirtio = false;
    // To save the ring config if it is cleared on hostmem map
    struct asg_ring_config mSavedConfig;
};

}  // namespace asg
}  // namespace android
}  // namespace emulation
