#include "android/emulation/goldfish_dma.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

static GoldfishDmaRegion* sGoldfishDMA = nullptr;

GoldfishDmaRegion* android_goldfish_dma_get() { return sGoldfishDMA; }
void android_goldfish_dma_set(GoldfishDmaRegion* x) { sGoldfishDMA = x; }

