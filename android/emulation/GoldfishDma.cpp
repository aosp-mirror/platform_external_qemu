#include "android/emulation/GoldfishDma.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

static GoldfishDmaRegion* sGoldfishDMA = nullptr;

void android_goldfish_dma_set(GoldfishDmaRegion* x) { sGoldfishDMA = x; }
void* android_goldfish_dma_read(uint32_t offset) {
    return (void*)((char*)sGoldfishDMA + offset);
}

