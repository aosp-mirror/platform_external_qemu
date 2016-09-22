#include "android/base/memory/LazyInstance.h"

#include "android/emulation/GoldfishDma.h"
#include "android/emulation/DmaMap.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

static GoldfishDmaRegion* sGoldfishDMA = nullptr;

uint32_t android_goldfish_dma_get_bufsize() { return kDmaBufSizeMB * 1024 * 1024; }
void android_goldfish_dma_init(GoldfishDmaRegion* x) { sGoldfishDMA = x; }
void* android_goldfish_dma_read(uint32_t offset) {
    return (void*)((char*)sGoldfishDMA + offset);
}

void android_goldfish_dma_add_buffer(uint64_t guest_paddr, uint64_t sz) {
    android::DmaMap::get()->addBuffer(guest_paddr, sz);
}

void android_goldfish_dma_remove_buffer(uint64_t guest_paddr) {
    android::DmaMap::get()->removeBuffer(guest_paddr);
}

uint64_t android_goldfish_dma_get_host_addr(uint64_t guest_paddr) {
    return android::DmaMap::get()->getHostAddr(guest_paddr);
}

void android_goldfish_dma_invalidate_host_mappings() {
    android::DmaMap::get()->invalidateHostMappings();
}
