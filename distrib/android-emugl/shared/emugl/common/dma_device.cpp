
#include "android/emulation/goldfish_dma.h"
#include "emugl/common/dma_device.h"

static GoldfishDmaRegion* defaultDmaGetter(void) {
    return nullptr;
}

emugl_dma_get_t dma_getter = defaultDmaGetter;

void set_dma_getter(emugl_dma_get_t f) {
    dma_getter = f;
}
