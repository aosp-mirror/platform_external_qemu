
#include "android/emulation/GoldfishDma.h"
#include "emugl/common/dma_device.h"

static void* defaultDmaGetHostAddr(uint64_t guest_paddr) { return nullptr; }
static void defaultDmaUnlock(uint64_t addr) { }

namespace emugl {

emugl_dma_get_host_addr_t g_emugl_dma_get_host_addr = defaultDmaGetHostAddr;
emugl_dma_unlock_t g_emugl_dma_unlock = defaultDmaUnlock;

void set_emugl_dma_get_host_addr(emugl_dma_get_host_addr_t f) {
    g_emugl_dma_get_host_addr = f;
}

void set_emugl_dma_unlock(emugl_dma_unlock_t f) {
    g_emugl_dma_unlock = f;
}

}  // namespace emugl
