
#include "android/emulation/GoldfishDma.h"
#include "emugl/common/dma_device.h"

static void* defaultDmaGetHostAddr(uint64_t guest_paddr) { return nullptr; }

namespace emugl {

emugl_dma_get_host_addr_t g_emugl_dma_get_host_addr = defaultDmaGetHostAddr;

void set_emugl_dma_get_host_addr(emugl_dma_get_host_addr_t f) {
    g_emugl_dma_get_host_addr = f;
}

}  // namespace emugl
