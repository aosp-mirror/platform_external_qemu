#include "android/emulation/GoldfishDma.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

static GoldfishDmaRegion* sGoldfishDMA = nullptr;

uint32_t android_goldfish_dma_get_bufsize() { return kDmaBufSizeMB * 1024 * 1024; }
void android_goldfish_dma_init(GoldfishDmaRegion* x) { sGoldfishDMA = x; }
void* android_goldfish_dma_read(uint32_t offset) {
    return (void*)((char*)sGoldfishDMA + offset);
}

