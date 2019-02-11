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

#include "android/emulation/address_space_host_media.h"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"

#define AS_DEVICE_DEBUG 1

#if AS_DEVICE_DEBUG
#define AS_DEVICE_DPRINT(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define AS_DEVICE_DPRINT(fmt,...)
#endif

namespace android {
namespace emulation {

namespace {

void* getHostAddress(uint64_t guest_phys_addr) {
    void* ptr = gQAndroidVmOperations->physicalMemoryGetAddr(guest_phys_addr);
    return ptr;
}

};  // namespace

AddressSpaceHostMediaContext::AddressSpaceHostMediaContext(uint64_t phys_addr, const address_space_device_control_ops* ops) : mControlOps(ops) {
    allocatePages(phys_addr, kNumPages);
}

void AddressSpaceHostMediaContext::perform(AddressSpaceDevicePingInfo *info) {
    handleMediaRequest(info);
}

AddressSpaceDeviceType AddressSpaceHostMediaContext::getDeviceType() const {
    return AddressSpaceDeviceType::Media;
}

void AddressSpaceHostMediaContext::save(base::Stream* stream) const {
    // TODO: NOT IMPLEMENTED
}

bool AddressSpaceHostMediaContext::load(base::Stream* stream) {
    // TODO: NOT IMPLEMENTED
    return false;
}

void AddressSpaceHostMediaContext::allocatePages(uint64_t phys_addr, int num_pages) {
    mHostBuffer = android::aligned_buf_alloc(kAlignment, num_pages * 4096);
    mControlOps->add_memory_mapping(
        phys_addr, mHostBuffer, num_pages * 4096);
    AS_DEVICE_DPRINT("Allocating host memory for media context: guest_addr 0x%" PRIx64 ", 0x%" PRIx64,
                     (uint64_t)phys_addr, (uint64_t)mHostBuffer);
}

void AddressSpaceHostMediaContext::handleMediaRequest(AddressSpaceDevicePingInfo *info) {
    mVpxDecoder.handlePing(info->metadata,
                           mControlOps->get_host_ptr(info->phys_addr));
}

}  // namespace emulation
}  // namespace android
