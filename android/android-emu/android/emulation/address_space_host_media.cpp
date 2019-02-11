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

AddressSpaceHostMediaContext::AddressSpaceHostMediaContext(uint64_t phys_addr) {
    allocatePages(phys_addr, kNumPages);
}

void AddressSpaceHostMediaContext::perform(AddressSpaceDevicePingInfo *info) {
    handleMediaRequest(info);
}

void AddressSpaceHostMediaContext::allocatePages(uint64_t phys_addr, int num_pages) {
    void* myptr = android::aligned_buf_alloc(kAlignment, num_pages * 4096);
    gQAndroidVmOperations->mapUserBackedRam(phys_addr, myptr, num_pages * 4096);
    AS_DEVICE_DPRINT("Allocating host memory for media context: guest_addr 0x%11x, 0x%11x",
                     phys_addr, myptr);
}

void AddressSpaceHostMediaContext::handleMediaRequest(AddressSpaceDevicePingInfo *info) {
    mVpxDecoder.handlePing(info->metadata,
                           AddressSpaceHostMediaContext::getHostAddress(info->phys_addr));
}

// static
void* AddressSpaceHostMediaContext::getHostAddress(uint64_t guest_phys_addr) {
    void* ptr = gQAndroidVmOperations->physicalMemoryGetAddr(guest_phys_addr);
    return ptr;
}

}  // namespace emulation
}  // namespace android
