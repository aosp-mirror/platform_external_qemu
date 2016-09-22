
#include "android/emulation/GoldfishDma.h"
#include "emugl/common/dma_device.h"

static void* defaultDmaReader(uint32_t offset) {
    return nullptr;
}

emugl_dma_read_t g_emugl_dma_read = defaultDmaReader;

void set_emugl_dma_read(emugl_dma_read_t f) {
    g_emugl_dma_read = f;
}
