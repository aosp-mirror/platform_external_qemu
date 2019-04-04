#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"

#include "android/emulation/address_space_host_memory_allocator.h"
#include "android/emulation/GoldfishDma.h"
#include "android/emulation/DmaMap.h"

#include "android/emulation/android_pipe_host.h"
#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

using android::emulation::AddressSpaceHostMemoryAllocatorContext;

static void android_goldfish_dma_add_buffer(void* pipe, uint64_t guest_paddr, uint64_t sz) {
    android::DmaMap::get()->addBuffer(pipe, guest_paddr, sz);
}

static void android_goldfish_dma_remove_buffer(uint64_t guest_paddr) {
    android::DmaMap::get()->removeBuffer(guest_paddr);
}

static void* android_goldfish_dma_get_host_addr(uint64_t guest_paddr) {
    void *result;

    result = AddressSpaceHostMemoryAllocatorContext::getHostAddr(guest_paddr);
    if (result) {
        return result;
    }

    result = android::DmaMap::get()->getHostAddr(guest_paddr);
    if (result) {
        return result;
    }

    fprintf(stderr, "%s: guest_paddr=%llx is not found\n", __func__,
            static_cast<unsigned long long>(guest_paddr));
    return NULL;
}

static void android_goldfish_dma_invalidate_host_mappings() {
    android::DmaMap::get()->invalidateHostMappings();
}

static void android_goldfish_dma_unlock(uint64_t guest_paddr) {
    void* hwpipe;

    hwpipe = AddressSpaceHostMemoryAllocatorContext::getPipeInstance(guest_paddr);
    if (!hwpipe) {
        hwpipe = android::DmaMap::get()->getPipeInstance(guest_paddr);
    }

    if (hwpipe) {
        android_pipe_host_signal_wake(hwpipe, PIPE_WAKE_UNLOCK_DMA);
    } else {
        fprintf(stderr, "%s: guest_paddr=%llx is not found\n", __func__,
                static_cast<unsigned long long>(guest_paddr));
    }
}

static void android_goldfish_dma_reset_host_mappings() {
    android::DmaMap::get()->resetHostMappings();
}

static void android_goldfish_dma_save_mappings(android::base::Stream* stream) {
    android::DmaMap::get()->save(stream);
}

static void android_goldfish_dma_load_mappings(android::base::Stream* stream) {
    android::DmaMap::get()->load(stream);
}

const GoldfishDmaOps android_goldfish_dma_ops = {
    .add_buffer = android_goldfish_dma_add_buffer,
    .remove_buffer = android_goldfish_dma_remove_buffer,
    .get_host_addr = android_goldfish_dma_get_host_addr,
    .invalidate_host_mappings = android_goldfish_dma_invalidate_host_mappings,
    .unlock = android_goldfish_dma_unlock,
    .reset_host_mappings = android_goldfish_dma_reset_host_mappings,
    .save_mappings = android_goldfish_dma_save_mappings,
    .load_mappings = android_goldfish_dma_load_mappings,
};
