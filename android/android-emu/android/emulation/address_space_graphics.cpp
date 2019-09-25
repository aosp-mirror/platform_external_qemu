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

#include "android/emulation/address_space_graphics.h"

#include "android/emulation/address_space_device.hpp"
#include "android/emulation/address_space_device.h"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/ring_buffer.h"
#include "android/base/SubAllocator.h"
#include "android/crashreport/crash-handler.h"

#include <memory>

#define ADDRESS_SPACE_GRAPHICS_PAGE_SIZE 4096
#define ADDRESS_SPACE_GRAPHICS_RING_SIZE 16384
#define ADDRESS_SPACE_GRAPHICS_MAX_CONTEXTS 256

// One control page,
// two rings per context (guest to host, host to guest),
// each of
// ADDRESS_SPACE_GRAPHICS_MAX_CONTEXTS contexts
#define ADDRESS_SPACE_GRAPHICS_CONTEXT_ALLOCATION_SIZE \
    (ADDRESS_SPACE_GRAPHICS_PAGE_SIZE + 2 * ADDRESS_SPACE_GRAPHICS_RING_SIZE)

#define ADDRESS_SPACE_GRAPHICS_BACKING_SIZE \
    (ADDRESS_SPACE_GRAPHICS_CONTEXT_ALLOCATION_SIZE * ADDRESS_SPACE_GRAPHICS_MAX_CONTEXTS)

using android::base::LazyInstance;
using android::base::SubAllocator;

namespace android {
namespace emulation {

class GraphicsBackingMemory {
public:
    GraphicsBackingMemory() :
        mMemory(
            aligned_buf_alloc(
                ADDRESS_SPACE_GRAPHICS_PAGE_SIZE,
                ADDRESS_SPACE_GRAPHICS_BACKING_SIZE)) {
        if (!mMemory) {
            crashhandler_die(
                "Failed to allocate graphics backing memory. Wanted %zu bytes",
                ADDRESS_SPACE_GRAPHICS_BACKING_SIZE);
        }

        mSubAllocator.reset(new SubAllocator(
            mMemory,
            ADDRESS_SPACE_GRAPHICS_BACKING_SIZE,
            ADDRESS_SPACE_GRAPHICS_PAGE_SIZE));
    }

    void clear() {
        get_address_space_device_hw_funcs()->freeSharedHostRegionLocked(
            mPhysAddr -
            get_address_space_device_hw_funcs()->getPhysAddrStartLocked());
        mPhysAddr = 0;

        if (mInitialized && m_ops) {
            m_ops->remove_memory_mapping(mPhysAddr, mMemory, ADDRESS_SPACE_GRAPHICS_BACKING_SIZE);
            mInitialized = false;
        }
    }

    ~GraphicsBackingMemory() {
        clear();

        mSubAllocator.reset();

        if (mMemory)
            aligned_buf_free(mMemory);
    }

    void initialize(const address_space_device_control_ops* ops) {
        if (mInitialized) return;

        uint64_t offset;

        int allocRes = get_address_space_device_hw_funcs()->allocSharedHostRegionLocked(
            ADDRESS_SPACE_GRAPHICS_BACKING_SIZE, &offset);

        if (allocRes) {
            crashhandler_die(
                "Failed to allocate physical address graphics backing memory.");
        }

        mPhysAddr =
            get_address_space_device_hw_funcs()->getPhysAddrStartLocked() +
                offset;

        m_ops = ops;
        m_ops->add_memory_mapping(mPhysAddr, mMemory, ADDRESS_SPACE_GRAPHICS_BACKING_SIZE);
        mInitialized = true;
    }

    char* allocContextBuffer() {
        return (char*)mSubAllocator->alloc(ADDRESS_SPACE_GRAPHICS_CONTEXT_ALLOCATION_SIZE);
    }

    uint64_t getOffset(char* ptr) {
        return mSubAllocator->getOffset(ptr);
    }

    void freeContextBuffer(char* ptr) {
        mSubAllocator->free(ptr);
    }

    void save(base::Stream* stream) {
    }

    bool load(base::Stream* stream) {
        clear();
    }

private:
    uint64_t mPhysAddr = 0;
    void* mMemory = 0;
    std::unique_ptr<SubAllocator> mSubAllocator;
    const address_space_device_control_ops *m_ops = 0;  // do not save/load
    bool mInitialized = false;
};

static LazyInstance<GraphicsBackingMemory> sGraphicsBackingMemory = LAZY_INSTANCE_INIT;

// static
void AddressSpaceGraphicsContext::init(const address_space_device_control_ops* ops) {
    sGraphicsBackingMemory->initialize(ops);
}

// static
void AddressSpaceGraphicsContext::clear() {
    sGraphicsBackingMemory->clear();
}

AddressSpaceGraphicsContext::AddressSpaceGraphicsContext() = default;
AddressSpaceGraphicsContext::~AddressSpaceGraphicsContext() {
    if (mBuffer) sGraphicsBackingMemory->freeContextBuffer(mBuffer);
}

void AddressSpaceGraphicsContext::perform(AddressSpaceDevicePingInfo *info) {
    uint64_t result;

    switch (static_cast<GraphicsCommand>(info->metadata)) {
    case GraphicsCommand::AllocOrGetOffset:
        if (!mBuffer) {
            mBuffer = sGraphicsBackingMemory->allocContextBuffer();
        }

        result = sGraphicsBackingMemory->getOffset(mBuffer);
        break;
    case GraphicsCommand::NotifyAvailable:
        result = 0;
        break;
    default:
        result = -1;
        break;
    }

    info->metadata = result;
}

AddressSpaceDeviceType AddressSpaceGraphicsContext::getDeviceType() const {
    return AddressSpaceDeviceType::Graphics;
}

void AddressSpaceGraphicsContext::save(base::Stream* stream) const {
}

bool AddressSpaceGraphicsContext::load(base::Stream* stream) {
}

}  // namespace emulation
}  // namespace android
